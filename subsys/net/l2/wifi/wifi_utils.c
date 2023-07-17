/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Utility functions to be used by the Wi-Fi subsytem.
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_wifi_utils, CONFIG_NET_L2_WIFI_MGMT_LOG_LEVEL);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/wifi.h>

/* Ensure 'strtok_r' is available even with -std=c99. */
char *strtok_r(char *str, const char *delim, char **saveptr);

static enum wifi_frequency_bands wifi_utils_map_band_str_to_idx(char *band_str)
{
	if (!strcmp(band_str, "2")) {
		return WIFI_FREQ_BAND_2_4_GHZ;
	} else if (!strcmp(band_str, "5")) {
		return WIFI_FREQ_BAND_5_GHZ;
	} else if (!strcmp(band_str, "6")) {
		return WIFI_FREQ_BAND_6_GHZ;
	} else {
		NET_ERR("Unknown band value: %s", band_str);
		return WIFI_FREQ_BAND_UNKNOWN;
	}
}


int wifi_utils_parse_scan_bands(char *scan_bands_str, uint8_t *band_map)
{
	char *band_str = NULL;
	char *ctx = NULL;
	enum wifi_frequency_bands band = WIFI_FREQ_BAND_UNKNOWN;

	if (!scan_bands_str) {
		return -EINVAL;
	}

	band_str = strtok_r(scan_bands_str, ",", &ctx);

	while (band_str) {
		band = wifi_utils_map_band_str_to_idx(band_str);

		if (band == WIFI_FREQ_BAND_UNKNOWN) {
			NET_ERR("Invalid band value: %s", band_str);
			return -EINVAL;
		}

		*band_map |= (1 << band);

		band_str = strtok_r(NULL, ",", &ctx);
	}

	return 0;
}
