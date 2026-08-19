/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/queue.h>

#include <zephyr/kernel.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "driver_zephyr.h"

#define IOVEC_DEFINED
#include <assert.h>
#include <wl_api.h>
#include "timeout.h"
#include "wl80211.h"

#include "supplicant.h"

#include "bflb_wifi.h"
#include "bflb_wifi_scan.h"

LOG_MODULE_DECLARE(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

#include "bflb_wifi_ie.h"

#define PRIVACY_CACHE_SIZE 64

#define BEACON_CAP_OFFSET         34
#define BEACON_IES_OFFSET         (BEACON_CAP_OFFSET + 2)
#define IEEE80211_CAP_PRIVACY_BIT 0x10

/* Privacy cache */

struct privacy_entry {
	uint8_t bssid[ETH_ALEN];
	bool has_privacy;
	bool used;
	uint8_t rsn_ie[MAX_CACHED_RSN_IE_LEN];
	uint8_t rsn_ie_len;
	uint8_t rsnxe[MAX_CACHED_RSNXE_LEN];
	uint8_t rsnxe_len;
};

static struct privacy_entry privacy_cache[PRIVACY_CACHE_SIZE];
static void (*orig_scan_ap_ind)(struct wl80211_scan_ops *, struct wl80211_scan_ap_ind *, uint8_t *,
				uint16_t);

static void privacy_cache_clear(void)
{
	memset(privacy_cache, 0, sizeof(privacy_cache));
}

static void privacy_cache_add(const uint8_t *bssid, bool has_privacy, const uint8_t *rsn_ie,
			      uint8_t rsn_ie_len, const uint8_t *rsnxe, uint8_t rsnxe_len)
{
	int free_idx = -1;
	int i;

	for (i = 0; i < PRIVACY_CACHE_SIZE; i++) {
		if (privacy_cache[i].used && memcmp(privacy_cache[i].bssid, bssid, ETH_ALEN) == 0) {
			privacy_cache[i].has_privacy = has_privacy;
			if (rsn_ie && rsn_ie_len > 0) {
				uint8_t copy_len = MIN(rsn_ie_len, MAX_CACHED_RSN_IE_LEN);

				memcpy(privacy_cache[i].rsn_ie, rsn_ie, copy_len);
				privacy_cache[i].rsn_ie_len = copy_len;
			}
			if (rsnxe && rsnxe_len > 0) {
				uint8_t copy_len = MIN(rsnxe_len, MAX_CACHED_RSNXE_LEN);

				memcpy(privacy_cache[i].rsnxe, rsnxe, copy_len);
				privacy_cache[i].rsnxe_len = copy_len;
			}
			return;
		}
		if (!privacy_cache[i].used && free_idx < 0) {
			free_idx = i;
		}
	}
	if (free_idx >= 0) {
		memcpy(privacy_cache[free_idx].bssid, bssid, ETH_ALEN);
		privacy_cache[free_idx].has_privacy = has_privacy;
		privacy_cache[free_idx].used = true;
		if (rsn_ie && rsn_ie_len > 0) {
			uint8_t copy_len = MIN(rsn_ie_len, MAX_CACHED_RSN_IE_LEN);

			memcpy(privacy_cache[free_idx].rsn_ie, rsn_ie, copy_len);
			privacy_cache[free_idx].rsn_ie_len = copy_len;
		}
		if (rsnxe && rsnxe_len > 0) {
			uint8_t copy_len = MIN(rsnxe_len, MAX_CACHED_RSNXE_LEN);

			memcpy(privacy_cache[free_idx].rsnxe, rsnxe, copy_len);
			privacy_cache[free_idx].rsnxe_len = copy_len;
		}
	}
}

static bool privacy_cache_has_privacy(const uint8_t *bssid)
{
	int i;

	for (i = 0; i < PRIVACY_CACHE_SIZE; i++) {
		if (privacy_cache[i].used && memcmp(privacy_cache[i].bssid, bssid, ETH_ALEN) == 0) {
			return privacy_cache[i].has_privacy;
		}
	}
	return false;
}

static void wrapped_scan_ap_ind(struct wl80211_scan_ops *ops, struct wl80211_scan_ap_ind *ind,
				uint8_t *frame, uint16_t frame_len)
{
	const uint8_t *rsn_ie = NULL;
	const uint8_t *rsnxe = NULL;
	uint8_t rsn_ie_len = 0;
	uint8_t rsnxe_len = 0;

	if (frame && frame_len > BEACON_IES_OFFSET) {
		bool priv = (frame[BEACON_CAP_OFFSET] & IEEE80211_CAP_PRIVACY_BIT) != 0;
		const uint8_t *ie = frame + BEACON_IES_OFFSET;
		const uint8_t *ie_end = frame + frame_len;

		while (ie + IEEE80211_TLV_HDR_LEN <= ie_end) {
			uint8_t eid = ie[0];
			uint8_t elen = ie[1];

			if (ie + IEEE80211_TLV_HDR_LEN + elen > ie_end) {
				break;
			}
			if (eid == WLAN_EID_RSN) {
				rsn_ie = ie;
				rsn_ie_len = IEEE80211_TLV_HDR_LEN + elen;
			} else if (eid == WLAN_EID_RSNXE) {
				rsnxe = ie;
				rsnxe_len = IEEE80211_TLV_HDR_LEN + elen;
			}
			ie += IEEE80211_TLV_HDR_LEN + elen;
		}

		privacy_cache_add(ind->bssid, priv, rsn_ie, rsn_ie_len, rsnxe, rsnxe_len);
	}

	if (orig_scan_ap_ind != NULL) {
		orig_scan_ap_ind(ops, ind, frame, frame_len);
	}
}

void bflb_privacy_cache_install_hook(void)
{
	struct wl80211_scan_ops *sops;

	privacy_cache_clear();

	sops = STAILQ_FIRST(&wl80211_glb.scan_ops);

	if (sops != NULL && sops->scan_ap_ind != wrapped_scan_ap_ind) {
		orig_scan_ap_ind = sops->scan_ap_ind;
		sops->scan_ap_ind = wrapped_scan_ap_ind;
	}
}

bool bflb_privacy_cache_lookup(const uint8_t *bssid)
{
	return privacy_cache_has_privacy(bssid);
}

const uint8_t *bflb_rsn_ie_cache_lookup(const uint8_t *bssid, uint8_t *out_len)
{
	int i;

	for (i = 0; i < PRIVACY_CACHE_SIZE; i++) {
		if (privacy_cache[i].used && privacy_cache[i].rsn_ie_len > 0 &&
		    memcmp(privacy_cache[i].bssid, bssid, ETH_ALEN) == 0) {
			*out_len = privacy_cache[i].rsn_ie_len;
			return privacy_cache[i].rsn_ie;
		}
	}
	*out_len = 0;
	return NULL;
}

const uint8_t *bflb_rsnxe_cache_lookup(const uint8_t *bssid, uint8_t *out_len)
{
	int i;

	for (i = 0; i < PRIVACY_CACHE_SIZE; i++) {
		if (privacy_cache[i].used && privacy_cache[i].rsnxe_len > 0 &&
		    memcmp(privacy_cache[i].bssid, bssid, ETH_ALEN) == 0) {
			*out_len = privacy_cache[i].rsnxe_len;
			return privacy_cache[i].rsnxe;
		}
	}
	*out_len = 0;
	return NULL;
}

/* IE serialisation for scan results */

static uint32_t scan_pairwise_cipher(uint8_t flags)
{
	if (flags & WL80211_SCAN_AP_RESULT_FLAGS_HAS_CCMP) {
		return RSN_CIPHER_SUITE_CCMP;
	}
	if (flags & WL80211_SCAN_AP_RESULT_FLAGS_HAS_TKIP) {
		return RSN_CIPHER_SUITE_TKIP;
	}
	return RSN_CIPHER_SUITE_CCMP;
}

static uint32_t scan_group_cipher(uint8_t flags)
{
	if (flags & WL80211_SCAN_AP_RESULT_FLAGS_HAS_TKIP) {
		return RSN_CIPHER_SUITE_TKIP;
	}
	if (flags & WL80211_SCAN_AP_RESULT_FLAGS_HAS_CCMP) {
		return RSN_CIPHER_SUITE_CCMP;
	}
	return RSN_CIPHER_SUITE_CCMP;
}

uint8_t *bflb_append_security_ie(uint8_t *pos, struct wl80211_scan_result_item *n)
{
	uint8_t *len_pos;
	uint32_t flags = n->flags;

	if (flags & WL80211_SCAN_AP_RESULT_FLAGS_HAS_RSN) {
		uint32_t pairwise = scan_pairwise_cipher(flags);
		uint32_t group = scan_group_cipher(flags);
		uint32_t km = n->key_mgmt;
		uint16_t capab = 0;
		uint8_t *akm_cnt_pos;
		uint16_t akm_count = 0;

		bool has_sae = km & (WPA_KEY_MGMT_SAE | WPA_KEY_MGMT_FT_SAE);
		bool has_psk =
			km & (WPA_KEY_MGMT_PSK | WPA_KEY_MGMT_PSK_SHA256 | WPA_KEY_MGMT_FT_PSK);
		bool has_sha256 = km & (WPA_KEY_MGMT_PSK_SHA256 | WPA_KEY_MGMT_IEEE8021X_SHA256);

		if (has_sae && has_psk) {
			capab = WPA_CAPABILITY_MFPC;
		} else if (has_sae) {
			capab = WPA_CAPABILITY_MFPC | WPA_CAPABILITY_MFPR;
		} else if (has_sha256) {
			capab = WPA_CAPABILITY_MFPC;
		} else {
			/* no MFP capability */
		}

		*pos++ = WLAN_EID_RSN;
		len_pos = pos++;

		pos = emit_le16(pos, 1); /* version */
		pos = emit_be32(pos, group);
		pos = emit_le16(pos, 1); /* pairwise count */
		pos = emit_be32(pos, pairwise);

		akm_cnt_pos = pos;
		pos += 2;

		if (km & WPA_KEY_MGMT_SAE) {
			pos = emit_be32(pos, RSN_AUTH_KEY_MGMT_SAE);
			akm_count++;
		}
		if (km & WPA_KEY_MGMT_FT_SAE) {
			pos = emit_be32(pos, RSN_AUTH_KEY_MGMT_FT_SAE);
			akm_count++;
		}
		if (km & WPA_KEY_MGMT_PSK_SHA256) {
			pos = emit_be32(pos, RSN_AUTH_KEY_MGMT_PSK_SHA256);
			akm_count++;
		}
		if (km & WPA_KEY_MGMT_FT_PSK) {
			pos = emit_be32(pos, RSN_AUTH_KEY_MGMT_FT_PSK);
			akm_count++;
		}
		if (km & WPA_KEY_MGMT_PSK) {
			pos = emit_be32(pos, RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X);
			akm_count++;
		}
		if (akm_count == 0) {
			pos = emit_be32(pos, RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X);
			akm_count = 1;
		}
		sys_put_le16(akm_count, akm_cnt_pos);

		pos = emit_le16(pos, capab);
		*len_pos = pos - len_pos - 1;
	} else if (flags & WL80211_SCAN_AP_RESULT_FLAGS_HAS_WPA) {
		uint32_t pairwise = (flags & WL80211_SCAN_AP_RESULT_FLAGS_HAS_CCMP)
					    ? WPA_CIPHER_SUITE_CCMP
					    : WPA_CIPHER_SUITE_TKIP;

		*pos++ = WLAN_EID_VENDOR;
		len_pos = pos++;

		pos = emit_buf(pos, wpa_oui_header, sizeof(wpa_oui_header));
		pos = put_cipher_suite_list(pos, WPA_CIPHER_SUITE_TKIP, pairwise,
					    WPA_AUTH_KEY_MGMT_PSK_OVER_802_1X);
		*len_pos = pos - len_pos - 1;
	} else {
		/* open network -- no security IE */
	}
	return pos;
}

/* Scan result delivery */

static void fill_scan_result(struct wifi_scan_result *result,
			     const struct wl80211_scan_result_item *n)
{
	size_t ssid_len = n->ssid ? strlen(n->ssid) : 0;

	if (ssid_len > WIFI_SSID_MAX_LEN) {
		ssid_len = WIFI_SSID_MAX_LEN;
	}
	if (n->ssid) {
		memcpy(result->ssid, n->ssid, ssid_len);
	}
	result->ssid_length = ssid_len;

	memcpy(result->mac, n->bssid, WIFI_MAC_ADDR_LEN);
	result->mac_length = WIFI_MAC_ADDR_LEN;
	result->channel = n->channel;
	result->band = (n->band == WL80211_BAND_5G) ? WIFI_FREQ_BAND_5_GHZ : WIFI_FREQ_BAND_2_4_GHZ;
	result->rssi = n->rssi;

	if (n->flags & WL80211_SCAN_AP_RESULT_FLAGS_HAS_RSN) {
		bool has_sae = n->key_mgmt & (WPA_KEY_MGMT_SAE | WPA_KEY_MGMT_FT_SAE);
		bool has_psk = n->key_mgmt &
			       (WPA_KEY_MGMT_PSK | WPA_KEY_MGMT_PSK_SHA256 | WPA_KEY_MGMT_FT_PSK);
		bool has_sha256 =
			n->key_mgmt & (WPA_KEY_MGMT_PSK_SHA256 | WPA_KEY_MGMT_IEEE8021X_SHA256);

		if (has_sae && has_psk) {
			result->security = WIFI_SECURITY_TYPE_SAE_AUTO;
		} else if (has_sae) {
			result->security = WIFI_SECURITY_TYPE_SAE;
		} else if ((n->key_mgmt & WPA_KEY_MGMT_PSK_SHA256) &&
			   !(n->key_mgmt & WPA_KEY_MGMT_PSK)) {
			result->security = WIFI_SECURITY_TYPE_PSK_SHA256;
		} else {
			result->security = WIFI_SECURITY_TYPE_PSK;
		}

		if (has_sae && !has_psk) {
			result->mfp = WIFI_MFP_REQUIRED;
		} else if ((has_sae && has_psk) || has_sha256) {
			result->mfp = WIFI_MFP_OPTIONAL;
		} else {
			result->mfp = WIFI_MFP_DISABLE;
		}
	} else if (n->flags & WL80211_SCAN_AP_RESULT_FLAGS_HAS_WPA) {
		result->security = WIFI_SECURITY_TYPE_WPA_PSK;
	} else if (bflb_privacy_cache_lookup(n->bssid)) {
		result->security = WIFI_SECURITY_TYPE_WEP;
	} else {
		result->security = WIFI_SECURITY_TYPE_NONE;
	}
}

void bflb_wifi_deliver_scan_results(struct bflb_wifi_dev *wdev)
{
	struct wl80211_scan_result_item *n;
	scan_result_cb_t cb;

	k_mutex_lock(&wdev->lock, K_FOREVER);
	cb = wdev->scan_cb;
	k_mutex_unlock(&wdev->lock);

	if (cb == NULL) {
		return;
	}

	RB_FOREACH(n, _scan_result_tree, &wl80211_scan_result)
	{
		struct wifi_scan_result result = {0};

		fill_scan_result(&result, n);
		cb(wdev->iface, 0, &result);
	}

	cb(wdev->iface, 0, NULL);

	k_mutex_lock(&wdev->lock, K_FOREVER);
	wdev->scan_cb = NULL;
	k_mutex_unlock(&wdev->lock);
}

int bflb_wifi_scan(const struct device *dev, struct net_if *iface, struct wifi_scan_params *params,
		   scan_result_cb_t cb)
{
	struct bflb_wifi_dev *wdev = dev->data;
	struct wl80211_scan_params scan_params = {0};
	int ret;

	ARG_UNUSED(iface);
	ARG_UNUSED(params);

	k_mutex_lock(&wdev->lock, K_FOREVER);
	if (wdev->scan_cb != NULL) {
		k_mutex_unlock(&wdev->lock);
		return -EBUSY;
	}
	wdev->scan_cb = cb;
	k_mutex_unlock(&wdev->lock);

	ret = wl80211_scan(&scan_params);
	if (ret != 0) {
		LOG_ERR("wl80211_scan failed: %d", ret);
		k_mutex_lock(&wdev->lock, K_FOREVER);
		wdev->scan_cb = NULL;
		k_mutex_unlock(&wdev->lock);
		return -EIO;
	}

	bflb_privacy_cache_install_hook();

	return 0;
}
