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

/* SLs interface (LCS-AP, TS 29.171): MME -> E-SMLC over SCTP/PPID 29. */

#ifndef LCS_AP_PATH_H
#define LCS_AP_PATH_H

#include "mme-context.h"
#include "mme-event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define lcs_ap_event_push mme_sctp_event_push

int lcs_ap_open(void);
void lcs_ap_close(void);

ogs_sock_t *lcs_ap_client(mme_esmlc_t *esmlc);

int lcs_ap_send(ogs_sock_t *sock, ogs_pkbuf_t *pkbuf, uint16_t stream_no);
int lcs_ap_send_to_esmlc(
        mme_esmlc_t *esmlc, ogs_pkbuf_t *pkbuf, uint16_t stream_no);

int lcs_ap_send_location_request(
        mme_esmlc_t *esmlc, const char *imsi_bcd, uint32_t correlation_id);

#ifdef __cplusplus
}
#endif

#endif /* LCS_AP_PATH_H */
