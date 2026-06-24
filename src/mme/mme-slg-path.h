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

#ifndef MME_SLG_PATH_H
#define MME_SLG_PATH_H

#ifdef __cplusplus
extern "C" {
#endif

/* SLg (TS 29.172) Diameter front door: the GMLC sends Provide-Location-Request
 * to the MME; the MME answers Provide-Location-Answer with the UE location. */
int mme_slg_init(void);
void mme_slg_final(void);

#ifdef __cplusplus
}
#endif

#endif /* MME_SLG_PATH_H */
