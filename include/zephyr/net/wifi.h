/*
 * Copyright (c) 2018 Texas Instruments, Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief General WiFi Definitions
 */

#ifndef ZEPHYR_INCLUDE_NET_WIFI_H_
#define ZEPHYR_INCLUDE_NET_WIFI_H_

enum wifi_security_type {
	WIFI_SECURITY_TYPE_NONE = 0,
	WIFI_SECURITY_TYPE_PSK,
	WIFI_SECURITY_TYPE_PSK_SHA256,
	WIFI_SECURITY_TYPE_SAE,

	__WIFI_SECURITY_TYPE_AFTER_LAST,
	WIFI_SECURITY_TYPE_MAX = __WIFI_SECURITY_TYPE_AFTER_LAST - 1
};

/* Management frame protection (IEEE 802.11w) options */
enum wifi_mfp_options {
	WIFI_MFP_DISABLE = 0,
	WIFI_MFP_OPTIONAL,
	WIFI_MFP_REQUIRED
};

enum wifi_frequency_bands {
	WIFI_FREQ_BAND_2_4_GHZ = 0,
	WIFI_FREQ_BAND_5_GHZ,
	WIFI_FREQ_BAND_6_GHZ
};

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PSK_MAX_LEN 64
#define WIFI_MAC_ADDR_LEN 6

#define WIFI_CHANNEL_MAX 233
#define WIFI_CHANNEL_ANY 255

#endif /* ZEPHYR_INCLUDE_NET_WIFI_H_ */
