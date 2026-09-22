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

#include "driver_zephyr.h"
#include "wl80211.h"
#include "mac_types.h"

#include "bflb_wifi.h"

#define STA_IDX_INVALID  0xFFU
#define MAX_ASSOC_IE_LEN 64

struct bflb_supp_ctx {
	struct bflb_wifi_dev *wdev;
	void *supp_drv_if_ctx;
	struct zep_wpa_supp_dev_callbk_fns supp_cb;
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_AP
	struct zep_hostapd_dev_callbk_fns hapd_cb;
	void *hapd_drv_if_ctx;
#endif

	struct k_work assoc_work;
	uint8_t assoc_bssid[ETH_ALEN];

	struct k_work_delayable connect_work;
	struct wl80211_connect_params connect_params;
	uint8_t connect_retries;
	bool connect_cancelled;

	uint16_t assoc_freq;

	uint8_t assoc_ie[MAX_ASSOC_IE_LEN];
	uint16_t assoc_ie_len;

	uint8_t sta_idx;

	uint8_t rsnxe[8];
	uint8_t rsnxe_len;

	uint8_t mdie[7];
	uint8_t mdie_len;

	bool open_network;

	uint8_t ext_auth_bssid[ETH_ALEN];
	uint8_t ext_auth_ssid[MAC_SSID_LEN];
};

extern struct bflb_supp_ctx g_supp_ctx;

struct zep_wpa_supp_dev_ops;

extern const struct zep_wpa_supp_dev_ops bflb_wpa_supp_ops;

uint8_t bflb_aid_list_find_sta(const uint8_t *mac);

void bflb_wpa_supp_register_wpa_cbs(void);
void bflb_wpa_supp_scan_done_event(void);
void bflb_wpa_supp_sta_connected_event(void);
void bflb_wifi_eapol_input(uint8_t vif_type, uint8_t *payload, size_t len);

#endif /* BFLB_WIFI_WPA_SUPP_H_ */
