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

#include "mme-context.h"
#include "mme-event.h"
#include "mme-timer.h"
#include "mme-sm.h"

#include "lcs-ap-path.h"
#include "lcs-ap-handler.h"

/*
 * SLs (LCS-AP, TS 29.171) is server-mode on the MME: the MME listens
 * (lcs_ap_server) and the E-SMLC is the SCTP client that dials in. So the FSM
 * has no outbound-connect/retry logic — it idles in `will_accept` until the
 * accept handler attaches an association and posts MME_EVENT_LCS_AP_LO_ACCEPT.
 */

void lcs_ap_state_initial(ogs_fsm_t *s, mme_event_t *e)
{
    mme_esmlc_t *esmlc = NULL;
    ogs_assert(s);
    ogs_assert(e);

    mme_sm_debug(e);

    esmlc = e->esmlc;
    ogs_assert(esmlc);

    OGS_FSM_TRAN(s, &lcs_ap_state_will_accept);
}

void lcs_ap_state_final(ogs_fsm_t *s, mme_event_t *e)
{
    ogs_assert(s);
    ogs_assert(e);

    mme_sm_debug(e);
}

void lcs_ap_state_will_accept(ogs_fsm_t *s, mme_event_t *e)
{
    mme_esmlc_t *esmlc = NULL;
    ogs_assert(s);
    ogs_assert(e);

    mme_sm_debug(e);

    esmlc = e->esmlc;
    ogs_assert(esmlc);

    switch (e->id) {
    case OGS_FSM_ENTRY_SIG:
        /* The SLs SCTP server is listening; wait for the E-SMLC to connect. */
        break;
    case OGS_FSM_EXIT_SIG:
        break;
    case MME_EVENT_LCS_AP_LO_ACCEPT:
    case MME_EVENT_LCS_AP_LO_SCTP_COMM_UP:
        OGS_FSM_TRAN(s, lcs_ap_state_connected);
        break;
    default:
        ogs_error("Unknown event %s", mme_event_get_name(e));
        break;
    }
}

void lcs_ap_state_connected(ogs_fsm_t *s, mme_event_t *e)
{
    mme_esmlc_t *esmlc = NULL;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_assert(s);
    ogs_assert(e);

    mme_sm_debug(e);

    esmlc = e->esmlc;
    ogs_assert(esmlc);

    switch (e->id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("[LCS-AP] SLs association up with E-SMLC");
        /* Optional self-test: send a Location-Service-Request when the E-SMLC
         * connects (the SLg PLR is the real trigger in normal operation). */
        if (mme_self()->sls_test_imsi) {
            ogs_info("[LCS-AP] Sending test Location-Service-Request "
                    "for IMSI[%s]", mme_self()->sls_test_imsi);
            lcs_ap_send_location_request(
                    esmlc, mme_self()->sls_test_imsi, 1);
        }
        break;
    case OGS_FSM_EXIT_SIG:
        break;
    case MME_EVENT_LCS_AP_LO_SCTP_COMM_UP:
        /* An accepted association may also surface COMM_UP via its notification;
         * we are already connected, so ignore it. */
        break;
    case MME_EVENT_LCS_AP_LO_CONNREFUSED:
        mme_esmlc_close(esmlc);
        OGS_FSM_TRAN(s, lcs_ap_state_will_accept);
        break;
    case MME_EVENT_LCS_AP_MESSAGE:
        pkbuf = e->pkbuf;
        ogs_assert(pkbuf);
        lcs_ap_handle_message(esmlc, pkbuf);
        break;
    default:
        ogs_error("Unknown event %s", mme_event_get_name(e));
        break;
    }
}

void lcs_ap_state_exception(ogs_fsm_t *s, mme_event_t *e)
{
    ogs_assert(s);
    ogs_assert(e);

    mme_sm_debug(e);

    switch (e->id) {
    case OGS_FSM_ENTRY_SIG:
        break;
    case OGS_FSM_EXIT_SIG:
        break;
    default:
        ogs_error("Unknown event %s", mme_event_get_name(e));
        break;
    }
}
