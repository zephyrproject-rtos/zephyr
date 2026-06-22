/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WIFI_PROV_H_
#define WIFI_PROV_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WIFI_PROV_SSID_MAX_LEN 32
#define WIFI_PROV_PSK_MAX_LEN  64

/* Sentinel value for an unset security type: falls back to auto-detect
 * (open if PSK empty, WPA2-PSK otherwise).
 */
#define WIFI_PROV_SECURITY_AUTO 0xFF

struct wifi_prov_creds {
	char ssid[WIFI_PROV_SSID_MAX_LEN + 1];
	char psk[WIFI_PROV_PSK_MAX_LEN + 1];
	uint8_t security;
};

typedef void (*wifi_prov_result_cb)(bool connected);

int wifi_prov_init(wifi_prov_result_cb cb);

bool wifi_prov_creds_valid(const struct wifi_prov_creds *creds);
bool wifi_prov_has_stored_creds(void);

int wifi_prov_creds_save(const struct wifi_prov_creds *creds);
int wifi_prov_creds_erase(void);

int wifi_prov_connect_stored(void);

#endif
