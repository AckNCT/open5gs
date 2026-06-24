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

/*
 * LCS-AP (SLs interface, 3GPP TS 29.171) — umbrella header.
 * Pulls in the asn1c-generated codec and exposes the encode/decode helpers,
 * mirroring lib/s1ap/ogs-s1ap.h.
 */

#ifndef OGS_LCS_AP_H
#define OGS_LCS_AP_H

#include "core/ogs-core.h"

#include "LCS_AP_APDU.h"
#include "LCS_AP_Accuracy-Fulfillment-Indicator.h"
#include "LCS_AP_Additional-PositioningDataSet.h"
#include "LCS_AP_Additional-PositioningMethodAndUsage.h"
#include "LCS_AP_Altitude-And-Direction.h"
#include "LCS_AP_Altitude.h"
#include "LCS_AP_Angle.h"
#include "LCS_AP_Barometric-Pressure.h"
#include "LCS_AP_Bearing.h"
#include "LCS_AP_C0.h"
#include "LCS_AP_Cell-Portion-ID.h"
#include "LCS_AP_CellIdentity.h"
#include "LCS_AP_Ciphering-Data-Ack.h"
#include "LCS_AP_Ciphering-Data-Error-Report-Contents.h"
#include "LCS_AP_Ciphering-Data-Error-Report.h"
#include "LCS_AP_Ciphering-Data-Set.h"
#include "LCS_AP_Ciphering-Data.h"
#include "LCS_AP_Ciphering-Key-Data-Result.h"
#include "LCS_AP_Ciphering-Key-Data.h"
#include "LCS_AP_Ciphering-Key.h"
#include "LCS_AP_Ciphering-Set-ID.h"
#include "LCS_AP_Civic-Address.h"
#include "LCS_AP_Confidence.h"
#include "LCS_AP_Connection-Oriented-Information.h"
#include "LCS_AP_Connectionless-Information.h"
#include "LCS_AP_Correlation-ID.h"
#include "LCS_AP_Country.h"
#include "LCS_AP_Coverage-Level.h"
#include "LCS_AP_Criticality.h"
#include "LCS_AP_DegreesLatitude.h"
#include "LCS_AP_DegreesLongitude.h"
#include "LCS_AP_Direction-Of-Altitude.h"
#include "LCS_AP_E-CGI.h"
#include "LCS_AP_E-SMLC-ID.h"
#include "LCS_AP_ENB-ID.h"
#include "LCS_AP_Ellipsoid-Arc.h"
#include "LCS_AP_Ellipsoid-Point-With-Altitude-And-Uncertainty-Ellipsoid.h"
#include "LCS_AP_Ellipsoid-Point-With-Altitude.h"
#include "LCS_AP_Ellipsoid-Point-With-Uncertainty-Ellipse.h"
#include "LCS_AP_GNSS-Positioning-Data-Set.h"
#include "LCS_AP_GNSS-Positioning-Method-And-Usage.h"
#include "LCS_AP_Geographical-Area.h"
#include "LCS_AP_Geographical-Coordinates.h"
#include "LCS_AP_Global-eNB-ID.h"
#include "LCS_AP_High-Accuracy-Altitude.h"
#include "LCS_AP_High-Accuracy-DegreesLatitude.h"
#include "LCS_AP_High-Accuracy-DegreesLongitude.h"
#include "LCS_AP_High-Accuracy-Ellipsoid-Point-With-Altitude-And-Scalable-Uncertainty-Ellipsoid.h"
#include "LCS_AP_High-Accuracy-Ellipsoid-Point-With-Altitude-And-Uncertainty-Ellipsoid.h"
#include "LCS_AP_High-Accuracy-Ellipsoid-Point-With-Scalable-Uncertainty-Ellipse.h"
#include "LCS_AP_High-Accuracy-Ellipsoid-Point-With-Uncertainty-Ellipse.h"
#include "LCS_AP_High-Accuracy-Extended-Uncertainty-Code.h"
#include "LCS_AP_High-Accuracy-Extended-Uncertainty-Ellipse.h"
#include "LCS_AP_High-Accuracy-Geographical-Coordinates.h"
#include "LCS_AP_High-Accuracy-Scalable-Uncertainty-Altitude.h"
#include "LCS_AP_High-Accuracy-Scalable-Uncertainty-Ellipse.h"
#include "LCS_AP_High-Accuracy-Uncertainty-Code.h"
#include "LCS_AP_High-Accuracy-Uncertainty-Ellipse.h"
#include "LCS_AP_Home-eNB-ID.h"
#include "LCS_AP_Horizontal-Accuracy.h"
#include "LCS_AP_Horizontal-Speed-And-Bearing.h"
#include "LCS_AP_Horizontal-Velocity-With-Uncertainty.h"
#include "LCS_AP_Horizontal-Velocity.h"
#include "LCS_AP_Horizontal-With-Vertical-Velocity-And-Uncertainty.h"
#include "LCS_AP_Horizontal-With-Vertical-Velocity.h"
#include "LCS_AP_IMEI.h"
#include "LCS_AP_IMSI.h"
#include "LCS_AP_Include-Velocity.h"
#include "LCS_AP_InitiatingMessage.h"
#include "LCS_AP_Inner-Radius.h"
#include "LCS_AP_International-Area-Indication.h"
#include "LCS_AP_LCS-AP-PDU.h"
#include "LCS_AP_LCS-Cause.h"
#include "LCS_AP_LCS-Client-Type.h"
#include "LCS_AP_LCS-Priority.h"
#include "LCS_AP_LCS-QoS.h"
#include "LCS_AP_LCS-Service-Type-ID.h"
#include "LCS_AP_LatitudeSign.h"
#include "LCS_AP_Location-Abort-Request.h"
#include "LCS_AP_Location-Request.h"
#include "LCS_AP_Location-Response.h"
#include "LCS_AP_Location-Type.h"
#include "LCS_AP_Long-Macro-eNB-ID.h"
#include "LCS_AP_Macro-eNB-ID.h"
#include "LCS_AP_Message-Identifier.h"
#include "LCS_AP_Misc-Cause.h"
#include "LCS_AP_MultipleAPDUs.h"
#include "LCS_AP_Network-Element.h"
#include "LCS_AP_Orientation-Major-Axis.h"
#include "LCS_AP_PLMN-ID.h"
#include "LCS_AP_Payload-Type.h"
#include "LCS_AP_Point-With-Uncertainty.h"
#include "LCS_AP_Point.h"
#include "LCS_AP_Polygon-Point.h"
#include "LCS_AP_Polygon.h"
#include "LCS_AP_Positioning-Data-Set.h"
#include "LCS_AP_Positioning-Data.h"
#include "LCS_AP_Positioning-Method-And-Usage.h"
#include "LCS_AP_Presence.h"
#include "LCS_AP_ProcedureCode.h"
#include "LCS_AP_Protocol-Cause.h"
#include "LCS_AP_ProtocolExtensionContainer.h"
#include "LCS_AP_ProtocolExtensionField.h"
#include "LCS_AP_ProtocolExtensionID.h"
#include "LCS_AP_ProtocolIE-Container.h"
#include "LCS_AP_ProtocolIE-ContainerList.h"
#include "LCS_AP_ProtocolIE-Field.h"
#include "LCS_AP_ProtocolIE-ID.h"
#include "LCS_AP_RAT-Type.h"
#include "LCS_AP_Radio-Network-Layer-Cause.h"
#include "LCS_AP_Reset-Acknowledge.h"
#include "LCS_AP_Reset-Request.h"
#include "LCS_AP_Response-Time.h"
#include "LCS_AP_Return-Error-Cause.h"
#include "LCS_AP_Return-Error-Type.h"
#include "LCS_AP_SIB-Types.h"
#include "LCS_AP_Short-Macro-eNB-ID.h"
#include "LCS_AP_Storage-Outcome.h"
#include "LCS_AP_SuccessfulOutcome.h"
#include "LCS_AP_TAIs-List.h"
#include "LCS_AP_TBCD-STRING.h"
#include "LCS_AP_Transport-Layer-Cause.h"
#include "LCS_AP_TriggeringMessage.h"
#include "LCS_AP_UE-Area-Indication.h"
#include "LCS_AP_UE-Country-Determination-Indication.h"
#include "LCS_AP_UE-Positioning-Capability.h"
#include "LCS_AP_Uncertainty-Altitude.h"
#include "LCS_AP_Uncertainty-Code.h"
#include "LCS_AP_Uncertainty-Ellipse.h"
#include "LCS_AP_UnsuccessfulOutcome.h"
#include "LCS_AP_Validity-Duration.h"
#include "LCS_AP_Validity-Start-Time.h"
#include "LCS_AP_Velocity-Estimate.h"
#include "LCS_AP_Vertical-Accuracy.h"
#include "LCS_AP_Vertical-Requested.h"
#include "LCS_AP_Vertical-Speed-Direction.h"
#include "LCS_AP_Vertical-Velocity.h"
#include "LCS_AP_asn_constant.h"

#include "asn1c/util/conv.h"
#include "asn1c/util/message.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SLs carries LCS-AP over SCTP with Payload Protocol Identifier (PPID) 29. */
#define OGS_SCTP_LCS_AP_PPID 29
/* Default SLs SCTP port the MME connects to on the E-SMLC. */
#define OGS_LCS_AP_SCTP_PORT 9082

typedef struct LCS_AP_LCS_AP_PDU ogs_lcs_ap_message_t;

ogs_pkbuf_t *ogs_lcs_ap_encode(ogs_lcs_ap_message_t *message);
int ogs_lcs_ap_decode(ogs_lcs_ap_message_t *message, ogs_pkbuf_t *pkbuf);
void ogs_lcs_ap_free(ogs_lcs_ap_message_t *message);

const char *ogs_lcs_ap_version(void);

#ifdef __cplusplus
}
#endif

#endif /* OGS_LCS_AP_H */
