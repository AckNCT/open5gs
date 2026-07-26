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

#include "ogs-sctp.h"
#include "ogs-lcs-ap.h"

#include "mme-event.h"
#include "mme-sm.h"

#include "lcs-ap-path.h"
#include "lcs-ap-build.h"

int lcs_ap_open(void)
{
    mme_esmlc_t *esmlc = NULL;

    /* SLs (LCS-AP) is client-mode on the MME: the E-SMLC listens and the MME
     * dials in. Init the per-E-SMLC FSM; its initial state starts the
     * connect-retry timer and calls lcs_ap_client(). */
    ogs_list_for_each(&mme_self()->esmlc_list, esmlc) {
        mme_event_t e;

        memset(&e, 0, sizeof(e));
        e.esmlc = esmlc;

        ogs_fsm_init(&esmlc->sm,
                lcs_ap_state_initial, lcs_ap_state_final, &e);
    }

    return OGS_OK;
}

void lcs_ap_close(void)
{
    mme_esmlc_t *esmlc = NULL;

    ogs_list_for_each(&mme_self()->esmlc_list, esmlc) {
        mme_event_t e;
        memset(&e, 0, sizeof(e));
        e.esmlc = esmlc;

        ogs_fsm_fini(&esmlc->sm, &e);
    }
}

int lcs_ap_send(ogs_sock_t *sock, ogs_pkbuf_t *pkbuf, uint16_t stream_no)
{
    int sent;

    ogs_assert(sock);
    ogs_assert(pkbuf);

    sent = ogs_sctp_sendmsg(sock, pkbuf->data, pkbuf->len,
            NULL, OGS_SCTP_LCS_AP_PPID, stream_no);
    if (sent < 0 || sent != pkbuf->len) {
        ogs_error("ogs_sctp_sendmsg(len:%d,ssn:%d) error (%d:%s)",
                pkbuf->len, stream_no, errno, strerror(errno));
        ogs_pkbuf_free(pkbuf);
        return OGS_ERROR;
    }

    ogs_pkbuf_free(pkbuf);
    return OGS_OK;
}

int lcs_ap_send_to_esmlc(
        mme_esmlc_t *esmlc, ogs_pkbuf_t *pkbuf, uint16_t stream_no)
{
    ogs_sock_t *sock = NULL;

    ogs_assert(esmlc);
    ogs_assert(pkbuf);
    sock = esmlc->sock;
    ogs_assert(sock);

    ogs_debug("    StreamNO[%d] E-SMLC-IP[%s]",
            stream_no, ogs_sockaddr_to_string_static(esmlc->sa_list));

    return lcs_ap_send(sock, pkbuf, stream_no);
}

int lcs_ap_send_location_request(
        mme_esmlc_t *esmlc, const char *imsi_bcd, uint32_t correlation_id)
{
    int rv;
    ogs_pkbuf_t *pkbuf = NULL;

    ogs_assert(esmlc);
    ogs_assert(imsi_bcd);

    ogs_info("[LCS-AP] Tx Location-Service-Request IMSI[%s] corr[0x%08x]",
            imsi_bcd, correlation_id);

    pkbuf = lcs_ap_build_location_request(imsi_bcd, correlation_id);
    if (!pkbuf) {
        ogs_error("lcs_ap_build_location_request() failed");
        return OGS_ERROR;
    }

    rv = lcs_ap_send_to_esmlc(esmlc, pkbuf, esmlc->ostream_id);
    ogs_expect(rv == OGS_OK);

    return rv;
}
