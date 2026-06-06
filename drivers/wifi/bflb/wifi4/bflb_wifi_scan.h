/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BFLB_WIFI_SCAN_H_
#define BFLB_WIFI_SCAN_H_

#include <stdint.h>
#include <zephyr/net/wifi_mgmt.h>

#include "bflb_wifi.h"

#define BFLB_SCAN_AP_MAX     ((uint8_t)CONFIG_BFLB_WIFI_SCAN_AP_MAX)
#define BFLB_SCAN_RSN_IE_MAX 50U
#define BFLB_SCAN_WPA_IE_MAX 40U

struct bflb_scan_ap {
	uint8_t bssid[BFLB_WIFI_MAC_ADDR_LEN];
	uint8_t ssid[BFLB_WIFI_SSID_MAX_LEN];
	uint8_t ssid_len;
	uint16_t center_freq;
	int8_t rssi;
	uint8_t channel;
	uint8_t security; /* wifi_security_type */
	uint16_t caps;    /* 802.11 capability field */
	/* Raw security IEs cached verbatim for the supplicant -- hostap
	 * byte-compares the RSN IE during 4-way handshake msg 3 validation.
	 */
	uint8_t rsn_ie[BFLB_SCAN_RSN_IE_MAX];
	uint8_t rsn_ie_len;
	uint8_t wpa_ie[BFLB_SCAN_WPA_IE_MAX];
	uint8_t wpa_ie_len;
};

int bflb_wifi_scan_start(struct bflb_wifi_dev *d);
void bflb_wifi_scan_handle_result(struct bflb_wifi_dev *d, const void *payload);
void bflb_wifi_deliver_scan_results(struct bflb_wifi_dev *d);

uint8_t bflb_wifi_scan_count(void);
const struct bflb_scan_ap *bflb_wifi_scan_entry(uint8_t idx);
const struct bflb_scan_ap *bflb_wifi_scan_find_ssid(const uint8_t *ssid, uint8_t ssid_len);

int bflb_wifi_scan(const struct device *dev, struct net_if *iface, struct wifi_scan_params *params,
		   scan_result_cb_t cb);

#endif /* BFLB_WIFI_SCAN_H_ */
