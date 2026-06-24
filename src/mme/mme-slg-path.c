/*
 * Copyright (C) 2026 by the M-Time Simulation project
 *
 * This file is part of Open5GS (AckNCT/open5gs `mtime-lcs` fork).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ogs-diameter-slg.h"

#include "mme-context.h"
#include "mme-event.h"
#include "mme-slg-path.h"
#include "lcs-ap-path.h"

static struct disp_hdl *hdl_slg_plr = NULL;

/*
 * SLg <-> SLs correlation.
 *
 * A Provide-Location-Request arrives on a freeDiameter thread; the matching
 * SLs Location-Service-Request must be sent from the MME event loop, and the
 * Provide-Location-Answer can only be completed once the SLs Location-Service-
 * Response comes back (also on the event loop). We therefore defer the PLA:
 * the callback parks the half-built answer in a small table keyed by a unique
 * Correlation-ID, hands the id to the event loop (MME_EVENT_SLG_PLR), and the
 * LCS-AP response handler later completes and sends the parked answer.
 */
#define SLG_MAX_PENDING 32

typedef struct slg_pending_s {
    bool in_use;
    uint32_t correlation_id;
    char imsi_bcd[OGS_MAX_IMSI_BCD_LEN+1];
    struct msg *ans;
} slg_pending_t;

static slg_pending_t slg_pending[SLG_MAX_PENDING];
static uint32_t slg_corr_next = 0x100; /* keep clear of the SLs self-test id (1) */
static ogs_thread_mutex_t slg_mutex;

static uint32_t slg_pending_add(const char *imsi_bcd, struct msg *ans)
{
    uint32_t cid = 0;
    int i;

    ogs_thread_mutex_lock(&slg_mutex);
    for (i = 0; i < SLG_MAX_PENDING; i++) {
        if (!slg_pending[i].in_use) {
            cid = slg_corr_next++;
            if (slg_corr_next == 0)
                slg_corr_next = 0x100;
            slg_pending[i].in_use = true;
            slg_pending[i].correlation_id = cid;
            ogs_cpystrn(slg_pending[i].imsi_bcd, imsi_bcd,
                    sizeof(slg_pending[i].imsi_bcd));
            slg_pending[i].ans = ans;
            break;
        }
    }
    ogs_thread_mutex_unlock(&slg_mutex);
    return cid; /* 0 if the table is full */
}

static bool slg_pending_peek_imsi(uint32_t cid, char *out, size_t len)
{
    bool found = false;
    int i;

    ogs_thread_mutex_lock(&slg_mutex);
    for (i = 0; i < SLG_MAX_PENDING; i++) {
        if (slg_pending[i].in_use && slg_pending[i].correlation_id == cid) {
            ogs_cpystrn(out, slg_pending[i].imsi_bcd, len);
            found = true;
            break;
        }
    }
    ogs_thread_mutex_unlock(&slg_mutex);
    return found;
}

static struct msg *slg_pending_take(uint32_t cid)
{
    struct msg *ans = NULL;
    int i;

    ogs_thread_mutex_lock(&slg_mutex);
    for (i = 0; i < SLG_MAX_PENDING; i++) {
        if (slg_pending[i].in_use && slg_pending[i].correlation_id == cid) {
            ans = slg_pending[i].ans;
            slg_pending[i].in_use = false;
            slg_pending[i].ans = NULL;
            break;
        }
    }
    ogs_thread_mutex_unlock(&slg_mutex);
    return ans;
}

/* Provide-Location-Request handler: GMLC -> MME (SLg), runs on an fd thread. */
static int mme_slg_plr_cb(struct msg **msg, struct avp *avp_unused,
        struct session *session, void *opaque, enum disp_action *act)
{
    int ret, rv;
    struct msg *qry, *ans;
    struct avp *avp;
    struct avp_hdr *hdr;
    union avp_value val;
    char imsi_bcd[OGS_MAX_IMSI_BCD_LEN+1] = "";
    uint32_t correlation_id;
    mme_event_t *e;

    ogs_assert(msg);
    ogs_debug("[SLg] Rx Provide-Location-Request");

    qry = *msg;
    ret = fd_msg_new_answer_from_req(fd_g_config->cnf_dict, msg, 0);
    ogs_assert(ret == 0);
    ans = *msg;

    /* Auth-Session-State = NO_STATE_MAINTAINED */
    ret = fd_msg_avp_new(ogs_diam_auth_session_state, 0, &avp);
    ogs_assert(ret == 0);
    val.i32 = OGS_DIAM_AUTH_SESSION_NO_STATE_MAINTAINED;
    ret = fd_msg_avp_setvalue(avp, &val);
    ogs_assert(ret == 0);
    ret = fd_msg_avp_add(ans, MSG_BRW_LAST_CHILD, avp);
    ogs_assert(ret == 0);

    /* Target subscriber IMSI from User-Name. */
    ret = fd_msg_search_avp(qry, ogs_diam_user_name, &avp);
    if (ret == 0 && avp) {
        ret = fd_msg_avp_hdr(avp, &hdr);
        if (ret == 0 && hdr->avp_value &&
                hdr->avp_value->os.len <= OGS_MAX_IMSI_BCD_LEN) {
            memcpy(imsi_bcd, hdr->avp_value->os.data, hdr->avp_value->os.len);
            imsi_bcd[hdr->avp_value->os.len] = '\0';
        }
    }

    /* Result-Code + Origin now; the Location-Estimate is filled in when the
     * SLs Location-Service-Response arrives (mme_slg_answer_location). */
    ret = fd_msg_rescode_set(ans, (char *)"DIAMETER_SUCCESS", NULL, NULL, 1);
    ogs_assert(ret == 0);
    ret = fd_msg_add_origin(ans, 0);
    ogs_assert(ret == 0);

    /* Park the answer and hand the SLs trigger to the event loop. */
    correlation_id = slg_pending_add(imsi_bcd, ans);
    if (correlation_id == 0) {
        ogs_error("[SLg] pending table full; answering PLR without location");
        ret = fd_msg_send(msg, NULL, NULL);
        ogs_assert(ret == 0);
        return 0;
    }
    *msg = NULL; /* take ownership: the answer is sent later */

    e = mme_event_new(MME_EVENT_SLG_PLR);
    ogs_assert(e);
    e->slg_correlation_id = correlation_id;
    rv = ogs_queue_push(ogs_app()->queue, e);
    if (rv != OGS_OK) {
        ogs_error("[SLg] ogs_queue_push() failed: %d", (int)rv);
        mme_event_free(e);
        ans = slg_pending_take(correlation_id);
        if (ans)
            (void)fd_msg_send(&ans, NULL, NULL);
        return 0;
    }
    ogs_pollset_notify(ogs_app()->pollset);

    ogs_info("[SLg] Provide-Location-Request IMSI[%s] corr[0x%08x] -> SLs",
            imsi_bcd, correlation_id);
    return 0;
}

/* Event loop: drive the SLs Location-Service-Request for a parked PLR. */
void mme_slg_handle_plr(uint32_t correlation_id)
{
    char imsi_bcd[OGS_MAX_IMSI_BCD_LEN+1] = "";
    mme_esmlc_t *esmlc = NULL;

    if (!slg_pending_peek_imsi(correlation_id, imsi_bcd, sizeof(imsi_bcd))) {
        ogs_warn("[SLg] handle_plr: no pending corr[0x%08x]", correlation_id);
        return;
    }

    esmlc = ogs_list_first(&mme_self()->esmlc_list);
    if (!esmlc || !esmlc->sock) {
        ogs_error("[SLg] no SLs connection to E-SMLC; answering PLR "
                "without location");
        mme_slg_answer_location(correlation_id, NULL, 0);
        return;
    }

    ogs_info("[SLg] corr[0x%08x] IMSI[%s]: Tx SLs Location-Service-Request",
            correlation_id, imsi_bcd);
    lcs_ap_send_location_request(esmlc, imsi_bcd, correlation_id);
}

/* Complete and send the parked PLA with the GAD Location-Estimate from SLs. */
void mme_slg_answer_location(uint32_t correlation_id,
        const uint8_t *gad, int gad_len)
{
    int ret;
    struct msg *ans;
    struct avp *avp;
    union avp_value val;

    ans = slg_pending_take(correlation_id);
    if (!ans) {
        /* No parked PLA (e.g. the SLs self-test correlation id). */
        ogs_debug("[SLg] answer: no pending corr[0x%08x]", correlation_id);
        return;
    }

    if (gad && gad_len > 0 && ogs_diam_slg_location_estimate) {
        ret = fd_msg_avp_new(ogs_diam_slg_location_estimate, 0, &avp);
        ogs_assert(ret == 0);
        val.os.data = (uint8_t *)gad;
        val.os.len = gad_len;
        ret = fd_msg_avp_setvalue(avp, &val);
        ogs_assert(ret == 0);
        ret = fd_msg_avp_add(ans, MSG_BRW_LAST_CHILD, avp);
        ogs_assert(ret == 0);
    }

    ret = fd_msg_send(&ans, NULL, NULL);
    ogs_assert(ret == 0);

    ogs_info("[SLg] Tx Provide-Location-Answer corr[0x%08x] (%d-byte estimate)",
            correlation_id, gad_len);
}

int mme_slg_init(void)
{
    int ret;
    struct disp_when data;

    ogs_thread_mutex_init(&slg_mutex);
    memset(slg_pending, 0, sizeof(slg_pending));

    /* Install the SLg dictionary objects. */
    ret = ogs_diam_slg_init();
    ogs_assert(ret == OGS_OK);

    /* Server handler for Provide-Location-Request. */
    memset(&data, 0, sizeof(data));
    data.app = ogs_diam_slg_application;
    data.command = ogs_diam_slg_cmd_plr;
    ret = fd_disp_register(mme_slg_plr_cb, DISP_HOW_CC, &data, NULL,
                &hdl_slg_plr);
    ogs_assert(ret == 0);

    /* Advertise SLg application support to peers (the GMLC). */
    ret = fd_disp_app_support(ogs_diam_slg_application, ogs_diam_vendor, 1, 0);
    ogs_assert(ret == 0);

    ogs_info("[SLg] Diameter front door ready (app %d, PLR/PLA)",
            OGS_DIAM_SLG_APPLICATION_ID);

    return OGS_OK;
}

void mme_slg_final(void)
{
    if (hdl_slg_plr)
        (void) fd_disp_unregister(&hdl_slg_plr, NULL);
    ogs_thread_mutex_destroy(&slg_mutex);
}
