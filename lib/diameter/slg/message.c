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

#include "ogs-diameter-slg.h"

#define CHECK_dict_search( _type, _criteria, _what, _result )    \
    CHECK_FCT(  fd_dict_search( fd_g_config->cnf_dict, (_type), (_criteria), (_what), (_result), ENOENT) );

struct dict_object *ogs_diam_slg_application = NULL;

struct dict_object *ogs_diam_slg_cmd_plr = NULL;
struct dict_object *ogs_diam_slg_cmd_pla = NULL;

struct dict_object *ogs_diam_slg_location_type = NULL;
struct dict_object *ogs_diam_slg_lcs_priority = NULL;
struct dict_object *ogs_diam_slg_lcs_reference_number = NULL;
struct dict_object *ogs_diam_slg_location_estimate = NULL;
struct dict_object *ogs_diam_slg_accuracy_fulfilment_indicator = NULL;
struct dict_object *ogs_diam_slg_age_of_location_estimate = NULL;

extern int ogs_dict_slg_entry(char *conffile);

int ogs_diam_slg_init(void)
{
    application_id_t id = OGS_DIAM_SLG_APPLICATION_ID;

    ogs_assert(ogs_dict_slg_entry(NULL) == 0);

    CHECK_dict_search(DICT_APPLICATION, APPLICATION_BY_ID, (void *)&id, &ogs_diam_slg_application);

    CHECK_dict_search(DICT_COMMAND, CMD_BY_NAME, "Provide-Location-Request", &ogs_diam_slg_cmd_plr);
    CHECK_dict_search(DICT_COMMAND, CMD_BY_NAME, "Provide-Location-Answer", &ogs_diam_slg_cmd_pla);

    /*
     * These AVPs are provided by the freeDiameter dict_dcca_3gpp extension. Look
     * them up best-effort (retval 0): a missing/renamed AVP leaves the global
     * NULL and the handler simply omits it, rather than aborting MME startup.
     */
    (void)fd_dict_search(fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME_ALL_VENDORS,
            "Location-Estimate", &ogs_diam_slg_location_estimate, 0);

    return 0;
}
