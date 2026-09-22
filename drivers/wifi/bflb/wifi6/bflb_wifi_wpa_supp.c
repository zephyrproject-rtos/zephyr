/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <mbedtls/platform_util.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/queue.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "driver_zephyr.h"
#include "config.h"

#define IOVEC_DEFINED
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
#include "wl80211_platform.h"

#include "supplicant.h"

#include "bflb_wifi.h"
#include "bflb_wifi_scan.h"
#include "bflb_wifi_ap.h"
#include "bflb_wifi_wpa_supp.h"

LOG_MODULE_DECLARE(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

#include "bflb_wifi_ie.h"

#define WLAN_EID_SSID 0

/* 802.11 FC byte masks */
#define IEEE80211_FC_SUBTYPE_MASK 0xfc
#define IEEE80211_STYPE_AUTH      0xb0

/* IEEE 802.11i TKIP key layout: 16B temporal key + 8B TX MIC + 8B RX MIC */
#define TKIP_KEY_LEN     32U
#define TKIP_MIC_KEY_OFF 16U
#define TKIP_MIC_KEY_LEN 8U

#define ETH_HDR_LEN 14

/* WEP key lengths (RFC 2661): 5 bytes = WEP-40, 13 bytes = WEP-104 */
#define WEP104_KEY_LEN 13U

/* Assumed noise floor for SNR calculations (no blob API to query it) */
#define BFLB_NOISE_FLOOR_DBM (-90)

/* IEEE 802.11 management frame header: FC(2) + Duration(2) + Addr1-3(18) + SeqCtrl(2) */
#define IEEE80211_MGMT_HDR_LEN 24U

/*
 * IEEE 802.11 Authentication frame body offsets (relative to frame start):
 *   [24] Auth Algorithm Number (2 octets)
 *   [26] Auth Transaction Sequence Number (2 octets)
 *   [28] Status Code (2 octets)
 *   [30] Start of algorithm-dependent elements (e.g. SAE Commit/Confirm)
 * Minimum frame length to contain all fixed auth fields = 30 octets.
 * See IEEE 802.11-2020 Section 9.3.3.12.
 */
#define IEEE80211_AUTH_ALG_OFF    (IEEE80211_MGMT_HDR_LEN + 0)
#define IEEE80211_AUTH_TXN_OFF    (IEEE80211_MGMT_HDR_LEN + 2)
#define IEEE80211_AUTH_STATUS_OFF (IEEE80211_MGMT_HDR_LEN + 4)
#define IEEE80211_AUTH_MIN_LEN    (IEEE80211_MGMT_HDR_LEN + 6)

/* SAE auth transaction numbers (IEEE 802.11-2020 Section 12.4) */
#define SAE_AUTH_COMMIT  1
#define SAE_AUTH_CONFIRM 2

/* HT Capabilities Info: HT20 + Short GI for 20MHz (IEEE 802.11n Table 9-152) */
#define BFLB_HT_CAP_HT20_SGI20 0x012CU

#define BFLB_24GHZ_BASE_FREQ       2407U
#define BFLB_24GHZ_CH14_FREQ       2484U
#define BFLB_24GHZ_CH_SPACING      5U
#define BFLB_DEFAULT_MAX_DBM       20
#define BFLB_DEFAULT_BEACON_TU     100
#define BFLB_DEFAULT_AP_CHANNEL    6
#define BFLB_CONNECT_RETRY_MAX     100
#define BFLB_EAPOL_TX_TIMEOUT_MS   1000
#define BFLB_CONNECT_WORK_DELAY_MS 100

#define BFLB_SAE_COMPLETE_DELAY_MS 50

/* Extern declarations */

int mac_cipher_suite_value(uint32_t cipher_suite);
extern const struct wpa_funcs *wpa_cbs;
extern int wl80211_supplicant_register_wpa_cb_internal(const struct wpa_funcs *cb);
extern int wl80211_supplicant_set_assoc_ie(uint8_t *ie, uint16_t ie_len);
extern void wpa_supplicant_event_wrapper(void *ctx, enum wpa_event_type event,
					 union wpa_event_data *data);
extern void wl80211_supplicant_set_hostap_private(void *priv);

/*
 * Bypass hostap events.c:6598 which gates sme_external_auth_mgmt_rx on
 * !(WPA_DRIVER_FLAGS_SME). We need SME for WPA2 but also need the
 * external-auth SAE path for the full-MAC blob.
 */
struct wpa_supplicant;
extern void sme_external_auth_mgmt_rx(struct wpa_supplicant *wpa_s, const uint8_t *auth_frame,
				      size_t len);

/* Forward declarations */

static void bflb_send_external_auth_rsp(uint16_t status);
static void sae_complete_work_handler(struct k_work *work);
void bflb_wifi_eapol_input(uint8_t vif_type, uint8_t *payload, size_t len);

/* Static variables */

static K_WORK_DELAYABLE_DEFINE(sae_complete_work, sae_complete_work_handler);
struct bflb_supp_ctx g_supp_ctx;
uint8_t bflb_aid_list_find_sta(const uint8_t *mac)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(wl80211_glb.aid_list); i++) {
		if (wl80211_glb.aid_list[i].used &&
		    memcmp(wl80211_glb.aid_list[i].mac, mac, ETH_ALEN) == 0) {
			return wl80211_glb.aid_list[i].sta_idx;
		}
	}
	return STA_IDX_INVALID;
}

static uint32_t cipher_to_rsn_oui(wifi_cipher_type_t c)
{
	switch (c) {
	case WIFI_CIPHER_TYPE_CCMP:
		return RSN_CIPHER_SUITE_CCMP;
	case WIFI_CIPHER_TYPE_TKIP:
		return RSN_CIPHER_SUITE_TKIP;
	case WIFI_CIPHER_TYPE_WEP104:
		return RSN_CIPHER_SUITE_WEP104;
	case WIFI_CIPHER_TYPE_WEP40:
		return RSN_CIPHER_SUITE_WEP40;
	case WIFI_CIPHER_TYPE_AES_CMAC128:
		return RSN_CIPHER_SUITE_AES_128_CMAC;
	default:
		return RSN_CIPHER_SUITE_CCMP;
	}
}

static uint32_t keymgmt_to_rsn_oui(uint16_t km)
{
	if (km & WPA_KEY_MGMT_SAE) {
		return RSN_AUTH_KEY_MGMT_SAE;
	}
	if (km & WPA_KEY_MGMT_FT_SAE) {
		return RSN_AUTH_KEY_MGMT_FT_SAE;
	}
	if (km & WPA_KEY_MGMT_PSK_SHA256) {
		return RSN_AUTH_KEY_MGMT_PSK_SHA256;
	}
	if (km & WPA_KEY_MGMT_IEEE8021X_SHA256) {
		return RSN_AUTH_KEY_MGMT_802_1X_SHA256;
	}
	if (km & WPA_KEY_MGMT_FT_PSK) {
		return RSN_AUTH_KEY_MGMT_FT_PSK;
	}
	if (km & WPA_KEY_MGMT_FT_IEEE8021X) {
		return RSN_AUTH_KEY_MGMT_FT_802_1X;
	}
	if (km & WPA_KEY_MGMT_PSK) {
		return RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X;
	}
	if (km & WPA_KEY_MGMT_IEEE8021X) {
		return RSN_AUTH_KEY_MGMT_UNSPEC_802_1X;
	}
	return RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X;
}

static uint32_t keymgmt_to_wpa_oui(uint16_t km)
{
	if (km & WPA_KEY_MGMT_PSK) {
		return WPA_AUTH_KEY_MGMT_PSK_OVER_802_1X;
	}
	if (km & WPA_KEY_MGMT_IEEE8021X) {
		return WPA_AUTH_KEY_MGMT_UNSPEC_802_1X;
	}
	return WPA_AUTH_KEY_MGMT_PSK_OVER_802_1X;
}

static wifi_cipher_type_t oui_to_cipher(const uint8_t *oui)
{
	uint32_t s = sys_get_be32(oui);

	switch (s) {
	case RSN_CIPHER_SUITE_TKIP:
	case WPA_CIPHER_SUITE_TKIP:
		return WIFI_CIPHER_TYPE_TKIP;
	case RSN_CIPHER_SUITE_CCMP:
	case WPA_CIPHER_SUITE_CCMP:
		return WIFI_CIPHER_TYPE_CCMP;
	case RSN_CIPHER_SUITE_AES_128_CMAC:
		return WIFI_CIPHER_TYPE_AES_CMAC128;
	default:
		return WIFI_CIPHER_TYPE_UNKNOWN;
	}
}

static int oui_to_akm(const uint8_t *oui)
{
	uint32_t s = sys_get_be32(oui);

	switch (s) {
	case RSN_AUTH_KEY_MGMT_UNSPEC_802_1X:
	case WPA_AUTH_KEY_MGMT_UNSPEC_802_1X:
		return WPA_KEY_MGMT_IEEE8021X;
	case RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X:
	case WPA_AUTH_KEY_MGMT_PSK_OVER_802_1X:
		return WPA_KEY_MGMT_PSK;
	case RSN_AUTH_KEY_MGMT_PSK_SHA256:
		return WPA_KEY_MGMT_PSK_SHA256;
	case RSN_AUTH_KEY_MGMT_SAE:
		return WPA_KEY_MGMT_SAE;
	case RSN_AUTH_KEY_MGMT_FT_SAE:
		return WPA_KEY_MGMT_FT_SAE;
	case RSN_AUTH_KEY_MGMT_FT_PSK:
		return WPA_KEY_MGMT_FT_PSK;
	case RSN_AUTH_KEY_MGMT_FT_802_1X:
		return WPA_KEY_MGMT_FT_IEEE8021X;
	default:
		return 0;
	}
}

/* IE serialisation */

static int gen_assoc_ie(wifi_connect_parm_t *parm, uint8_t *buf)
{
	uint8_t *pos = buf;
	uint8_t *len_pos;
	uint16_t capab;

	if (parm->proto == SEC_PROTO_WPA2 || parm->proto == SEC_PROTO_WPA3) {
		*pos++ = WLAN_EID_RSN;
		len_pos = pos++;

		pos = put_cipher_suite_list(pos, cipher_to_rsn_oui(parm->group_cipher),
					    cipher_to_rsn_oui(parm->pairwise_cipher),
					    keymgmt_to_rsn_oui(parm->key_mgmt));

		capab = parm->pmf_required ? (WPA_CAPABILITY_MFPC | WPA_CAPABILITY_MFPR) : 0;

		pos = emit_le16(pos, capab);

		if (parm->pmf_required && parm->mgmt_group_cipher == WIFI_CIPHER_TYPE_AES_CMAC128) {
			pos = emit_le16(pos, 0); /* 0 PMKIDs */
			pos = emit_be32(pos, RSN_CIPHER_SUITE_AES_128_CMAC);
		}
		*len_pos = (pos - buf) - 2;
	} else if (parm->proto == SEC_PROTO_WPA) {
		*pos++ = WLAN_EID_VENDOR;
		len_pos = pos++;
		pos = emit_buf(pos, wpa_oui_header, sizeof(wpa_oui_header));

		pos = put_cipher_suite_list(pos, cipher_to_rsn_oui(parm->group_cipher),
					    cipher_to_rsn_oui(parm->pairwise_cipher),
					    keymgmt_to_wpa_oui(parm->key_mgmt));

		*len_pos = (pos - buf) - 2;
	} else {
		return 0;
	}

	return (int)(pos - buf);
}

static int bridge_parse_wpa_ie(const uint8_t *wpa_ie, size_t wpa_ie_len, wifi_wpa_ie_t *data)
{
	const uint8_t *pos, *end;
	uint16_t count;
	uint16_t i;

	LOG_DBG("parse_wpa_ie: ie=%p len=%zu tag=%u", (void *)wpa_ie, wpa_ie_len,
		wpa_ie != NULL ? wpa_ie[0] : 0);

	if (data == NULL || wpa_ie == NULL || wpa_ie_len < 2) {
		return -1;
	}

	memset(data, 0, sizeof(*data));

	if (wpa_ie[0] == WLAN_EID_RSN) {
		data->proto = WPA_PROTO_RSN;
		pos = wpa_ie + 2 + 2; /* skip tag+len, version */
		end = wpa_ie + 2 + wpa_ie[1];
	} else if (wpa_ie[0] == WLAN_EID_VENDOR && wpa_ie_len >= 8 && wpa_ie[2] == 0x00 &&
		   wpa_ie[3] == 0x50 && wpa_ie[4] == 0xF2 && wpa_ie[5] == 0x01) {
		/* WPA IE */
		data->proto = WPA_PROTO_WPA;
		pos = wpa_ie + 2 + 4 + 2; /* skip tag+len, OUI+type, version */
		end = wpa_ie + 2 + wpa_ie[1];
	} else {
		return -1;
	}

	if (pos + 4 > end) {
		return 0;
	}
	data->group_cipher = oui_to_cipher(pos);
	pos += 4;

	if (pos + 2 > end) {
		return 0;
	}
	count = sys_get_le16(pos);
	pos += 2;

	if (count > 0 && pos + 4 <= end) {
		data->pairwise_cipher = oui_to_cipher(pos);
		if (pos + (size_t)count * 4 > end) {
			return 0;
		}
		pos += count * 4;
	}

	if (pos + 2 > end) {
		return 0;
	}
	count = sys_get_le16(pos);
	pos += 2;

	for (i = 0; i < count && pos + 4 <= end; i++) {
		data->key_mgmt |= oui_to_akm(pos);
		pos += 4;
	}

	if (data->proto == WPA_PROTO_RSN && pos + 2 <= end) {
		data->capabilities = sys_get_le16(pos);
		pos += 2;

		/* PMKID count + PMKIDs */
		if (pos + 2 <= end) {
			data->num_pmkid = sys_get_le16(pos);
			pos += 2;
			if (data->num_pmkid > 0 &&
			    (size_t)data->num_pmkid <= (size_t)(end - pos) / 16) {
				data->pmkid = pos;
				pos += data->num_pmkid * 16;
			} else {
				data->num_pmkid = 0;
			}
		}

		/* Group management cipher */
		if (pos + 4 <= end) {
			data->mgmt_group_cipher = oui_to_cipher(pos);
		}
	}

	return 0;
}

/* Work handlers */

static void assoc_resp_work_handler(struct k_work *work)
{
	struct bflb_supp_ctx *ctx = CONTAINER_OF(work, struct bflb_supp_ctx, assoc_work);
	union wpa_event_data event;

	if (ctx->supp_drv_if_ctx == NULL || ctx->supp_cb.assoc_resp == NULL) {
		return;
	}

	/* Bring the interface up before notifying hostap -- hostap will
	 * immediately start the 4WHS which requires L2 to be ready.
	 * The blob calls bridge_sta_connect before posting
	 * WL80211_EVT_STA_CONNECTED, so carrier_on may not have run yet.
	 */
	if (ctx->wdev != NULL && ctx->wdev->iface != NULL) {
		net_if_carrier_on(ctx->wdev->iface);
	}

	memset(&event, 0, sizeof(event));
	event.assoc_info.addr = ctx->assoc_bssid;
	event.assoc_info.freq = ctx->assoc_freq;
	LOG_DBG("assoc_resp: freq=%u authenticating=%u associated=%u", ctx->assoc_freq,
		wl80211_glb.authenticating, wl80211_glb.associated);
	ctx->supp_cb.assoc_resp(ctx->supp_drv_if_ctx, &event, WLAN_STATUS_SUCCESS);
}

static void connect_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bflb_supp_ctx *ctx = CONTAINER_OF(dwork, struct bflb_supp_ctx, connect_work);
	int ret;

	if (ctx->connect_cancelled) {
		mbedtls_platform_zeroize(ctx->connect_params.password,
					 sizeof(ctx->connect_params.password));
		return;
	}

	if (wl80211_glb.scanning != 0) {
		if (++ctx->connect_retries > BFLB_CONNECT_RETRY_MAX) {
			LOG_ERR("Connect aborted: blob stuck scanning");
			mbedtls_platform_zeroize(ctx->connect_params.password,
						 sizeof(ctx->connect_params.password));
			if (ctx->supp_drv_if_ctx != NULL && ctx->supp_cb.assoc_resp != NULL) {
				union wpa_event_data event;

				memset(&event, 0, sizeof(event));
				ctx->supp_cb.assoc_resp(ctx->supp_drv_if_ctx, &event,
							WLAN_STATUS_UNSPECIFIED_FAILURE);
			}
			return;
		}
		LOG_DBG("Blob still scanning, retrying connect in 100ms");
		k_work_reschedule(&ctx->connect_work, K_MSEC(BFLB_CONNECT_WORK_DELAY_MS));
		return;
	}

	wl80211_supplicant_set_assoc_ie(NULL, 0);

	LOG_DBG("connect_work: connecting=%u scanning=%u", wl80211_glb.connecting,
		wl80211_glb.scanning);
	ret = wl80211_sta_connect(&ctx->connect_params);
	mbedtls_platform_zeroize(ctx->connect_params.password,
				 sizeof(ctx->connect_params.password));
	if (ret != 0) {
		LOG_ERR("wl80211_sta_connect failed: %d", ret);
		if (ctx->supp_drv_if_ctx != NULL && ctx->supp_cb.assoc_resp != NULL) {
			union wpa_event_data event;

			memset(&event, 0, sizeof(event));
			ctx->supp_cb.assoc_resp(ctx->supp_drv_if_ctx, &event,
						WLAN_STATUS_UNSPECIFIED_FAILURE);
		}
		return;
	}
	LOG_DBG("wl80211_sta_connect submitted");
}

/*
 * Blob bridge callbacks (struct wpa_funcs).
 *
 * The blob has an internal supplicant interface (wpa_funcs) that it calls
 * during connection. In the stock SDK these point to the blob's built-in
 * supplicant. We replace them with our bridge that routes events to the
 * Zephyr hostap supplicant instead:
 *
 *   blob calls bridge_sta_config -> we build association IEs for the blob
 *   blob calls bridge_sta_connect -> we notify hostap that L2 assoc is done
 *   blob calls bridge_sta_disconnected_cb -> we notify hostap of deauth
 *   blob calls bridge_sta_rx_eapol -> we inject EAPOL into Zephyr net stack
 */

static bool bridge_sta_init(void)
{
	return true;
}

static bool bridge_sta_deinit(void)
{
	return true;
}

static void bridge_sta_config(wifi_connect_parm_t *parm)
{
	struct bflb_supp_ctx *ctx = &g_supp_ctx;
	int ie_len;

	if (parm == NULL) {
		return;
	}
	LOG_DBG("vif=%u sta=%u proto=%u key_mgmt=0x%x "
		"pairwise=%u group=%u pmf=%d",
		parm->vif_idx, parm->sta_idx, parm->proto, parm->key_mgmt, parm->pairwise_cipher,
		parm->group_cipher, parm->pmf_required);

	ctx->sta_idx = parm->sta_idx;
	LOG_DBG("saved sta_idx=%u", ctx->sta_idx);

	if (ctx->assoc_ie_len == 0) {
		ie_len = gen_assoc_ie(parm, ctx->assoc_ie);
		if (ie_len > 0) {
			ctx->assoc_ie_len = ie_len;
		}
	}
	if (ctx->rsnxe_len > 0 && ctx->assoc_ie_len + ctx->rsnxe_len <= sizeof(ctx->assoc_ie)) {
		memcpy(ctx->assoc_ie + ctx->assoc_ie_len, ctx->rsnxe, ctx->rsnxe_len);
		ctx->assoc_ie_len += ctx->rsnxe_len;
	}
	if (ctx->mdie_len > 0 && ctx->assoc_ie_len + ctx->mdie_len <= sizeof(ctx->assoc_ie)) {
		memcpy(ctx->assoc_ie + ctx->assoc_ie_len, ctx->mdie, ctx->mdie_len);
		ctx->assoc_ie_len += ctx->mdie_len;
	}

	if (ctx->assoc_ie_len > 0) {
		wl80211_supplicant_set_assoc_ie(ctx->assoc_ie, ctx->assoc_ie_len);
		LOG_HEXDUMP_DBG(ctx->assoc_ie, ctx->assoc_ie_len, "assoc IE");
	}
}

static void bridge_sta_connect(wifi_connect_parm_t *parm)
{
	struct bflb_supp_ctx *ctx = &g_supp_ctx;

	LOG_DBG("parm=%p drv_ctx=%p", parm, ctx->supp_drv_if_ctx);

	if (ctx->supp_drv_if_ctx == NULL || parm == NULL) {
		return;
	}

	/* Defer assoc_resp to a work queue -- we're called from blob context
	 * and must not re-enter blob APIs or trigger heavy hostap processing.
	 * assoc_bssid was already saved by bflb_wpa_supp_associate(); the
	 * blob's parm->bssid is often all-zeros, so don't overwrite.
	 */
	k_work_submit(&ctx->assoc_work);
}

static void bridge_sta_disconnected_cb(uint8_t reason_code)
{
	struct bflb_supp_ctx *ctx = &g_supp_ctx;
	union wpa_event_data event;

	LOG_DBG("disconnect: reason=%u", reason_code);

	if (ctx->supp_drv_if_ctx == NULL || ctx->supp_cb.deauth == NULL) {
		return;
	}

	if (ctx->wdev != NULL && ctx->wdev->iface != NULL &&
	    !net_if_is_carrier_ok(ctx->wdev->iface)) {
		return;
	}

	memset(&event, 0, sizeof(event));
	event.deauth_info.reason_code = reason_code;
	ctx->supp_cb.deauth(ctx->supp_drv_if_ctx, &event, NULL);
}

/* Return 1 = "handled" so the blob's internal supplicant doesn't also process it */
static int bridge_sta_rx_eapol(uint8_t *src_addr, uint8_t *buf, uint32_t len)
{
	ARG_UNUSED(src_addr);

	bflb_wifi_eapol_input(WL80211_VIF_STA, buf, len);
	return 1;
}

static struct wpa_funcs bridge_wpa_cb = {
	.wpa_sta_init = bridge_sta_init,
	.wpa_sta_deinit = bridge_sta_deinit,
	.wpa_sta_config = bridge_sta_config,
	.wpa_sta_connect = bridge_sta_connect,
	.wpa_sta_disconnected_cb = bridge_sta_disconnected_cb,
	.wpa_sta_rx_eapol = bridge_sta_rx_eapol,

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_AP
	.wpa_ap_init = bflb_ap_bridge_init,
	.wpa_ap_deinit = bflb_ap_bridge_deinit,
	.wpa_ap_join = bflb_ap_bridge_join,
	.wpa_ap_sta_associated = bflb_ap_bridge_sta_associated,
	.wpa_ap_remove = bflb_ap_bridge_remove,
	.wpa_ap_rx_eapol = bflb_ap_bridge_rx_eapol,
#endif

	.wpa_parse_wpa_ie = bridge_parse_wpa_ie,

	.wpa3_build_sae_msg = NULL,
	.wpa3_parse_sae_msg = NULL,
	.wpa3_clear_sae = NULL,
};

/* zep_wpa_supp_dev_ops implementation */

static void *bflb_wpa_supp_init(void *supp_drv_if_ctx, const char *iface_name,
				struct zep_wpa_supp_dev_callbk_fns *callbk_fns)
{
	struct bflb_supp_ctx *ctx = &g_supp_ctx;

	ARG_UNUSED(iface_name);

	ctx->wdev = &wl80211_dev;
	ctx->supp_drv_if_ctx = supp_drv_if_ctx;
	memcpy(&ctx->supp_cb, callbk_fns, sizeof(ctx->supp_cb));

	return ctx;
}

static void bflb_wpa_supp_deinit(void *if_priv)
{
	struct bflb_supp_ctx *ctx = if_priv;

	if (ctx != NULL) {
		mbedtls_platform_zeroize(ctx->connect_params.password,
					 sizeof(ctx->connect_params.password));
		memset(ctx->assoc_ie, 0, sizeof(ctx->assoc_ie));
		memset(&ctx->supp_cb, 0, sizeof(ctx->supp_cb));
		ctx->supp_drv_if_ctx = NULL;
	}
}

static int bflb_wpa_supp_scan2(void *if_priv, struct wpa_driver_scan_params *params)
{
	struct wl80211_scan_params scan_params = {0};
	int ret;

	ARG_UNUSED(if_priv);

	if (params != NULL && params->ssids[0].ssid_len > 0) {
		scan_params.ssid = params->ssids[0].ssid;
		scan_params.ssid_len = params->ssids[0].ssid_len;
	}

	LOG_DBG("scan2: pre scanning=%u connecting=%u", wl80211_glb.scanning,
		wl80211_glb.connecting);

	ret = wl80211_scan(&scan_params);
	if (ret != 0) {
		LOG_ERR("wl80211_scan failed: %d", ret);
		return -EIO;
	}

	bflb_privacy_cache_install_hook();

	LOG_DBG("scan2: post scanning=%u connecting=%u", wl80211_glb.scanning,
		wl80211_glb.connecting);

	return 0;
}

static int bflb_wpa_supp_scan_abort(void *if_priv)
{
	ARG_UNUSED(if_priv);
	return 0;
}

/*
 * Convert blob scan results into wpa_scan_res structs for the supplicant.
 *
 * The blob stores scan results as wl80211_scan_result_item with decoded
 * fields (key_mgmt bitmask, cipher flags) but no raw IEs. The supplicant
 * expects raw 802.11 IEs (SSID, RSN/WPA), so we synthesize them from the
 * blob's decoded fields -- unless a real RSN IE was cached from the beacon
 * frame, in which case we use that verbatim to avoid byte-level mismatches
 * in wpa_compare_rsn_ie() during EAPOL 4WHS msg 3 validation.
 */
static int bflb_wpa_supp_get_scan_results(void *if_priv)
{
	struct bflb_supp_ctx *ctx = if_priv;
	struct wl80211_scan_result_item *n;
	int count = 0;

	if (ctx == NULL || ctx->supp_drv_if_ctx == NULL) {
		return -EINVAL;
	}

	RB_FOREACH(n, _scan_result_tree, &wl80211_scan_result)
	{
		uint8_t ie_buf[MAX_ASSOC_IE_LEN + MAX_CACHED_RSN_IE_LEN + MAX_CACHED_RSNXE_LEN];
		uint8_t *ie_pos = ie_buf;
		size_t ssid_len = n->ssid != NULL ? strlen(n->ssid) : 0;
		size_t ie_len;
		uint8_t res_buf[sizeof(struct wpa_scan_res) + MAX_ASSOC_IE_LEN +
				MAX_CACHED_RSN_IE_LEN + MAX_CACHED_RSNXE_LEN]
			__aligned(__alignof__(struct wpa_scan_res));
		struct wpa_scan_res *r = (struct wpa_scan_res *)res_buf;
		bool more;
		uint8_t cached_len;
		const uint8_t *cached_rsn;
		uint8_t rsnxe_len;
		const uint8_t *cached_rsnxe;

		if (ssid_len > WIFI_SSID_MAX_LEN) {
			ssid_len = WIFI_SSID_MAX_LEN;
		}

		*ie_pos++ = WLAN_EID_SSID;
		*ie_pos++ = ssid_len;
		if (ssid_len != 0) {
			ie_pos = emit_buf(ie_pos, n->ssid, ssid_len);
		}

		cached_rsn = bflb_rsn_ie_cache_lookup(n->bssid, &cached_len);

		if (cached_rsn != NULL && cached_len > 0) {
			ie_pos = emit_buf(ie_pos, cached_rsn, cached_len);
		} else {
			ie_pos = bflb_append_security_ie(ie_pos, n);
		}

		cached_rsnxe = bflb_rsnxe_cache_lookup(n->bssid, &rsnxe_len);

		if (cached_rsnxe != NULL && rsnxe_len > 0) {
			ie_pos = emit_buf(ie_pos, cached_rsnxe, rsnxe_len);
		}

		ie_len = ie_pos - ie_buf;

		memset(r, 0, sizeof(*r));
		memcpy(r->bssid, n->bssid, ETH_ALEN);
		r->freq = _channel_to_freq(n->band == WL80211_BAND_5G ? WL80211_BAND_5G
								      : WL80211_BAND_2G4,
					   n->channel);
		r->level = n->rssi;
		r->ie_len = ie_len;
		r->caps = IEEE80211_CAP_ESS;

		if (n->flags &
		    (WL80211_SCAN_AP_RESULT_FLAGS_HAS_RSN | WL80211_SCAN_AP_RESULT_FLAGS_HAS_WPA)) {
			r->caps |= IEEE80211_CAP_PRIVACY;
		} else if (bflb_privacy_cache_lookup(n->bssid)) {
			r->caps |= IEEE80211_CAP_PRIVACY;
			LOG_DBG("privacy from cache for %02x:%02x:%02x:%02x:%02x:%02x", n->bssid[0],
				n->bssid[1], n->bssid[2], n->bssid[3], n->bssid[4], n->bssid[5]);
		} else {
			/* open network */
		}

		memcpy((uint8_t *)(r + 1), ie_buf, ie_len);

		more = (RB_NEXT(_scan_result_tree, &wl80211_scan_result, n) != NULL);
		ctx->supp_cb.scan_res(ctx->supp_drv_if_ctx, r, more);
		count++;
	}

	if (count == 0 && ctx->supp_cb.scan_done != NULL) {
		union wpa_event_data event;

		memset(&event, 0, sizeof(event));
		event.scan_info.aborted = false;
		ctx->supp_cb.scan_done(ctx->supp_drv_if_ctx, &event);
	}

	return 0;
}

/*
 * Authenticate callback -- the supplicant's SME calls this before associate.
 *
 * In a soft-MAC driver this would send an 802.11 Auth frame. In our full-MAC
 * design the blob handles auth internally as part of wl80211_sta_connect().
 * We synthesize an immediate success response so the SME proceeds to the
 * associate step, where we actually kick off the blob's connect.
 *
 * For SAE: the blob will separately send EXTERNAL_AUTH_IND once the connect
 * is started, triggering the real SAE handshake via _external_auth_ind().
 */
static int bflb_wpa_supp_authenticate(void *if_priv, struct wpa_driver_auth_params *params,
				      struct wpa_bss *curr_bss)
{
	struct bflb_supp_ctx *ctx = if_priv;

	ARG_UNUSED(curr_bss);

	if (ctx == NULL || params == NULL) {
		return -EINVAL;
	}

	if (ctx->supp_cb.auth_resp != NULL) {
		union wpa_event_data event;

		memset(&event, 0, sizeof(event));
		memcpy(event.auth.peer, params->bssid, ETH_ALEN);
		event.auth.auth_type = WLAN_AUTH_OPEN;
		event.auth.status_code = WLAN_STATUS_SUCCESS;
		ctx->supp_cb.auth_resp(ctx->supp_drv_if_ctx, &event);
	}

	return 0;
}

static void bflb_wpa_supp_fill_passphrase(struct bflb_supp_ctx *ctx,
					  struct wl80211_connect_params *conn)
{
	struct zep_drv_if_ctx *zep_ctx;
	struct wpa_supplicant *wpa_s;
	struct wpa_ssid *ssid;

	if (ctx->supp_drv_if_ctx == NULL) {
		return;
	}

	zep_ctx = ctx->supp_drv_if_ctx;
	wpa_s = zep_ctx != NULL ? zep_ctx->supp_if_ctx : NULL;
	ssid = wpa_s != NULL ? wpa_s->current_ssid : NULL;

	if (ssid != NULL && ssid->passphrase != NULL) {
		size_t plen = strlen(ssid->passphrase);

		memcpy(conn->password, ssid->passphrase, MIN(plen, sizeof(conn->password) - 1));
	}
}

/* WPA supplicant associate callback -- bridge between hostap and the full-MAC blob.
 * Translates supplicant's association parameters into wl80211_connect_params,
 * extracts RSN/RSNXE/MDIE from the supplicant's IE buffer for later use in
 * the 4-way handshake, and defers the actual blob connect call via a work item
 * (the blob's PHY needs time to leave scan state before starting a join).
 */
static int bflb_wpa_supp_associate(void *if_priv, struct wpa_driver_associate_params *params)
{
	struct bflb_supp_ctx *ctx = if_priv;
	struct wl80211_connect_params conn = {0};

	if (ctx == NULL || params == NULL) {
		return -EINVAL;
	}

	if (params->ssid != NULL && params->ssid_len > 0) {
		memcpy(conn.ssid, params->ssid, MIN(params->ssid_len, sizeof(conn.ssid) - 1));
	}

	if (params->bssid != NULL) {
		memcpy(conn.bssid, params->bssid, ETH_ALEN);
		memcpy(ctx->assoc_bssid, params->bssid, ETH_ALEN);
	}

	if (params->freq.freq != 0) {
		conn.channel = wl80211_freq_to_channel(params->freq.freq);
		ctx->assoc_freq = params->freq.freq;
	}

	ctx->rsnxe_len = 0;
	ctx->mdie_len = 0;

	/* Walk the supplicant's association IEs (TLV chain) and extract
	 * RSN, RSNXE and MDIE -- these are cached for the 4-way handshake
	 * and for FT (Fast BSS Transition) support in the blob bridge.
	 */
	if (params->wpa_ie != NULL && params->wpa_ie_len > 0) {
		const uint8_t *ie = params->wpa_ie;
		uint16_t remaining = params->wpa_ie_len;

		while (remaining >= IEEE80211_TLV_HDR_LEN) {
			uint8_t eid = ie[0];
			uint8_t elen = ie[1];
			uint16_t ie_total = IEEE80211_TLV_HDR_LEN + elen;

			if (ie_total > remaining) {
				break;
			}
			if (eid == WLAN_EID_RSN) {
				if (ie_total <= sizeof(ctx->assoc_ie)) {
					memcpy(ctx->assoc_ie, ie, ie_total);
					ctx->assoc_ie_len = ie_total;
					LOG_HEXDUMP_DBG(ctx->assoc_ie, ctx->assoc_ie_len, "RSN IE");
				}
			} else if (eid == WLAN_EID_RSNX) {
				if (ie_total <= sizeof(ctx->rsnxe)) {
					memcpy(ctx->rsnxe, ie, ie_total);
					ctx->rsnxe_len = ie_total;
					LOG_HEXDUMP_DBG(ctx->rsnxe, ctx->rsnxe_len, "RSNXE");
				}
			} else if (eid == WLAN_EID_MOBILITY_DOMAIN) {
				if (ie_total <= sizeof(ctx->mdie)) {
					memcpy(ctx->mdie, ie, ie_total);
					ctx->mdie_len = ie_total;
					LOG_HEXDUMP_DBG(ctx->mdie, ctx->mdie_len, "MDIE");
				}
			} else {
				/* skip unhandled IE */
			}
			ie += ie_total;
			remaining -= ie_total;
		}
	}

	bflb_wpa_supp_fill_passphrase(ctx, &conn);

	ctx->open_network = (params->key_mgmt_suite == WPA_KEY_MGMT_NONE);

	switch (params->key_mgmt_suite) {
	case WPA_KEY_MGMT_SAE:
	case WPA_KEY_MGMT_FT_SAE:
		conn.auth_type = WL80211_AUTHTYPE_SAE;
		conn.mfp = WL80211_MFP_REQUIRED;
		break;
	default:
		if (params->auth_alg & WPA_AUTH_ALG_SHARED) {
			conn.auth_type = WL80211_AUTHTYPE_SHARED_KEY;
		} else {
			conn.auth_type = WL80211_AUTHTYPE_OPEN_SYSTEM;
		}
		break;
	}

	LOG_DBG("associate: ssid='%s' bssid=%02x:%02x:%02x:%02x:%02x:%02x ch=%d auth=%d mfp=%d"
		" km=0x%x connecting=%u",
		conn.ssid, conn.bssid[0], conn.bssid[1], conn.bssid[2], conn.bssid[3],
		conn.bssid[4], conn.bssid[5], conn.channel, conn.auth_type, conn.mfp,
		params->key_mgmt_suite, wl80211_glb.connecting);

	/* Defer the actual wl80211_sta_connect() call. The blob's scan PHY
	 * state needs time to return to idle after the preceding hostap scan;
	 * calling connect immediately triggers an internal join-scan that
	 * asserts in ke_msg_send -> scan_set_channel_request.
	 */
	memcpy(&ctx->connect_params, &conn, sizeof(conn));
	ctx->connect_retries = 0;
	ctx->connect_cancelled = false;
	k_work_reschedule(&ctx->connect_work, K_MSEC(BFLB_CONNECT_WORK_DELAY_MS));

	return 0;
}

static int bflb_wpa_supp_deauthenticate(void *if_priv, const char *addr, unsigned short reason_code)
{
	struct bflb_supp_ctx *ctx = if_priv;

	ARG_UNUSED(addr);

	LOG_DBG("deauthenticate: reason=%u authenticating=%u connecting=%u associated=%u",
		reason_code, wl80211_glb.authenticating, wl80211_glb.connecting,
		wl80211_glb.associated);
	if (wl80211_glb.authenticating != 0) {
		k_work_cancel_delayable(&sae_complete_work);
		bflb_send_external_auth_rsp(WLAN_STATUS_UNSPECIFIED_FAILURE);
	}
	if (ctx != NULL) {
		ctx->connect_cancelled = true;
		k_work_cancel_delayable(&ctx->connect_work);
	}
	wl80211_sta_disconnect();
	return 0;
}

static int bflb_wpa_supp_set_key(void *if_priv, const unsigned char *ifname, enum wpa_alg alg,
				 const unsigned char *addr, int key_idx, int set_tx,
				 const unsigned char *seq, size_t seq_len, const unsigned char *key,
				 size_t key_len, enum key_flag key_flag)
{
	struct bflb_supp_ctx *ctx = if_priv;
	uint8_t vif_idx = WL80211_VIF_STA;
	uint8_t sta_idx = 0;
	uint8_t cipher_suite;
	uint8_t tkip_key[TKIP_KEY_LEN];
	bool pairwise;

	ARG_UNUSED(ifname);
	ARG_UNUSED(seq);
	ARG_UNUSED(seq_len);

	if (ctx == NULL) {
		return -EINVAL;
	}

	if (ctx->wdev->ap_enabled) {
		vif_idx = WL80211_VIF_AP;
	}

	pairwise = (key_flag & KEY_FLAG_PAIRWISE) != 0;

	if (alg == WPA_ALG_NONE) {
		if (pairwise) {
			if (vif_idx == WL80211_VIF_AP && addr != NULL) {
				sta_idx = bflb_aid_list_find_sta(addr);
				if (sta_idx == STA_IDX_INVALID) {
					sta_idx = bflb_ap_find_sta(addr);
				}
			} else {
				sta_idx = ctx->sta_idx;
			}
			wl80211_mac_clr_ptk(vif_idx == WL80211_VIF_AP
						    ? wl80211_mac_vif[WL80211_VIF_AP]
						    : vif_idx,
					    sta_idx);
		} else {
			wl80211_mac_clr_gtk(vif_idx == WL80211_VIF_AP
						    ? wl80211_mac_vif[WL80211_VIF_AP]
						    : vif_idx,
					    STA_IDX_INVALID);
		}
		return 0;
	}

	if (alg == WPA_ALG_BIP_CMAC_128) {
		if (key != NULL && key_len == 16) {
			wl80211_mac_set_key(vif_idx == WL80211_VIF_AP
						    ? wl80211_mac_vif[WL80211_VIF_AP]
						    : vif_idx,
					    STA_IDX_INVALID, key_idx, (uint8_t *)key, key_len,
					    MAC_CIPHER_BIP_CMAC_128, 0, 0);
		}
		return 0;
	}

	/* Determine cipher suite from alg + key_len */
	switch (alg) {
	case WPA_ALG_CCMP:
		cipher_suite = mac_cipher_suite_value(RSN_CIPHER_SUITE_CCMP);
		break;
	case WPA_ALG_TKIP:
		cipher_suite = mac_cipher_suite_value(RSN_CIPHER_SUITE_TKIP);
		/* hostap orders TKIP MIC keys as [TxMIC][RxMIC], but the
		 * blob's hardware expects [RxMIC][TxMIC] -- swap them.
		 */
		if (key_len == TKIP_KEY_LEN) {
			memcpy(tkip_key, key, TKIP_KEY_LEN);
			memcpy(tkip_key + TKIP_MIC_KEY_OFF,
			       key + TKIP_MIC_KEY_OFF + TKIP_MIC_KEY_LEN, TKIP_MIC_KEY_LEN);
			memcpy(tkip_key + TKIP_MIC_KEY_OFF + TKIP_MIC_KEY_LEN,
			       key + TKIP_MIC_KEY_OFF, TKIP_MIC_KEY_LEN);
			key = tkip_key;
		}
		break;
	case WPA_ALG_WEP:
		if (key_len == WEP104_KEY_LEN) {
			cipher_suite = mac_cipher_suite_value(RSN_CIPHER_SUITE_WEP104);
		} else {
			cipher_suite = mac_cipher_suite_value(RSN_CIPHER_SUITE_WEP40);
		}
		break;
	default:
		return -ENOTSUP;
	}

	if (pairwise) {
		if (vif_idx == WL80211_VIF_AP && addr != NULL) {
			sta_idx = bflb_aid_list_find_sta(addr);
			if (sta_idx == STA_IDX_INVALID) {
				sta_idx = bflb_ap_find_sta(addr);
			}
		} else {
			sta_idx = ctx->sta_idx;
		}
	}

	/* Resolve the AP's STA index from the VIF table -- the value from
	 * bridge_sta_config may be stale after a reconnect.
	 */
	if (pairwise && vif_idx == WL80211_VIF_STA) {
		void *vif = vif_info_get_vif(WL80211_VIF_STA);

		if (vif != NULL) {
			uint8_t ap_sta = mac_vif_get_sta_ap_id(vif);

			LOG_DBG("set_key: bridge_sta_idx=%u ap_sta_id=%u", sta_idx, ap_sta);
			if (ap_sta != STA_IDX_INVALID) {
				sta_idx = ap_sta;
			}
		}
	}

	LOG_DBG("set_key: %s vif=%u sta=%u cipher=%u key_len=%zu "
		"key_idx=%d set_tx=%d",
		pairwise ? "PTK" : "GTK", vif_idx, pairwise ? sta_idx : STA_IDX_INVALID,
		cipher_suite, key_len, key_idx, set_tx);

	if (pairwise && vif_idx == WL80211_VIF_STA) {
		/* Wait for EAPOL msg 4 TX completion before installing the
		 * PTK -- the AP must receive msg 4 unencrypted before we
		 * switch the hardware to the new key.
		 */
		bflb_wifi_wait_eapol_tx_done(K_MSEC(BFLB_EAPOL_TX_TIMEOUT_MS));
	}

	wl80211_mac_set_key(vif_idx == WL80211_VIF_AP ? wl80211_mac_vif[WL80211_VIF_AP] : vif_idx,
			    pairwise ? sta_idx : STA_IDX_INVALID, key_idx, (uint8_t *)key, key_len,
			    cipher_suite, set_tx, pairwise);

	mbedtls_platform_zeroize(tkip_key, sizeof(tkip_key));
	return 0;
}

static int bflb_wpa_supp_set_supp_port(void *if_priv, int authorized, char *bssid)
{
	struct bflb_supp_ctx *ctx = if_priv;
	uint8_t real_sta;
	void *vif;

	ARG_UNUSED(bssid);

	if (ctx == NULL) {
		return -EINVAL;
	}

	if (ctx->wdev->ap_enabled) {
		if (bssid != NULL) {
			uint8_t idx = bflb_aid_list_find_sta((const uint8_t *)bssid);

			if (idx == STA_IDX_INVALID) {
				idx = bflb_ap_find_sta((const uint8_t *)bssid);
			}
			if (idx != STA_IDX_INVALID) {
				wl80211_mac_ctrl_port(idx, authorized);
			}
		}
		return 0;
	}

	real_sta = ctx->sta_idx;
	vif = vif_info_get_vif(WL80211_VIF_STA);

	if (vif != NULL) {
		uint8_t ap_sta = mac_vif_get_sta_ap_id(vif);

		if (ap_sta != STA_IDX_INVALID) {
			real_sta = ap_sta;
		}
	}

	LOG_DBG("set_supp_port: authorized=%d sta_idx=%u (real=%u)", authorized, ctx->sta_idx,
		real_sta);
	wl80211_mac_ctrl_port(real_sta, authorized);

	if (authorized != 0) {
		wl80211_glb.link_up = 1;

		/* Populate supplicant's PHY mode flags from the blob's TX
		 * format string (e.g. "HE"=Wi-Fi 6, "VHT"=Wi-Fi 5, "HT"=Wi-Fi 4).
		 * The supplicant uses these for capability reporting.
		 */
		if (ctx->supp_drv_if_ctx != NULL) {
			struct zep_drv_if_ctx *zep_ctx = ctx->supp_drv_if_ctx;
			struct wpa_supplicant *wpa_s = zep_ctx->supp_if_ctx;

			if (wpa_s != NULL) {
				const char *fmt = export_stats_get_tx_format();

				wpa_s->connection_set = 1;
				wpa_s->connection_ht = 0;
				wpa_s->connection_vht = 0;
				wpa_s->connection_he = 0;
				if (fmt != NULL && fmt[0] == 'H' && fmt[1] == 'E') {
					wpa_s->connection_he = 1;
				} else if (fmt != NULL && fmt[0] == 'V') {
					wpa_s->connection_vht = 1;
				} else if (fmt != NULL && fmt[0] == 'H' && fmt[1] == 'T') {
					wpa_s->connection_ht = 1;
				}
			}
		}
	} else {
		wl80211_glb.link_up = 0;
	}

	return 0;
}

static unsigned long bflb_tx_rate_kbps(void)
{
	const char *fmt = export_stats_get_tx_format();
	uint8_t mcs = export_stats_get_tx_mcs();

	if (fmt == NULL || fmt[0] == '\0') {
		return 0;
	}
	if (fmt[0] == 'H' && fmt[1] == 'E') {
		if (mcs < ARRAY_SIZE(HE_RATE_KBPS_BW20)) {
			return (unsigned long)HE_RATE_KBPS_BW20[mcs] * 100UL;
		}
	} else if (fmt[0] == 'V') {
		if (mcs < ARRAY_SIZE(VHT_RATE_KBPS_BW20)) {
			return (unsigned long)VHT_RATE_KBPS_BW20[mcs] * 100UL;
		}
	} else if (fmt[0] == 'H' && fmt[1] == 'T') {
		if (mcs < ARRAY_SIZE(HT_RATE_KBPS_BW20)) {
			return (unsigned long)HT_RATE_KBPS_BW20[mcs] * 100UL;
		}
	} else if (fmt[0] == 'N') {
		if (mcs < ARRAY_SIZE(NONHT_OFDM_KBPS)) {
			return (unsigned long)NONHT_OFDM_KBPS[mcs] * 100UL;
		}
	} else {
		/* unknown format */
	}
	return 0;
}

static int bflb_wpa_supp_signal_poll(void *if_priv, struct wpa_signal_info *si,
				     unsigned char *bssid)
{
	struct mac_addr mac_bssid;
	uint16_t aid;
	int8_t rssi;
	void *vif;
	int ch;

	ARG_UNUSED(if_priv);

	if (si == NULL) {
		return -EINVAL;
	}

	memset(si, 0, sizeof(*si));

	if (wl80211_sta_is_connected() == 0) {
		return -ENOLINK;
	}

	vif = vif_info_get_vif(WL80211_VIF_STA);
	if (vif == NULL) {
		return -ENOLINK;
	}

	mac_vif_get_sta_status(vif, &mac_bssid, &aid, &rssi);
	si->data.signal = rssi;
	si->data.current_tx_rate = bflb_tx_rate_kbps();
	si->current_noise = BFLB_NOISE_FLOOR_DBM;

	ch = wl80211_glb.last_connect_params != NULL ? wl80211_sta_get_channel() : -1;
	if (ch > 0) {
		uint8_t band = (ch > 14) ? WL80211_BAND_5G : WL80211_BAND_2G4;

		si->frequency = _channel_to_freq(band, ch);
	}

	if (bssid != NULL) {
		memcpy(bssid, &mac_bssid, ETH_ALEN);
	}

	return 0;
}

static int bflb_wpa_supp_get_capa(void *if_priv, struct wpa_driver_capa *capa)
{
	ARG_UNUSED(if_priv);

	if (capa == NULL) {
		return -EINVAL;
	}

	memset(capa, 0, sizeof(*capa));

	capa->flags = WPA_DRIVER_FLAGS_SME | WPA_DRIVER_FLAGS_SAE |
		      WPA_DRIVER_FLAGS_SET_KEYS_AFTER_ASSOC_DONE | WPA_DRIVER_FLAGS_AP;

	capa->enc = WPA_DRIVER_CAPA_ENC_CCMP | WPA_DRIVER_CAPA_ENC_TKIP |
		    WPA_DRIVER_CAPA_ENC_WEP40 | WPA_DRIVER_CAPA_ENC_WEP104;
	capa->key_mgmt = WPA_DRIVER_CAPA_KEY_MGMT_WPA | WPA_DRIVER_CAPA_KEY_MGMT_WPA2 |
			 WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK | WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK |
			 WPA_DRIVER_CAPA_KEY_MGMT_SAE;

	capa->max_scan_ssids = 1;

	return 0;
}

static int bflb_wpa_supp_get_wiphy(void *if_priv)
{
	struct bflb_supp_ctx *ctx = if_priv;
	struct wpa_supp_event_supported_band band = {0};
	const struct ieee80211_dot_d *country;
	int i;

	if (ctx == NULL || ctx->supp_drv_if_ctx == NULL || ctx->supp_cb.get_wiphy_res == NULL) {
		return -EINVAL;
	}

	country = wl80211_get_country();

	band.band = 0; /* NL80211_BAND_2GHZ */
	band.wpa_supp_n_channels = 0;

	if (country != NULL) {
		for (i = 0; i < country->channel24G_num &&
			    band.wpa_supp_n_channels < WPA_SUPP_SBAND_MAX_CHANNELS;
		     i++) {
			uint8_t ch = country->channel24G_chan[i];
			uint16_t freq =
				(ch == 14) ? BFLB_24GHZ_CH14_FREQ
					   : (BFLB_24GHZ_BASE_FREQ + ch * BFLB_24GHZ_CH_SPACING);

			band.channels[band.wpa_supp_n_channels].center_frequency = freq;
			band.channels[band.wpa_supp_n_channels].wpa_supp_max_power =
				BFLB_DEFAULT_MAX_DBM;
			band.channels[band.wpa_supp_n_channels].ch_valid = 1;
			band.wpa_supp_n_channels++;
		}
	} else {
		/* Default: channels 1-11 */
		for (i = 1; i <= 11; i++) {
			band.channels[band.wpa_supp_n_channels].center_frequency =
				BFLB_24GHZ_BASE_FREQ + i * BFLB_24GHZ_CH_SPACING;
			band.channels[band.wpa_supp_n_channels].wpa_supp_max_power =
				BFLB_DEFAULT_MAX_DBM;
			band.channels[band.wpa_supp_n_channels].ch_valid = 1;
			band.wpa_supp_n_channels++;
		}
	}

	band.wpa_supp_n_bitrates = ARRAY_SIZE(BG_RATES_100KBPS);
	for (i = 0; i < (int)ARRAY_SIZE(BG_RATES_100KBPS); i++) {
		band.bitrates[i].wpa_supp_bitrate = BG_RATES_100KBPS[i];
	}

	band.ht_cap.wpa_supp_ht_supported = 1;
	band.ht_cap.wpa_supp_cap = BFLB_HT_CAP_HT20_SGI20;

	ctx->supp_cb.get_wiphy_res(ctx->supp_drv_if_ctx, &band);
	ctx->supp_cb.get_wiphy_res(ctx->supp_drv_if_ctx, NULL);

	return 0;
}

static int bflb_wpa_supp_set_country(void *priv, const char *alpha2)
{
	char code[3];

	ARG_UNUSED(priv);

	if (alpha2 == NULL || alpha2[0] == '\0' || alpha2[1] == '\0') {
		return -EINVAL;
	}

	code[0] = alpha2[0];
	code[1] = alpha2[1];
	code[2] = '\0';
	return wl80211_set_country_code(code) != 0 ? -EINVAL : 0;
}

static int bflb_wpa_supp_get_country(void *priv, char *alpha2)
{
	const struct ieee80211_dot_d *country;

	ARG_UNUSED(priv);

	country = wl80211_get_country();
	if (country == NULL || country->code == NULL) {
		return -ENOENT;
	}

	alpha2[0] = country->code[0];
	alpha2[1] = country->code[1];

	return 0;
}

static int bflb_wpa_supp_send_mlme(void *if_priv, const u8 *data, size_t data_len, int noack,
				   unsigned int freq, int no_cck, int offchanok,
				   unsigned int wait_time, int cookie)
{
	struct bflb_supp_ctx *ctx = if_priv;
	int ret;

	ARG_UNUSED(noack);
	ARG_UNUSED(no_cck);
	ARG_UNUSED(offchanok);
	ARG_UNUSED(wait_time);
	ARG_UNUSED(cookie);

	if (data == NULL || data_len < IEEE80211_MGMT_HDR_LEN) {
		return -EINVAL;
	}

	/* Full-MAC blob handles AP management frames internally */
	if (ctx->wdev->ap_enabled) {
		return 0;
	}

	/* Full-MAC blob handles most STA mgmt frames internally.
	 * Only pass through Auth frames (subtype 0xb0) for SAE external auth.
	 */
	if ((data[0] & IEEE80211_FC_SUBTYPE_MASK) != IEEE80211_STYPE_AUTH) {
		return 0;
	}

	LOG_DBG("send_mlme: len=%zu freq=%u fc=0x%04x", data_len, freq, data[0] | (data[1] << 8));
	LOG_HEXDUMP_DBG(data, MIN(data_len, 30), "send_mlme frame header");

	ret = wl80211_output_raw(WL80211_VIF_STA, (void *)data, data_len, WL80211_MAC_TX_FLAG_MGMT,
				 NULL, NULL);
	LOG_DBG("send_mlme: output_raw ret=%d", ret);
	if (ret != 0) {
		LOG_ERR("send_mlme: output_raw failed: %d", ret);
		return -EIO;
	}

	return 0;
}

static int bflb_wpa_supp_get_conn_info(void *if_priv, struct wpa_conn_info *info)
{
	void *vif;

	ARG_UNUSED(if_priv);

	if (info == NULL) {
		return -EINVAL;
	}

	memset(info, 0, sizeof(*info));

	vif = vif_info_get_vif(WL80211_VIF_STA);
	if (vif != NULL) {
		info->beacon_interval = mac_vif_get_bcn_int(vif);
	}

	return 0;
}

static int bflb_wpa_supp_register_frame(void *if_priv, u16 type, const u8 *match, size_t match_len,
					bool multicast)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(type);
	ARG_UNUSED(match);
	ARG_UNUSED(match_len);
	ARG_UNUSED(multicast);
	return 0;
}

/* Public functions */

void bflb_wpa_supp_register_wpa_cbs(void)
{
	k_work_init(&g_supp_ctx.assoc_work, assoc_resp_work_handler);
	k_work_init_delayable(&g_supp_ctx.connect_work, connect_work_handler);
	wl80211_supplicant_register_wpa_cb_internal(&bridge_wpa_cb);
	wpa_cbs->wpa_sta_init();
}

void bflb_wifi_eapol_input(uint8_t vif_type, uint8_t *payload, size_t len)
{
	struct bflb_wifi_dev *wdev = &wl80211_dev;
	struct net_pkt *pkt;

	if (payload == NULL || len < ETH_HDR_LEN || len > NET_ETH_MTU + ETH_HDR_LEN) {
		return;
	}

	LOG_DBG("EAPOL RX: vif=%u len=%zu first=%02x%02x", vif_type, len, payload[0], payload[1]);

	if (wdev->iface == NULL) {
		return;
	}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_AP
	if (vif_type == WL80211_VIF_AP && len > ETH_HDR_LEN) {
		/* Blob payload: [dst 6B][src 6B][type 2B][EAPOL data...]
		 * Deliver via EVENT_EAPOL_RX to the supplicant thread which
		 * routes to hostapd internally. Deep-copy the buffers since
		 * the event is queued asynchronously; cleanup is in
		 * supp_main.c's EVENT_EAPOL_RX handler.
		 */
		struct zep_drv_if_ctx *drv_ctx = g_supp_ctx.supp_drv_if_ctx;
		uint8_t *src_copy;
		uint8_t *data_copy;
		size_t eapol_len;
		union wpa_event_data event;

		if (drv_ctx == NULL || drv_ctx->supp_if_ctx == NULL) {
			return;
		}

		eapol_len = len - ETH_HDR_LEN;
		src_copy = os_zalloc(ETH_ALEN);
		data_copy = os_zalloc(eapol_len);
		if (src_copy == NULL || data_copy == NULL) {
			os_free(src_copy);
			os_free(data_copy);
			return;
		}
		memcpy(src_copy, payload + ETH_ALEN, ETH_ALEN);
		memcpy(data_copy, payload + ETH_HDR_LEN, eapol_len);

		LOG_DBG("AP EAPOL RX: %zu bytes from %02x:%02x:%02x:%02x:%02x:%02x", eapol_len,
			src_copy[0], src_copy[1], src_copy[2], src_copy[3], src_copy[4],
			src_copy[5]);

		memset(&event, 0, sizeof(event));
		event.eapol_rx.src = src_copy;
		event.eapol_rx.data = data_copy;
		event.eapol_rx.data_len = eapol_len;
		event.eapol_rx.encrypted = FRAME_ENCRYPTION_UNKNOWN;
		event.eapol_rx.link_id = -1;

		wpa_supplicant_event_wrapper(drv_ctx->supp_if_ctx, EVENT_EAPOL_RX, &event);
		return;
	}
#endif

	pkt = net_pkt_rx_alloc_with_buffer(wdev->iface, len, AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		return;
	}

	if (net_pkt_write(pkt, payload, len) != 0) {
		net_pkt_unref(pkt);
		return;
	}

	if (net_recv_data(wdev->iface, pkt) != 0) {
		net_pkt_unref(pkt);
	}
}

void bflb_wpa_supp_scan_done_event(void)
{
	struct bflb_supp_ctx *ctx = &g_supp_ctx;
	union wpa_event_data event;

	if (ctx->supp_drv_if_ctx == NULL || ctx->supp_cb.scan_done == NULL) {
		return;
	}

	memset(&event, 0, sizeof(event));
	event.scan_info.aborted = false;
	ctx->supp_cb.scan_done(ctx->supp_drv_if_ctx, &event);
}

void bflb_wpa_supp_sta_connected_event(void)
{
	struct bflb_supp_ctx *ctx = &g_supp_ctx;

	if (ctx->open_network) {
		ctx->open_network = false;
		LOG_DBG("open network connected, sending assoc_resp");
		k_work_submit(&ctx->assoc_work);
	}
}

/* External auth (SAE) -- bypasses Zephyr hostap callback interface */

/*
 * Tell the blob's SM (Station Management) task that external authentication
 * is complete. The blob uses a ke_msg IPC mechanism: we allocate a response
 * message, fill in the result, and post it to TASK_SM. This mirrors what
 * the blob's internal supplicant would do after SAE completes.
 */
static void bflb_send_external_auth_rsp(uint16_t status)
{
	struct sm_external_auth_required_rsp *rsp;

	rsp = ke_msg_alloc(SM_EXTERNAL_AUTH_REQUIRED_RSP, TASK_SM, TASK_API, sizeof(*rsp));
	if (rsp != NULL) {
		rsp->vif_idx = WL80211_VIF_STA;
		rsp->status = status;
		ke_msg_send(rsp);
	}
	wl80211_glb.authenticating = 0;
	LOG_DBG("external_auth_rsp: status=%u", status);
}

/*
 * Blob ke_msg handler for SM_EXTERNAL_AUTH_REQUIRED_IND.
 *
 * When the blob needs the host to perform SAE authentication (instead of
 * its internal supplicant), it posts this message. We translate it into a
 * hostap EVENT_EXTERNAL_AUTH event so the Zephyr supplicant drives the
 * SAE handshake via send_mlme / _sme_auth_mgmt_rx.
 */
int _external_auth_ind(ke_msg_id_t const msgid, void *param, ke_task_id_t const dest_id,
		       ke_task_id_t const src_id)
{
	struct sm_external_auth_required_ind *ind = param;
	struct bflb_supp_ctx *ctx = &g_supp_ctx;
	struct zep_drv_if_ctx *drv_if_ctx;
	union wpa_event_data event;

	ARG_UNUSED(msgid);
	ARG_UNUSED(dest_id);
	ARG_UNUSED(src_id);

	if (ctx->supp_drv_if_ctx == NULL) {
		LOG_WRN("external_auth_ind: no supplicant context");
		return -EINVAL;
	}

	if (wl80211_glb.authenticating != 0) {
		return 0;
	}

	memcpy(ctx->ext_auth_bssid, ind->bssid.array, ETH_ALEN);
	memcpy(ctx->ext_auth_ssid, ind->ssid.array, MIN(ind->ssid.length, MAC_SSID_LEN));

	wl80211_glb.authenticating = 1;

	memset(&event, 0, sizeof(event));
	event.external_auth.action = EXT_AUTH_START;
	event.external_auth.bssid = ctx->ext_auth_bssid;
	event.external_auth.ssid = ctx->ext_auth_ssid;
	event.external_auth.ssid_len = ind->ssid.length;
	event.external_auth.key_mgmt_suite = ind->akm;

	drv_if_ctx = ctx->supp_drv_if_ctx;
	LOG_DBG("external_auth_ind: ssid_len=%u akm=0x%08x", ind->ssid.length, ind->akm);
	wpa_supplicant_event_wrapper(drv_if_ctx->supp_if_ctx, EVENT_EXTERNAL_AUTH, &event);

	return 0;
}

/* Deferred from _sme_auth_mgmt_rx after SAE Confirm success */
static void sae_complete_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (wl80211_glb.authenticating != 0) {
		bflb_send_external_auth_rsp(WLAN_STATUS_SUCCESS);
	}
}

/*
 * Management frame RX handler -- called by the blob for auth/assoc frames.
 *
 * The blob's full-MAC design normally handles 802.11 auth internally, but
 * SAE (WPA3) requires the host supplicant to run the SAE state machine.
 * The blob signals this via EXTERNAL_AUTH_IND, then forwards raw auth
 * frames here for the supplicant to process.
 *
 * For SAE, the flow is:
 *   1. Blob sends EXTERNAL_AUTH_IND -> _external_auth_ind()
 *   2. Supplicant sends SAE Commit via send_mlme
 *   3. AP replies with SAE Commit -> arrives here, forwarded to supplicant
 *   4. Supplicant sends SAE Confirm via send_mlme
 *   5. AP replies with SAE Confirm (txn=2, status=SUCCESS) -> arrives here
 *   6. We tell the blob auth succeeded via external_auth_rsp (deferred)
 *   7. Blob proceeds to association
 *
 * Non-SAE auth frames are forwarded directly to the supplicant's mgmt_rx.
 */
void _sme_auth_mgmt_rx(uint8_t *frame, size_t len)
{
	struct bflb_supp_ctx *ctx = &g_supp_ctx;
	struct zep_drv_if_ctx *drv_if_ctx;
	int ch;
	uint16_t freq = 0;

	if (ctx->supp_drv_if_ctx == NULL || ctx->supp_cb.mgmt_rx == NULL ||
	    len < IEEE80211_MGMT_HDR_LEN) {
		return;
	}

	ch = wl80211_glb.last_connect_params != NULL ? wl80211_sta_get_channel() : -1;
	if (ch > 0) {
		uint8_t band = (ch > 14) ? WL80211_BAND_5G : WL80211_BAND_2G4;

		freq = _channel_to_freq(band, ch);
	}

	LOG_DBG("sme_auth_mgmt_rx: len=%zu freq=%u", len, freq);

	if (wl80211_glb.authenticating && len >= IEEE80211_AUTH_MIN_LEN) {
		uint16_t auth_alg = sys_get_le16(frame + IEEE80211_AUTH_ALG_OFF);

		if (auth_alg == WLAN_AUTH_SAE) {
			uint16_t auth_txn;
			uint16_t status;

			drv_if_ctx = ctx->supp_drv_if_ctx;
			sme_external_auth_mgmt_rx((struct wpa_supplicant *)drv_if_ctx->supp_if_ctx,
						  frame, len);

			auth_txn = sys_get_le16(frame + IEEE80211_AUTH_TXN_OFF);
			status = sys_get_le16(frame + IEEE80211_AUTH_STATUS_OFF);

			if (auth_txn == SAE_AUTH_CONFIRM && status == WLAN_STATUS_SUCCESS) {
				k_work_reschedule(&sae_complete_work,
						  K_MSEC(BFLB_SAE_COMPLETE_DELAY_MS));
			}
			return;
		}
	}

	ctx->supp_cb.mgmt_rx(ctx->supp_drv_if_ctx, (char *)frame, len, freq, 0);
}

/* Ops table */

const struct zep_wpa_supp_dev_ops bflb_wpa_supp_ops = {
	.init = bflb_wpa_supp_init,
	.deinit = bflb_wpa_supp_deinit,
	.scan2 = bflb_wpa_supp_scan2,
	.scan_abort = bflb_wpa_supp_scan_abort,
	.get_scan_results2 = bflb_wpa_supp_get_scan_results,
	.authenticate = bflb_wpa_supp_authenticate,
	.associate = bflb_wpa_supp_associate,
	.deauthenticate = bflb_wpa_supp_deauthenticate,
	.set_key = bflb_wpa_supp_set_key,
	.set_supp_port = bflb_wpa_supp_set_supp_port,
	.signal_poll = bflb_wpa_supp_signal_poll,
	.get_capa = bflb_wpa_supp_get_capa,
	.get_wiphy = bflb_wpa_supp_get_wiphy,
	.set_country = bflb_wpa_supp_set_country,
	.get_country = bflb_wpa_supp_get_country,
	.send_mlme = bflb_wpa_supp_send_mlme,
	.get_conn_info = bflb_wpa_supp_get_conn_info,
	.register_frame = bflb_wpa_supp_register_frame,
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_AP
	.init_ap = bflb_wpa_supp_init_ap,
	.start_ap = bflb_wpa_supp_start_ap,
	.change_beacon = bflb_wpa_supp_change_beacon,
	.stop_ap = bflb_wpa_supp_stop_ap,
	.deinit_ap = bflb_wpa_supp_deinit_ap,
	.hapd_init = bflb_hapd_init,
	.hapd_deinit = bflb_hapd_deinit,
	.set_ap = bflb_set_ap,
	.sta_add = bflb_sta_add,
	.sta_set_flags = bflb_sta_set_flags,
	.sta_remove = bflb_sta_remove,
	.get_inact_sec = bflb_get_inact_sec,
	.register_mgmt_frame = bflb_wpa_supp_register_mgmt_frame,
#endif
};
