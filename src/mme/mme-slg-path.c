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
#include "mme-slg-path.h"

static struct disp_hdl *hdl_slg_plr = NULL;

/*
 * GAD "Ellipsoid Point" shape (3GPP TS 23.032 #7.3.2): 7 octets,
 * shape-type(4b)+spare(4b) | latitude(sign + 23 bits) | longitude(24 bits).
 * The GMLC decodes this exact layout from the PLA Location-Estimate.
 */
static void slg_gad_ellipsoid_point(uint8_t *b, double lat, double lon)
{
    double alat = lat < 0.0 ? -lat : lat;
    uint32_t lat_i = (uint32_t)(alat / 90.0 * (double)(1 << 23)) & 0x7FFFFF;
    int32_t lon_i = (int32_t)(lon / 360.0 * (double)(1 << 24));

    if (lat < 0.0)
        lat_i |= 0x800000;

    b[0] = 0x00; /* shape type 0 = Ellipsoid Point */
    b[1] = (lat_i >> 16) & 0xFF;
    b[2] = (lat_i >> 8) & 0xFF;
    b[3] = lat_i & 0xFF;
    b[4] = (lon_i >> 16) & 0xFF;
    b[5] = (lon_i >> 8) & 0xFF;
    b[6] = lon_i & 0xFF;
}

/* Provide-Location-Request handler: GMLC -> MME (SLg). */
static int mme_slg_plr_cb(struct msg **msg, struct avp *avp_unused,
        struct session *session, void *opaque, enum disp_action *act)
{
    int ret;
    struct msg *qry, *ans;
    struct avp *avp;
    struct avp_hdr *hdr;
    union avp_value val;
    char imsi_bcd[OGS_MAX_IMSI_BCD_LEN+1] = "";
    uint8_t gad[7];

    ogs_assert(msg);
    ogs_debug("[SLg] Rx Provide-Location-Request");

    /* Create the answer from the request (copies Session-Id). */
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

    /* Target subscriber: IMSI carried in User-Name. */
    ret = fd_msg_search_avp(qry, ogs_diam_user_name, &avp);
    if (ret == 0 && avp) {
        ret = fd_msg_avp_hdr(avp, &hdr);
        if (ret == 0 && hdr->avp_value &&
                hdr->avp_value->os.len <= OGS_MAX_IMSI_BCD_LEN) {
            memcpy(imsi_bcd, hdr->avp_value->os.data, hdr->avp_value->os.len);
            imsi_bcd[hdr->avp_value->os.len] = '\0';
        }
    }
    ogs_info("[SLg] Provide-Location-Request IMSI[%s]", imsi_bcd);

    /*
     * Increment B2b: answer synchronously with a placeholder Location-Estimate
     * to prove the SLg Diameter front door end to end. B2c replaces this with
     * the real path: trigger the SLs Location-Service-Request to the E-SMLC,
     * hold this PLA, and answer it when the Location-Service-Response arrives.
     */
    slg_gad_ellipsoid_point(gad, 32.0855, 34.7822);
    ret = fd_msg_avp_new(ogs_diam_slg_location_estimate, 0, &avp);
    ogs_assert(ret == 0);
    val.os.data = gad;
    val.os.len = sizeof(gad);
    ret = fd_msg_avp_setvalue(avp, &val);
    ogs_assert(ret == 0);
    ret = fd_msg_avp_add(ans, MSG_BRW_LAST_CHILD, avp);
    ogs_assert(ret == 0);

    /* Result-Code = DIAMETER_SUCCESS */
    ret = fd_msg_rescode_set(ans, (char *)"DIAMETER_SUCCESS", NULL, NULL, 1);
    ogs_assert(ret == 0);

    /* Origin-Host / Origin-Realm */
    ret = fd_msg_add_origin(ans, 0);
    ogs_assert(ret == 0);

    ret = fd_msg_send(msg, NULL, NULL);
    ogs_assert(ret == 0);

    ogs_info("[SLg] Tx Provide-Location-Answer IMSI[%s] (placeholder estimate)",
            imsi_bcd);

    return 0;
}

int mme_slg_init(void)
{
    int ret;
    struct disp_when data;

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
}
