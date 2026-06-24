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

#ifndef LCS_AP_BUILD_H
#define LCS_AP_BUILD_H

#include "ogs-lcs-ap.h"

#ifdef __cplusplus
extern "C" {
#endif

ogs_pkbuf_t *lcs_ap_build_location_request(
        const char *imsi_bcd, uint32_t correlation_id);

#ifdef __cplusplus
}
#endif

#endif /* LCS_AP_BUILD_H */
