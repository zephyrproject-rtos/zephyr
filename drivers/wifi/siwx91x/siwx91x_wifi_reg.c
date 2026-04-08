/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/net/wifi.h>
#include "sli_wifi_types.h"

static const char *const ww_codes[] = { "00" };
static const sli_wifi_set_region_ap_request_t ww_params = {
	.set_region_code_from_user_cmd  = SET_REGION_CODE_FROM_USER,
	.country_code                   = "00 ",
	.no_of_rules                    = 1,
	.channel_info[0].first_channel  = 1,
	.channel_info[0].no_of_channels = 11,
	.channel_info[0].max_tx_power   = 20,
};

static const char *const us_codes[] = {
	"AE", "AR", "AS", "BB", "BM", "BR", "BS", "CA", "CO", "CR", "CU", "CX", "DM", "DO",
	"EC", "FM", "GD", "GY", "GU", "HN", "HT", "JM", "KY", "LB", "LK", "MH", "MN", "MP",
	"MO", "MY", "NI", "PA", "PE", "PG", "PH", "PK", "PR", "PW", "PY", "SG", "MX", "SV",
	"TC", "TH", "TT", "US", "UY", "VE", "VI", "VN", "VU",
};
static const sli_wifi_set_region_ap_request_t us_params = {
	.set_region_code_from_user_cmd  = SET_REGION_CODE_FROM_USER,
	.country_code                   = "US ",
	.no_of_rules                    = 1,
	.channel_info[0].first_channel  = 1,
	.channel_info[0].no_of_channels = 11,
	.channel_info[0].max_tx_power   = 30,
};

static const char *const eu_codes[] = {
	"AD", "AF", "AI", "AL", "AM", "AN", "AT", "AW", "AU", "AZ", "BA", "BE", "BG", "BH", "BL",
	"BT", "BY", "CH", "CY", "CZ", "DE", "DK", "EE", "ES", "FR", "GB", "GE", "GF", "GL", "GP",
	"GR", "GT", "HK", "HR", "HU", "ID", "IE", "IL", "IN", "IR", "IS", "IT", "JO", "KH", "FI",
	"KN", "KW", "KZ", "LC", "LI", "LT", "LU", "LV", "MD", "ME", "MK", "MF", "MT", "MV", "MQ",
	"NL", "NO", "NZ", "OM", "PF", "PL", "PM", "PT", "QA", "RO", "RS", "RU", "SA", "SE", "SI",
	"SK", "SR", "SY", "TR", "TW", "UA", "UZ", "VC", "WF", "WS", "YE", "RE", "YT"
};
static const sli_wifi_set_region_ap_request_t eu_params = {
	.set_region_code_from_user_cmd  = SET_REGION_CODE_FROM_USER,
	.country_code                   = "EU ",
	.no_of_rules                    = 1,
	.channel_info[0].first_channel  = 1,
	.channel_info[0].no_of_channels = 13,
	.channel_info[0].max_tx_power   = 20,
};

static const char *const jp_codes[] = { "BD", "BN", "BO", "CL", "BZ", "JP", "NP" };
static const sli_wifi_set_region_ap_request_t jp_params = {
	.set_region_code_from_user_cmd  = SET_REGION_CODE_FROM_USER,
	.country_code                   = "JP ",
	.no_of_rules                    = 1,
	.channel_info[0].first_channel  = 1,
	.channel_info[0].no_of_channels = 14,
	.channel_info[0].max_tx_power   = 20,
};

static const char *const kr_codes[] = { "KR", "KP" };
static const sli_wifi_set_region_ap_request_t kr_params = {
	.set_region_code_from_user_cmd  = SET_REGION_CODE_FROM_USER,
	.country_code                   = "KR ",
	.no_of_rules                    = 1,
	.channel_info[0].first_channel  = 1,
	.channel_info[0].no_of_channels = 13,
	.channel_info[0].max_tx_power   = 23,
};

static const char *const sg_codes[] = { "SG" };
static const sli_wifi_set_region_ap_request_t sg_params = {
	.set_region_code_from_user_cmd  = SET_REGION_CODE_FROM_USER,
	.country_code                   = "SG ",
	.no_of_rules                    = 1,
	.channel_info[0].first_channel  = 1,
	.channel_info[0].no_of_channels = 13,
	.channel_info[0].max_tx_power   = 27,
};

static const char *const cn_codes[] = { "CN" };
static const sli_wifi_set_region_ap_request_t cn_params = {
	.set_region_code_from_user_cmd  = SET_REGION_CODE_FROM_USER,
	.country_code                   = "CN ",
	.no_of_rules                    = 1,
	.channel_info[0].first_channel  = 1,
	.channel_info[0].no_of_channels = 13,
	.channel_info[0].max_tx_power   = 20,
};


static const struct siwx91x_wifi_region {
	const char *const *z_codes;
	size_t z_codes_len;
	const sli_wifi_set_region_ap_request_t *params;
	sl_wifi_region_code_t sl_code;
} regions[] = {
	{ ww_codes, ARRAY_SIZE(ww_codes), &ww_params, SL_WIFI_REGION_WORLD_DOMAIN },
	{ us_codes, ARRAY_SIZE(us_codes), &us_params, SL_WIFI_REGION_US },
	{ eu_codes, ARRAY_SIZE(eu_codes), &eu_params, SL_WIFI_REGION_EU },
	{ jp_codes, ARRAY_SIZE(jp_codes), &jp_params, SL_WIFI_REGION_JP },
	{ kr_codes, ARRAY_SIZE(kr_codes), &kr_params, SL_WIFI_REGION_KR },
	{ sg_codes, ARRAY_SIZE(sg_codes), &sg_params, SL_WIFI_REGION_SG },
	{ cn_codes, ARRAY_SIZE(cn_codes), &cn_params, SL_WIFI_REGION_CN },
};

const struct siwx91x_wifi_region *siwx91x_map_country_code_to_region(const char *country_code)
{
	__ASSERT(country_code, "Invalid argument");

	ARRAY_FOR_EACH(regions, i) {
		for (size_t j = 0; j < regions[i].z_codes_len; j++) {
			if (!strcmp(country_code, regions[i].z_codes[j])) {
				return &regions[i];
			}
		}
	}
	return NULL;
}
