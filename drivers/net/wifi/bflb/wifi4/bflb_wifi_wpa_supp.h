/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BFLB_WIFI_WPA_SUPP_H_
#define BFLB_WIFI_WPA_SUPP_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>

#include "bflb_wifi.h"

#define BFLB_MAX_ASSOC_IE_LEN 64

struct zep_wpa_supp_dev_ops;

struct bflb_supp_ctx {
	struct bflb_wifi_dev *wdev;
	void *supp_drv_if_ctx;
	/* Callbacks into the supplicant; mirrored as raw storage to avoid
	 * pulling hostap headers into every driver file.
	 */
	struct k_work_delayable connect_work;

	uint8_t assoc_bssid[BFLB_WIFI_MAC_ADDR_LEN];
	uint16_t assoc_freq;

	uint8_t assoc_ie[BFLB_MAX_ASSOC_IE_LEN];
	uint16_t assoc_ie_len;

	uint8_t sta_idx;
	bool open_network;
	bool assoc_reported;

	/* Pending connect parameters (consumed by connect_work). */
	uint8_t conn_ssid[BFLB_WIFI_SSID_MAX_LEN];
	uint8_t conn_ssid_len;
	uint8_t conn_bssid[BFLB_WIFI_MAC_ADDR_LEN];
	uint8_t conn_channel;
	bool conn_secured;
	bool conn_sae;
	char conn_phrase[BFLB_WIFI_PSK_MAX_LEN + 1];
};

extern struct bflb_supp_ctx g_supp_ctx;
extern const struct zep_wpa_supp_dev_ops bflb_wpa_supp_ops;

void bflb_wpa_supp_scan_done_event(void);
void bflb_wpa_supp_assoc_event(void);
void bflb_wpa_supp_connect_result(uint16_t status);
void bflb_wpa_supp_disconnected_event(uint16_t reason);
void bflb_wifi_eapol_input(const uint8_t *src, const uint8_t *buf, uint32_t len);
void bflb_wpa_supp_set_pmk(const uint8_t *pmk, size_t pmk_len, const uint8_t *pmkid);

#endif /* BFLB_WIFI_WPA_SUPP_H_ */
