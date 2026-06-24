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
#include "mme-slg-path.h"

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
    case LCS_AP_LCS_AP_PDU_PR_successfulOutcome: {
        LCS_AP_SuccessfulOutcome_t *out = message.choice.successfulOutcome;
        uint32_t correlation_id = 0;
        uint8_t gad[7] = {0};
        int gad_len = 0;

        ogs_info("[LCS-AP] Rx Location-Service-Response (procedureCode %d) "
                "from E-SMLC [%s]",
                (int)out->procedureCode,
                ogs_sockaddr_to_string_static(esmlc->sa_list));

        if (out->value.present ==
                LCS_AP_SuccessfulOutcome__value_PR_Location_Response) {
            LCS_AP_Location_Response_t *resp =
                    &out->value.choice.Location_Response;
            int i;
            for (i = 0; i < resp->protocolIEs.list.count; i++) {
                LCS_AP_Location_Response_IEs_t *ie =
                        resp->protocolIEs.list.array[i];
                if (!ie)
                    continue;
                if (ie->id == LCS_AP_id_Correlation_ID && ie->value.present ==
                        LCS_AP_Location_Response_IEs__value_PR_Correlation_ID) {
                    LCS_AP_Correlation_ID_t *c =
                            &ie->value.choice.Correlation_ID;
                    if (c->buf && c->size >= 4)
                        correlation_id = ((uint32_t)c->buf[0] << 24) |
                                ((uint32_t)c->buf[1] << 16) |
                                ((uint32_t)c->buf[2] << 8) | c->buf[3];
                } else if (ie->id == LCS_AP_id_Location_Estimate &&
                        ie->value.present ==
                        LCS_AP_Location_Response_IEs__value_PR_Geographical_Area) {
                    LCS_AP_Geographical_Area_t *ga =
                            &ie->value.choice.Geographical_Area;
                    if (ga->present == LCS_AP_Geographical_Area_PR_point &&
                            ga->choice.point) {
                        LCS_AP_Geographical_Coordinates_t *gc =
                                &ga->choice.point->geographical_Coordinates;
                        /* Geographical-Coordinates -> GAD ellipsoid point: the
                         * 2^23 latitude / 2^24 longitude scaling is identical. */
                        uint32_t latv = (uint32_t)gc->degreesLatitude & 0x7FFFFF;
                        uint32_t lonv = (uint32_t)gc->degreesLongitude & 0xFFFFFF;
                        if (gc->latitudeSign != 0) /* south */
                            latv |= 0x800000;
                        gad[0] = 0x00; /* shape type 0 = Ellipsoid Point */
                        gad[1] = (latv >> 16) & 0xFF;
                        gad[2] = (latv >> 8) & 0xFF;
                        gad[3] = latv & 0xFF;
                        gad[4] = (lonv >> 16) & 0xFF;
                        gad[5] = (lonv >> 8) & 0xFF;
                        gad[6] = lonv & 0xFF;
                        gad_len = 7;
                    }
                }
            }
        }

        /* Hand the position to the parked SLg PLA (no-op for the self-test id). */
        mme_slg_answer_location(correlation_id, gad_len ? gad : NULL, gad_len);
        break;
    }
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
