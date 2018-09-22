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
};

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PSK_MAX_LEN 64

#define WIFI_CHANNEL_MAX 14
#define WIFI_CHANNEL_ANY 255

#endif /* ZEPHYR_INCLUDE_NET_WIFI_H_ */
