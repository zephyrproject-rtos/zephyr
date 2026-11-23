/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* wl80211 supplicant bridge: blob-required symbols and timer helpers. */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/queue.h>

#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

#include <assert.h>
#include <wl_api.h>
#include "timeout.h"
#include "macsw_config.h"
#include "macsw_plat.h"
#include "mac_types.h"
#include "mac_frame.h"
#include "ieee80211.h"
#include "phy.h"
#include "macsw.h"
#include "wl80211.h"
#include "wl80211_mac.h"
#include "supplicant.h"

extern void bflb_wifi_eapol_input(uint8_t vif_type, uint8_t *payload, size_t len);

const struct wpa_funcs *wpa_cbs;
static uint8_t *g_assoc_ie;
static uint16_t g_assoc_ie_len;
static void *g_hostap_private;

int wl80211_supplicant_init(void)
{
	if (wpa_cbs && wpa_cbs->wpa_sta_init) {
		wpa_cbs->wpa_sta_init();
	}
	return 0;
}

int wl80211_supplicant_register_wpa_cb_internal(const struct wpa_funcs *cb)
{
	wpa_cbs = cb;
	return 0;
}

int wl80211_supplicant_unregister_wpa_cb_internal(void)
{
	wpa_cbs = NULL;
	return 0;
}

int wl80211_supplicant_eapol_input(uint8_t vif_type, uint8_t *payload, size_t len)
{
	LOG_DBG("eapol_input: vif=%u len=%zu", vif_type, len);
	bflb_wifi_eapol_input(vif_type, payload, len);
	return 0;
}

int wl80211_supplicant_set_sta_key_internal(uint8_t vif_idx, uint8_t sta_idx, bflb_wpa_alg_t alg,
					    int key_idx, int set_tx, uint8_t *seq, size_t seq_len,
					    uint8_t *key, size_t key_len, bool pairwise)
{
	LOG_DBG("set_sta_key: vif=%u sta=%u alg=%d key_idx=%d pairwise=%d", vif_idx, sta_idx, alg,
		key_idx, pairwise);
	return 0;
}

bool wl80211_supplicant_auth_done_internal(uint8_t sta_idx, uint16_t reason_code)
{
	LOG_DBG("auth_done: sta=%u reason=%u", sta_idx, reason_code);
	return true;
}

int wl80211_supplicant_sta_update_ap_info_internal(void)
{
	LOG_DBG("sta_update_ap_info called");
	return 1;
}

uint8_t wl80211_supplicant_sta_set_reset_param_internal(uint8_t reset_flag)
{
	LOG_DBG("sta_set_reset_param: flag=%u", reset_flag);
	return 0;
}

bool wl80211_supplicant_sta_is_ap_notify_completed_rsne_internal(void)
{
	LOG_DBG("is_ap_notify_completed_rsne called");
	return true;
}

bool wl80211_supplicant_sta_is_running_internal(void)
{
	LOG_DBG("is_running called");
	return true;
}

int wl80211_supplicant_set_assoc_ie(uint8_t *ie, uint16_t ie_len)
{
	LOG_DBG("set_assoc_ie: ie=%p len=%u", ie, ie_len);
	g_assoc_ie = ie;
	g_assoc_ie_len = ie_len;
	return 0;
}

int wl80211_supplicant_get_assoc_ie(uint8_t **ie, uint16_t *ie_len)
{
	*ie = g_assoc_ie;
	*ie_len = g_assoc_ie_len;
	LOG_DBG("get_assoc_ie: ie=%p len=%u", g_assoc_ie, g_assoc_ie_len);
	return 0;
}

void *wl80211_supplicant_get_hostap_private_internal(void)
{
	LOG_DBG("get_hostap_private: %p", g_hostap_private);
	return g_hostap_private;
}

void wl80211_supplicant_set_hostap_private(void *priv)
{
	g_hostap_private = priv;
}
