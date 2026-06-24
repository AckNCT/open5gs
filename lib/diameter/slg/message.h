/* 3GPP TS 29.172 SLg
 * Copyright (C) 2019-2025 by Sukchan Lee <acetcom@gmail.com>
 *
 * This file is part of Open5GS.
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

#if !defined(OGS_DIAMETER_INSIDE) && !defined(OGS_DIAMETER_COMPILATION)
#error "This header cannot be included directly."
#endif

#ifndef OGS_DIAM_SLG_MESSAGE_H
#define OGS_DIAM_SLG_MESSAGE_H

#include "ogs-crypt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OGS_DIAM_SLG_APPLICATION_ID                     16777255

#define OGS_DIAM_SLG_CMD_CODE_PROVIDE_LOCATION          8388620

/* SLg-Location-Type - 3GPP TS 29.172 #7.4.2 */
#define OGS_DIAM_SLG_LOCATION_TYPE_CURRENT_LOCATION                  0
#define OGS_DIAM_SLG_LOCATION_TYPE_CURRENT_OR_LAST_KNOWN_LOCATION    1
#define OGS_DIAM_SLG_LOCATION_TYPE_INITIAL_LOCATION                  2
#define OGS_DIAM_SLG_LOCATION_TYPE_ACTIVATE_DEFERRED_LOCATION        3
#define OGS_DIAM_SLG_LOCATION_TYPE_CANCEL_DEFERRED_LOCATION          4
#define OGS_DIAM_SLG_LOCATION_TYPE_NOTIFICATION_VERIFICATION_ONLY    5

extern struct dict_object *ogs_diam_slg_application;

extern struct dict_object *ogs_diam_slg_cmd_plr;
extern struct dict_object *ogs_diam_slg_cmd_pla;

extern struct dict_object *ogs_diam_slg_location_type;
extern struct dict_object *ogs_diam_slg_lcs_priority;
extern struct dict_object *ogs_diam_slg_lcs_reference_number;
extern struct dict_object *ogs_diam_slg_location_estimate;
extern struct dict_object *ogs_diam_slg_accuracy_fulfilment_indicator;
extern struct dict_object *ogs_diam_slg_age_of_location_estimate;

int ogs_diam_slg_init(void);

#ifdef __cplusplus
}
#endif

#endif /* OGS_DIAM_SLG_MESSAGE_H */
