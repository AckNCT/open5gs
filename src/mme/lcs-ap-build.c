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

#include "lcs-ap-build.h"

/*
 * Build an SLs Location-Service-Request (TS 29.171) to ask the E-SMLC to
 * locate the given subscriber. A minimal but valid PDU: Correlation-ID,
 * Location-Type (current/geographic), IMSI and LCS-Priority.
 */
ogs_pkbuf_t *lcs_ap_build_location_request(
        const char *imsi_bcd, uint32_t correlation_id)
{
    ogs_lcs_ap_message_t message;
    LCS_AP_InitiatingMessage_t *initiatingMessage = NULL;
    LCS_AP_Location_Request_t *Location_Request = NULL;
    LCS_AP_Location_Request_IEs_t *ie = NULL;

    LCS_AP_Correlation_ID_t *Correlation_ID = NULL;
    LCS_AP_Location_Type_t *Location_Type = NULL;
    LCS_AP_IMSI_t *IMSI = NULL;
    LCS_AP_LCS_Priority_t *LCS_Priority = NULL;

    uint8_t corr_buf[4];
    uint8_t imsi_buf[OGS_MAX_IMSI_LEN];
    int imsi_len = 0;
    uint8_t prio = 0; /* highest priority */

    ogs_assert(imsi_bcd);

    memset(&message, 0, sizeof(message));

    message.present = LCS_AP_LCS_AP_PDU_PR_initiatingMessage;
    message.choice.initiatingMessage =
        CALLOC(1, sizeof(LCS_AP_InitiatingMessage_t));
    initiatingMessage = message.choice.initiatingMessage;
    initiatingMessage->procedureCode = LCS_AP_id_Location_Service_Request;
    initiatingMessage->criticality = LCS_AP_Criticality_reject;
    initiatingMessage->value.present =
        LCS_AP_InitiatingMessage__value_PR_Location_Request;

    Location_Request = &initiatingMessage->value.choice.Location_Request;

    /* Correlation-ID (4 octets), to match request and response. */
    ie = CALLOC(1, sizeof(LCS_AP_Location_Request_IEs_t));
    ASN_SEQUENCE_ADD(&Location_Request->protocolIEs.list, ie);
    ie->id = LCS_AP_id_Correlation_ID;
    ie->criticality = LCS_AP_Criticality_reject;
    ie->value.present = LCS_AP_Location_Request_IEs__value_PR_Correlation_ID;
    Correlation_ID = &ie->value.choice.Correlation_ID;
    corr_buf[0] = (correlation_id >> 24) & 0xff;
    corr_buf[1] = (correlation_id >> 16) & 0xff;
    corr_buf[2] = (correlation_id >> 8) & 0xff;
    corr_buf[3] = correlation_id & 0xff;
    ogs_asn_buffer_to_OCTET_STRING(corr_buf, sizeof(corr_buf), Correlation_ID);

    /* Location-Type: geographic information (current location). */
    ie = CALLOC(1, sizeof(LCS_AP_Location_Request_IEs_t));
    ASN_SEQUENCE_ADD(&Location_Request->protocolIEs.list, ie);
    ie->id = LCS_AP_id_Location_Type;
    ie->criticality = LCS_AP_Criticality_reject;
    ie->value.present = LCS_AP_Location_Request_IEs__value_PR_Location_Type;
    Location_Type = &ie->value.choice.Location_Type;
    *Location_Type = LCS_AP_Location_Type_geographic_Information;

    /* IMSI (TBCD-encoded). */
    ie = CALLOC(1, sizeof(LCS_AP_Location_Request_IEs_t));
    ASN_SEQUENCE_ADD(&Location_Request->protocolIEs.list, ie);
    ie->id = LCS_AP_id_IMSI;
    ie->criticality = LCS_AP_Criticality_reject;
    ie->value.present = LCS_AP_Location_Request_IEs__value_PR_IMSI;
    IMSI = &ie->value.choice.IMSI;
    ogs_bcd_to_buffer(imsi_bcd, imsi_buf, &imsi_len);
    ogs_asn_buffer_to_OCTET_STRING(imsi_buf, imsi_len, IMSI);

    /* LCS-Priority (1 octet). */
    ie = CALLOC(1, sizeof(LCS_AP_Location_Request_IEs_t));
    ASN_SEQUENCE_ADD(&Location_Request->protocolIEs.list, ie);
    ie->id = LCS_AP_id_LCS_Priority;
    ie->criticality = LCS_AP_Criticality_ignore;
    ie->value.present = LCS_AP_Location_Request_IEs__value_PR_LCS_Priority;
    LCS_Priority = &ie->value.choice.LCS_Priority;
    ogs_asn_buffer_to_OCTET_STRING(&prio, 1, LCS_Priority);

    /* ogs_lcs_ap_encode() -> ogs_asn_encode() frees `message` internally after
     * encoding; freeing it again here would double-free (talloc abort / crash).
     * Mirrors src/mme/s1ap-build.c, which just returns ogs_s1ap_encode(&pdu). */
    return ogs_lcs_ap_encode(&message);
}

ogs_pkbuf_t *lcs_ap_build_connection_oriented_info(
        uint32_t correlation_id, long payload_type,
        const uint8_t *apdu, size_t apdu_len)
{
    ogs_lcs_ap_message_t message;
    LCS_AP_InitiatingMessage_t *initiatingMessage = NULL;
    LCS_AP_Connection_Oriented_Information_t *ConnInfo = NULL;
    LCS_AP_Connection_Oriented_Information_IEs_t *ie = NULL;

    LCS_AP_APDU_t *APDU = NULL;
    LCS_AP_Correlation_ID_t *Correlation_ID = NULL;
    LCS_AP_Payload_Type_t *Payload_Type = NULL;

    uint8_t corr_buf[4];

    ogs_assert(apdu);

    memset(&message, 0, sizeof(message));

    message.present = LCS_AP_LCS_AP_PDU_PR_initiatingMessage;
    message.choice.initiatingMessage =
        CALLOC(1, sizeof(LCS_AP_InitiatingMessage_t));
    initiatingMessage = message.choice.initiatingMessage;
    initiatingMessage->procedureCode =
        LCS_AP_id_Connection_Oriented_Information_Transfer;
    initiatingMessage->criticality = LCS_AP_Criticality_reject;
    initiatingMessage->value.present =
        LCS_AP_InitiatingMessage__value_PR_Connection_Oriented_Information;

    ConnInfo = &initiatingMessage->value.choice.Connection_Oriented_Information;

    /* APDU (id 1) — the LPP/LPPa response PDU. */
    ie = CALLOC(1, sizeof(LCS_AP_Connection_Oriented_Information_IEs_t));
    ASN_SEQUENCE_ADD(&ConnInfo->protocolIEs.list, ie);
    ie->id = LCS_AP_id_APDU;
    ie->criticality = LCS_AP_Criticality_reject;
    ie->value.present =
        LCS_AP_Connection_Oriented_Information_IEs__value_PR_APDU;
    APDU = &ie->value.choice.APDU;
    ogs_asn_buffer_to_OCTET_STRING((void *)apdu, (int)apdu_len, APDU);

    /* Correlation-ID (id 2) — echo the request's, so the E-SMLC can match. */
    ie = CALLOC(1, sizeof(LCS_AP_Connection_Oriented_Information_IEs_t));
    ASN_SEQUENCE_ADD(&ConnInfo->protocolIEs.list, ie);
    ie->id = LCS_AP_id_Correlation_ID;
    ie->criticality = LCS_AP_Criticality_reject;
    ie->value.present =
        LCS_AP_Connection_Oriented_Information_IEs__value_PR_Correlation_ID;
    Correlation_ID = &ie->value.choice.Correlation_ID;
    corr_buf[0] = (correlation_id >> 24) & 0xff;
    corr_buf[1] = (correlation_id >> 16) & 0xff;
    corr_buf[2] = (correlation_id >> 8) & 0xff;
    corr_buf[3] = correlation_id & 0xff;
    ogs_asn_buffer_to_OCTET_STRING(corr_buf, sizeof(corr_buf), Correlation_ID);

    /* Payload-Type (id 15) — lPP or lPPa. */
    ie = CALLOC(1, sizeof(LCS_AP_Connection_Oriented_Information_IEs_t));
    ASN_SEQUENCE_ADD(&ConnInfo->protocolIEs.list, ie);
    ie->id = LCS_AP_id_Payload_Type;
    ie->criticality = LCS_AP_Criticality_reject;
    ie->value.present =
        LCS_AP_Connection_Oriented_Information_IEs__value_PR_Payload_Type;
    Payload_Type = &ie->value.choice.Payload_Type;
    *Payload_Type = payload_type;

    /* ogs_lcs_ap_encode() frees `message` internally (see above). */
    return ogs_lcs_ap_encode(&message);
}
