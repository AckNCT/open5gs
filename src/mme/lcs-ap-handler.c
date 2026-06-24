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

#include "mme-context.h"
#include "ogs-lcs-ap.h"

#include "lcs-ap-handler.h"

/*
 * Decode and log an inbound SLs LCS-AP PDU from the E-SMLC. The expected
 * message in the positioning flow is the SuccessfulOutcome carrying a
 * Location-Service-Response. Deep IE extraction (the Location-Estimate) is
 * wired up with the positioning glue in increment 3c.
 */
void lcs_ap_handle_message(mme_esmlc_t *esmlc, ogs_pkbuf_t *pkbuf)
{
    int rv;
    ogs_lcs_ap_message_t message;

    ogs_assert(esmlc);
    ogs_assert(pkbuf);

    memset(&message, 0, sizeof(message));
    rv = ogs_lcs_ap_decode(&message, pkbuf);
    if (rv != OGS_OK) {
        ogs_error("[LCS-AP] Failed to decode message from E-SMLC");
        return;
    }

    switch (message.present) {
    case LCS_AP_LCS_AP_PDU_PR_initiatingMessage:
        ogs_info("[LCS-AP] Rx InitiatingMessage (procedureCode %d) "
                "from E-SMLC [%s]",
                (int)message.choice.initiatingMessage->procedureCode,
                ogs_sockaddr_to_string_static(esmlc->sa_list));
        break;
    case LCS_AP_LCS_AP_PDU_PR_successfulOutcome:
        ogs_info("[LCS-AP] Rx Location-Service-Response (procedureCode %d) "
                "from E-SMLC [%s]",
                (int)message.choice.successfulOutcome->procedureCode,
                ogs_sockaddr_to_string_static(esmlc->sa_list));
        break;
    case LCS_AP_LCS_AP_PDU_PR_unsuccessfulOutcome:
        ogs_warn("[LCS-AP] Rx UnsuccessfulOutcome (procedureCode %d) "
                "from E-SMLC [%s]",
                (int)message.choice.unsuccessfulOutcome->procedureCode,
                ogs_sockaddr_to_string_static(esmlc->sa_list));
        break;
    default:
        ogs_warn("[LCS-AP] Rx unknown PDU present %d", message.present);
        break;
    }

    ogs_lcs_ap_free(&message);
}
