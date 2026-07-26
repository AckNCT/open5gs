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
 * SLs (LCS-AP, TS 29.171) is client-mode on the MME: the E-SMLC listens
 * (lcs_ap server, in the E-SMLC process) and the MME dials in. The FSM
 * mirrors sgsap-sm.c's connect/retry logic: `will_connect` starts a timer and
 * calls lcs_ap_client(); on failure the timer fires and retries.
 */

void lcs_ap_state_initial(ogs_fsm_t *s, mme_event_t *e)
{
    mme_esmlc_t *esmlc = NULL;
    ogs_assert(s);
    ogs_assert(e);

    mme_sm_debug(e);

    esmlc = e->esmlc;
    ogs_assert(esmlc);

    esmlc->t_conn = ogs_timer_add(ogs_app()->timer_mgr,
            mme_timer_lcs_ap_cli_conn_to_srv, esmlc);
    if (!esmlc->t_conn) {
        ogs_error("ogs_timer_add() failed");
        return;
    }

    OGS_FSM_TRAN(s, &lcs_ap_state_will_connect);
}

void lcs_ap_state_final(ogs_fsm_t *s, mme_event_t *e)
{
    mme_esmlc_t *esmlc = NULL;
    ogs_assert(s);
    ogs_assert(e);

    mme_sm_debug(e);

    esmlc = e->esmlc;
    ogs_assert(esmlc);

    ogs_timer_delete(esmlc->t_conn);
}

void lcs_ap_state_will_connect(ogs_fsm_t *s, mme_event_t *e)
{
    char buf[OGS_ADDRSTRLEN];

    mme_esmlc_t *esmlc = NULL;
    ogs_sockaddr_t *addr = NULL;
    ogs_assert(s);
    ogs_assert(e);

    mme_sm_debug(e);

    esmlc = e->esmlc;
    ogs_assert(esmlc);

    ogs_assert(esmlc->t_conn);

    switch (e->id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_timer_start(esmlc->t_conn,
                mme_timer_cfg(MME_TIMER_LCS_AP_CLI_CONN_TO_SRV)->duration);
        lcs_ap_client(esmlc);
        break;
    case OGS_FSM_EXIT_SIG:
        ogs_timer_stop(esmlc->t_conn);
        break;
    case MME_EVENT_LCS_AP_TIMER:
        switch (e->timer_id) {
        case MME_TIMER_LCS_AP_CLI_CONN_TO_SRV:
            esmlc = e->esmlc;
            ogs_assert(esmlc);
            addr = esmlc->sa_list;
            ogs_assert(addr);

            ogs_warn("[LCS-AP] Connect to E-SMLC [%s]:%d failed",
                        OGS_ADDR(addr, buf), OGS_PORT(addr));

            ogs_assert(esmlc->t_conn);
            ogs_timer_start(esmlc->t_conn,
                mme_timer_cfg(MME_TIMER_LCS_AP_CLI_CONN_TO_SRV)->duration);

            mme_esmlc_close(esmlc);
            lcs_ap_client(esmlc);
            break;
        default:
            ogs_error("Unknown timer[%s:%d]",
                    mme_timer_get_name(e->timer_id), e->timer_id);
            break;
        }
        break;
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
    case MME_EVENT_LCS_AP_LO_CONNREFUSED:
        mme_esmlc_close(esmlc);
        OGS_FSM_TRAN(s, lcs_ap_state_will_connect);
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
