/*********************************************************************************************************
 * Software License Agreement (BSD License)                                                               *
 * Author: Sukchan Lee <acetcom@gmail.com>>                                                                *
 *                                                                                                        *
 * Copyright (c) 2017, Open5gs Group
 * All rights reserved.                                                                                   *
 *                                                                                                        *
 * Written under contract by nfotex IT GmbH, http://nfotex.com/                                           *
 *                                                                                                        *
 * Redistribution and use of this software in source and binary forms, with or without modification, are  *
 * permitted provided that the following conditions are met:                                              *
 *                                                                                                        *
 * * Redistributions of source code must retain the above                                                 *
 *   copyright notice, this list of conditions and the                                                    *
 *   following disclaimer.                                                                                *
 *                                                                                                        *
 * * Redistributions in binary form must reproduce the above                                              *
 *   copyright notice, this list of conditions and the                                                    *
 *   following disclaimer in the documentation and/or other                                               *
 *   materials provided with the distribution.                                                            *
 *                                                                                                        *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED *
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A *
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR *
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT     *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS    *
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR *
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF   *
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                                             *
 *********************************************************************************************************/

#include <freeDiameter/extension.h>

/*
 * SLg (3GPP TS 29.172) dictionary.
 *
 * The SLg/LCS AVPs (SLg-Location-Type, LCS-Priority, LCS-Reference-Number,
 * Location-Estimate, ...) are already registered by the freeDiameter
 * dict_dcca_3gpp extension that mme.conf loads, so we must NOT redefine them
 * here (fd_dict_new would fail with "File exists" and crash MME startup).
 * This dictionary only adds what dict_dcca_3gpp does not: the SLg application
 * and the Provide-Location-Request/Answer commands. The command ABNF lists the
 * base Diameter AVPs only; the GMLC's 3GPP AVPs are still recognised (defined
 * by dict_dcca_3gpp) and read by the handler via dict search.
 */

#define CHECK_dict_new( _type, _data, _parent, _ref )    \
    CHECK_FCT(  fd_dict_new( fd_g_config->cnf_dict, (_type), (_data), (_parent), (_ref))  );

#define CHECK_dict_search( _type, _criteria, _what, _result )    \
    CHECK_FCT(  fd_dict_search( fd_g_config->cnf_dict, (_type), (_criteria), (_what), (_result), ENOENT) );

struct local_rules_definition {
    struct dict_avp_request avp_vendor_plus_name;
    enum rule_position    position;
    int             min;
    int            max;
};

#define RULE_ORDER( _position ) ((((_position) == RULE_FIXED_HEAD) || ((_position) == RULE_FIXED_TAIL)) ? 1 : 0 )

/* Attention! This version of the macro uses AVP_BY_NAME_AND_VENDOR, in contrast to most other copies! */
#define PARSE_loc_rules( _rulearray, _parent) {                                \
    int __ar;                                            \
    for (__ar=0; __ar < sizeof(_rulearray) / sizeof((_rulearray)[0]); __ar++) {            \
        struct dict_rule_data __data = { NULL,                             \
            (_rulearray)[__ar].position,                            \
            0,                                         \
            (_rulearray)[__ar].min,                                \
            (_rulearray)[__ar].max};                            \
        __data.rule_order = RULE_ORDER(__data.rule_position);                    \
        CHECK_FCT(  fd_dict_search(                                 \
            fd_g_config->cnf_dict,                                \
            DICT_AVP,                                     \
            AVP_BY_NAME_AND_VENDOR,                             \
            &(_rulearray)[__ar].avp_vendor_plus_name,                    \
            &__data.rule_avp, 0 ) );                            \
        if ( !__data.rule_avp ) {                                \
            TRACE_DEBUG(INFO, "AVP Not found: '%s'", (_rulearray)[__ar].avp_vendor_plus_name.avp_name);        \
            return ENOENT;                                    \
        }                                            \
        CHECK_FCT_DO( fd_dict_new( fd_g_config->cnf_dict, DICT_RULE, &__data, _parent, NULL),    \
            {                                            \
                TRACE_DEBUG(INFO, "Error on rule with AVP '%s'",                  \
                        (_rulearray)[__ar].avp_vendor_plus_name.avp_name);        \
                return EINVAL;                                      \
            } );                                              \
    }                                                      \
}

int ogs_dict_slg_entry(char *conffile)
{
    struct dict_object *slg = NULL;
    application_id_t app_id = 16777255;
    TRACE_ENTRY("%p", conffile);

    /* Applications section: create the SLg application if not already present. */
    {
        (void)fd_dict_search(fd_g_config->cnf_dict, DICT_APPLICATION,
                APPLICATION_BY_ID, (void *)&app_id, &slg, 0);
        if (!slg) {
            struct dict_object * vendor;
            CHECK_FCT(fd_dict_search(fd_g_config->cnf_dict, DICT_VENDOR, VENDOR_BY_NAME, "3GPP", &vendor, ENOENT));
            struct dict_application_data app_data = { 16777255, "SLg" };
            CHECK_FCT(fd_dict_new(fd_g_config->cnf_dict, DICT_APPLICATION, &app_data, vendor, &slg));
        }
    }

    /* Command section */
    {
        /* SLg-Provide-Location-Request - 3GPP TS 29.172 #7.3.1 */
        {
            struct dict_object * cmd = NULL;
            (void)fd_dict_search(fd_g_config->cnf_dict, DICT_COMMAND,
                    CMD_BY_NAME, "Provide-Location-Request", &cmd, 0);
            if (!cmd) {
                struct dict_cmd_data data = {
                    8388620,                                                /* Code */
                    "Provide-Location-Request",                             /* Name */
                    CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE | CMD_FLAG_ERROR, /* Fixed flags */
                    CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE                   /* Fixed flag values */
                };
                struct local_rules_definition rules[] =
                {
                    {  { .avp_name = "Session-Id" }, RULE_FIXED_HEAD, -1, 1 },
                    {  { .avp_name = "Vendor-Specific-Application-Id" }, RULE_OPTIONAL, -1, 1 },
                    {  { .avp_name = "Auth-Session-State" }, RULE_REQUIRED, -1, 1 },
                    {  { .avp_name = "Origin-Host" }, RULE_REQUIRED, -1, 1 },
                    {  { .avp_name = "Origin-Realm" }, RULE_REQUIRED, -1, 1 },
                    {  { .avp_name = "Destination-Host" }, RULE_OPTIONAL, -1, 1 },
                    {  { .avp_name = "Destination-Realm" }, RULE_REQUIRED, -1, 1 },
                    {  { .avp_name = "User-Name" }, RULE_OPTIONAL, -1, 1 },
                };
                CHECK_dict_new(DICT_COMMAND, &data, slg, &cmd);
                PARSE_loc_rules(rules, cmd);
            }
        }

        /* SLg-Provide-Location-Answer - 3GPP TS 29.172 #7.3.2 */
        {
            struct dict_object * cmd = NULL;
            (void)fd_dict_search(fd_g_config->cnf_dict, DICT_COMMAND,
                    CMD_BY_NAME, "Provide-Location-Answer", &cmd, 0);
            if (!cmd) {
                struct dict_cmd_data data = {
                    8388620,                                                /* Code */
                    "Provide-Location-Answer",                              /* Name */
                    CMD_FLAG_REQUEST | CMD_FLAG_PROXIABLE | CMD_FLAG_ERROR, /* Fixed flags */
                    CMD_FLAG_PROXIABLE                                      /* Fixed flag values */
                };
                struct local_rules_definition rules[] =
                {
                    {  { .avp_name = "Session-Id" }, RULE_FIXED_HEAD, -1, 1 },
                    {  { .avp_name = "Vendor-Specific-Application-Id" }, RULE_OPTIONAL, -1, 1 },
                    {  { .avp_name = "Result-Code" }, RULE_OPTIONAL, -1, 1 },
                    {  { .avp_name = "Auth-Session-State" }, RULE_REQUIRED, -1, 1 },
                    {  { .avp_name = "Origin-Host" }, RULE_REQUIRED, -1, 1 },
                    {  { .avp_name = "Origin-Realm" }, RULE_REQUIRED, -1, 1 },
                };
                CHECK_dict_new(DICT_COMMAND, &data, slg, &cmd);
                PARSE_loc_rules(rules, cmd);
            }
        }
    }

    LOG_D( "Extension 'Dictionary definitions for 3GPP SLg' initialized");
    return 0;
}

#if 0 /* modified by acetcom */
EXTENSION_ENTRY("dict_slg", ogs_dict_slg_entry, "dict_dcca_3gpp");
#endif
