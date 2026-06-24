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
 * LCS-AP — the SLs interface between the MME and the E-SMLC (3GPP TS 29.171).
 *
 * Phase A increment 1: library scaffolding + build plumbing only. The APER
 * ASN.1 codec (asn1c-generated, mirroring lib/asn1c/s1ap) and the message
 * build/parse glue are added in later increments.
 */

#ifndef OGS_LCS_AP_H
#define OGS_LCS_AP_H

#include "ogs-core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SLs carries LCS-AP over SCTP with Payload Protocol Identifier (PPID) 29. */
#define OGS_SCTP_LCS_AP_PPID 29

/*
 * TS 29.171 elementary-procedure codes. Preliminary values — these are
 * superseded by the generated ASN.1 module (LCS-AP-Constants) in increment 2.
 */
typedef enum {
    OGS_LCS_AP_PROCEDURE_LOCATION_SERVICE_REQUEST = 0,
    OGS_LCS_AP_PROCEDURE_CONNECTION_ORIENTED_INFORMATION = 1,
    OGS_LCS_AP_PROCEDURE_CONNECTIONLESS_INFORMATION = 2,
    OGS_LCS_AP_PROCEDURE_RESET = 3,
} ogs_lcs_ap_procedure_code_e;

/* Returns a static "lcs-ap <version>" identification string. */
const char *ogs_lcs_ap_version(void);

#ifdef __cplusplus
}
#endif

#endif /* OGS_LCS_AP_H */
