/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr wpa_supplicant (hostap) driver ops for the BL60x full-MAC
 * firmware.  The firmware handles 802.11 auth/assoc internally via
 * SM_CONNECT_REQ; the supplicant runs the EAPOL 4-way handshake over the
 * data path and installs keys through the firmware key API.
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <mbedtls/platform_util.h>

#include "driver_zephyr.h"
#include "config.h"
#include "rsn_supp/wpa.h"

#include "bflb_wifi.h"
#include "bflb_wifi_scan.h"
#include "bflb_wifi_ccmp.h"
#include "bflb_wifi_wpa_supp.h"

LOG_MODULE_DECLARE(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

#define WLAN_EID_SSID 0
#define WLAN_EID_RSN  48

#define BFLB_24GHZ_BASE_FREQ   2407U
#define BFLB_24GHZ_CH14_FREQ   2484U
#define BFLB_24GHZ_CH_SPACING  5U
#define BFLB_24GHZ_CH_MAX      13U
#define BFLB_DEFAULT_MAX_DBM   20
#define BFLB_DEFAULT_BEACON_TU 100

#define BFLB_EAPOL_TX_TIMEOUT_MS 1000
/* Let the FW put EAPOL msg 4 on air before switching keys. */
#define BFLB_M4_TX_SETTLE_MS     50

#define CCMP_TK_LEN 16U

/* Firmware key/port API cipher algo ids (supplicant_api.h's enum wpa_alg
 * collides with hostap's, so the functions are declared directly below).
 */
#define BFLB_FW_ALG_CCMP 3
#define BFLB_FW_ALG_IGTK 7

extern bool bl_wifi_auth_done_internal(uint8_t sta_idx, uint16_t reason_code);
extern int bl_wifi_set_sta_key_internal(uint8_t vif_idx, uint8_t sta_idx, int alg, int key_idx,
					int set_tx, uint8_t *seq, size_t seq_len, uint8_t *key,
					size_t key_len, bool pairwise);

/* 802.11b/g basic rates in 100 kbps units. */
static const unsigned short bg_rates_100kbps[] = {
	10, 20, 55, 110, 60, 90, 120, 180, 240, 360, 480, 540,
};

struct bflb_supp_ctx g_supp_ctx;

static struct zep_wpa_supp_dev_callbk_fns supp_cb;

static void connect_work_handler(struct k_work *work);
static void *bflb_wpa_supp_init(void *supp_drv_if_ctx, const char *iface_name,
				struct zep_wpa_supp_dev_callbk_fns *callbk_fns);
static void bflb_wpa_supp_deinit(void *if_priv);
static int bflb_wpa_supp_scan2(void *if_priv, struct wpa_driver_scan_params *params);
static int bflb_wpa_supp_scan_abort(void *if_priv);
static uint16_t bflb_channel_to_freq(uint8_t ch);
static int bflb_wpa_supp_get_scan_results(void *if_priv);
static int bflb_wpa_supp_authenticate(void *if_priv, struct wpa_driver_auth_params *params,
				      struct wpa_bss *curr_bss);
static void bflb_wpa_supp_fill_passphrase(struct bflb_supp_ctx *ctx);
static int bflb_wpa_supp_associate(void *if_priv, struct wpa_driver_associate_params *params);
static int bflb_wpa_supp_deauthenticate(void *if_priv, const char *addr,
					unsigned short reason_code);
static int bflb_wpa_supp_set_key(void *if_priv, const unsigned char *ifname, enum wpa_alg alg,
				 const unsigned char *addr, int key_idx, int set_tx,
				 const unsigned char *seq, size_t seq_len, const unsigned char *key,
				 size_t key_len, enum key_flag key_flag);
static int bflb_wpa_supp_set_supp_port(void *if_priv, int authorized, char *bssid);
static int bflb_wpa_supp_signal_poll(void *if_priv, struct wpa_signal_info *si,
				     unsigned char *bssid);
static int bflb_wpa_supp_get_capa(void *if_priv, struct wpa_driver_capa *capa);
static int bflb_wpa_supp_get_wiphy(void *if_priv);
static int bflb_wpa_supp_set_country(void *priv, const char *alpha2);
static int bflb_wpa_supp_get_country(void *priv, char *alpha2);
static int bflb_wpa_supp_send_mlme(void *if_priv, const u8 *data, size_t data_len, int noack,
				   unsigned int freq, int no_cck, int offchanok,
				   unsigned int wait_time, int cookie);
static int bflb_wpa_supp_get_conn_info(void *if_priv, struct wpa_conn_info *info);
static int bflb_wpa_supp_register_frame(void *if_priv, u16 type, const u8 *match, size_t match_len,
					bool multicast);

/* Deferred connect: SM commands block on IPC confirmations, so the blob
 * connect runs on the system work queue instead of the supplicant thread.
 */
static void connect_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bflb_supp_ctx *ctx = CONTAINER_OF(dwork, struct bflb_supp_ctx, connect_work);
	union wpa_event_data event;
	struct bflb_wifi_connect_params conn;
	int ret;

	conn.ssid = ctx->conn_ssid;
	conn.ssid_len = ctx->conn_ssid_len;
	conn.bssid = ctx->conn_bssid;
	conn.channel = ctx->conn_channel;
	conn.secured = ctx->conn_secured;
	conn.sae = ctx->conn_sae;
	conn.phrase = ctx->conn_secured ? ctx->conn_phrase : NULL;

	ret = bflb_wifi_connect_req(&bflb_wifi, &conn);
	mbedtls_platform_zeroize(ctx->conn_phrase, sizeof(ctx->conn_phrase));

	if (ret != 0) {
		LOG_ERR("connect failed: %d", ret);
		if (ctx->supp_drv_if_ctx != NULL && supp_cb.assoc_resp != NULL) {
			memset(&event, 0, sizeof(event));
			event.assoc_reject.bssid = ctx->assoc_bssid;
			event.assoc_reject.status_code = WLAN_STATUS_UNSPECIFIED_FAILURE;
			supp_cb.assoc_resp(ctx->supp_drv_if_ctx, &event,
					   WLAN_STATUS_UNSPECIFIED_FAILURE);
		}
	}
}

/* zep_wpa_supp_dev_ops */

static void *bflb_wpa_supp_init(void *supp_drv_if_ctx, const char *iface_name,
				struct zep_wpa_supp_dev_callbk_fns *callbk_fns)
{
	struct bflb_supp_ctx *ctx = &g_supp_ctx;

	ARG_UNUSED(iface_name);

	ctx->wdev = &bflb_wifi;
	ctx->supp_drv_if_ctx = supp_drv_if_ctx;
	memcpy(&supp_cb, callbk_fns, sizeof(supp_cb));
	k_work_init_delayable(&ctx->connect_work, connect_work_handler);

	return ctx;
}

static void bflb_wpa_supp_deinit(void *if_priv)
{
	struct bflb_supp_ctx *ctx = if_priv;

	if (ctx != NULL) {
		mbedtls_platform_zeroize(ctx->conn_phrase, sizeof(ctx->conn_phrase));
		memset(ctx->assoc_ie, 0, sizeof(ctx->assoc_ie));
		memset(&supp_cb, 0, sizeof(supp_cb));
		ctx->supp_drv_if_ctx = NULL;
	}
}

static int bflb_wpa_supp_scan2(void *if_priv, struct wpa_driver_scan_params *params)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(params);

	return bflb_wifi_scan_start(&bflb_wifi);
}

static int bflb_wpa_supp_scan_abort(void *if_priv)
{
	ARG_UNUSED(if_priv);
	return 0;
}

static uint16_t bflb_channel_to_freq(uint8_t ch)
{
	if (ch == 14U) {
		return BFLB_24GHZ_CH14_FREQ;
	}
	return BFLB_24GHZ_BASE_FREQ + (uint16_t)ch * BFLB_24GHZ_CH_SPACING;
}

/* Convert cached scan results into wpa_scan_res structs.  The raw RSN/WPA
 * IEs cached from the beacon are passed verbatim -- hostap byte-compares
 * the RSN IE during 4-way handshake msg 3 validation.
 */
static int bflb_wpa_supp_get_scan_results(void *if_priv)
{
	struct bflb_supp_ctx *ctx = if_priv;
	uint8_t count;

	if (ctx == NULL || ctx->supp_drv_if_ctx == NULL || supp_cb.scan_res == NULL) {
		return -EINVAL;
	}

	count = bflb_wifi_scan_count();

	for (uint8_t i = 0; i < count; i++) {
		const struct bflb_scan_ap *ap = bflb_wifi_scan_entry(i);
		uint8_t res_buf[sizeof(struct wpa_scan_res) + 2 + BFLB_WIFI_SSID_MAX_LEN +
				BFLB_SCAN_RSN_IE_MAX + BFLB_SCAN_WPA_IE_MAX]
			__aligned(__alignof__(struct wpa_scan_res));
		struct wpa_scan_res *r = (struct wpa_scan_res *)res_buf;
		uint8_t *ie_pos = (uint8_t *)(r + 1);
		size_t ie_len;

		if (ap == NULL) {
			break;
		}

		memset(r, 0, sizeof(*r));

		*ie_pos++ = WLAN_EID_SSID;
		*ie_pos++ = ap->ssid_len;
		memcpy(ie_pos, ap->ssid, ap->ssid_len);
		ie_pos += ap->ssid_len;

		if (ap->rsn_ie_len > 0U) {
			memcpy(ie_pos, ap->rsn_ie, ap->rsn_ie_len);
			ie_pos += ap->rsn_ie_len;
		}
		if (ap->wpa_ie_len > 0U) {
			memcpy(ie_pos, ap->wpa_ie, ap->wpa_ie_len);
			ie_pos += ap->wpa_ie_len;
		}

		ie_len = ie_pos - (uint8_t *)(r + 1);

		memcpy(r->bssid, ap->bssid, BFLB_WIFI_MAC_ADDR_LEN);
		r->freq = bflb_channel_to_freq(ap->channel);
		r->level = ap->rssi;
		r->ie_len = ie_len;
		r->caps = ap->caps;

		supp_cb.scan_res(ctx->supp_drv_if_ctx, r, i + 1U < count);
	}

	if (count == 0U && supp_cb.scan_done != NULL) {
		union wpa_event_data event;

		memset(&event, 0, sizeof(event));
		event.scan_info.aborted = false;
		supp_cb.scan_done(ctx->supp_drv_if_ctx, &event);
	}

	return 0;
}

/* The full-MAC firmware performs 802.11 auth internally as part of the
 * connect; synthesize an immediate success so the SME proceeds to the
 * associate step, where the connect is actually kicked off.
 */
static int bflb_wpa_supp_authenticate(void *if_priv, struct wpa_driver_auth_params *params,
				      struct wpa_bss *curr_bss)
{
	struct bflb_supp_ctx *ctx = if_priv;

	ARG_UNUSED(curr_bss);

	if (ctx == NULL || params == NULL) {
		return -EINVAL;
	}

	if (supp_cb.auth_resp != NULL) {
		union wpa_event_data event;

		memset(&event, 0, sizeof(event));
		memcpy(event.auth.peer, params->bssid, ETH_ALEN);
		event.auth.auth_type = WLAN_AUTH_OPEN;
		event.auth.status_code = WLAN_STATUS_SUCCESS;
		supp_cb.auth_resp(ctx->supp_drv_if_ctx, &event);
	}

	return 0;
}

static void bflb_wpa_supp_fill_passphrase(struct bflb_supp_ctx *ctx)
{
	struct zep_drv_if_ctx *zep_ctx;
	struct wpa_supplicant *wpa_s;
	struct wpa_ssid *ssid;

	ctx->conn_phrase[0] = '\0';

	zep_ctx = ctx->supp_drv_if_ctx;
	wpa_s = zep_ctx != NULL ? zep_ctx->supp_if_ctx : NULL;
	ssid = wpa_s != NULL ? wpa_s->current_ssid : NULL;

	if (ssid != NULL) {
		const char *pw = ssid->sae_password != NULL ? ssid->sae_password : ssid->passphrase;

		if (pw != NULL) {
			size_t plen = strlen(pw);

			memcpy(ctx->conn_phrase, pw, MIN(plen, sizeof(ctx->conn_phrase) - 1));
		}
	}
}

static int bflb_wpa_supp_associate(void *if_priv, struct wpa_driver_associate_params *params)
{
	struct bflb_supp_ctx *ctx = if_priv;

	if (ctx == NULL || params == NULL) {
		return -EINVAL;
	}

	if (params->ssid == NULL || params->ssid_len == 0 ||
	    params->ssid_len > BFLB_WIFI_SSID_MAX_LEN) {
		return -EINVAL;
	}
	memcpy(ctx->conn_ssid, params->ssid, params->ssid_len);
	ctx->conn_ssid_len = params->ssid_len;

	memset(ctx->conn_bssid, 0, sizeof(ctx->conn_bssid));
	if (params->bssid != NULL) {
		memcpy(ctx->conn_bssid, params->bssid, ETH_ALEN);
		memcpy(ctx->assoc_bssid, params->bssid, ETH_ALEN);
	}

	ctx->conn_channel = 0;
	if (params->freq.freq != 0) {
		ctx->assoc_freq = params->freq.freq;
		if (params->freq.freq == BFLB_24GHZ_CH14_FREQ) {
			ctx->conn_channel = 14;
		} else if (params->freq.freq > BFLB_24GHZ_BASE_FREQ) {
			ctx->conn_channel =
				(params->freq.freq - BFLB_24GHZ_BASE_FREQ) / BFLB_24GHZ_CH_SPACING;
		} else {
			/* Out-of-band frequency: leave the channel unresolved */
		}
	}

	/* Save the supplicant's association IEs -- the wpa_funcs bridge
	 * installs them via bl_wifi_set_appie_internal during the join.
	 */
	ctx->assoc_ie_len = 0;
	if (params->wpa_ie != NULL && params->wpa_ie_len > 0 &&
	    params->wpa_ie_len <= sizeof(ctx->assoc_ie)) {
		memcpy(ctx->assoc_ie, params->wpa_ie, params->wpa_ie_len);
		ctx->assoc_ie_len = params->wpa_ie_len;
	}

	ctx->open_network = (params->key_mgmt_suite == WPA_KEY_MGMT_NONE);
	ctx->conn_secured = !ctx->open_network;
	ctx->conn_sae = (params->key_mgmt_suite & (WPA_KEY_MGMT_SAE | WPA_KEY_MGMT_FT_SAE)) != 0;
	ctx->assoc_reported = false;

	bflb_wpa_supp_fill_passphrase(ctx);

	LOG_INF("associate: ssid='%.*s' ch=%d km=0x%x secured=%d", ctx->conn_ssid_len,
		ctx->conn_ssid, ctx->conn_channel, params->key_mgmt_suite, ctx->conn_secured);

	k_work_reschedule(&ctx->connect_work, K_NO_WAIT);

	return 0;
}

static int bflb_wpa_supp_deauthenticate(void *if_priv, const char *addr, unsigned short reason_code)
{
	struct bflb_supp_ctx *ctx = if_priv;

	ARG_UNUSED(addr);
	ARG_UNUSED(reason_code);

	if (ctx != NULL) {
		k_work_cancel_delayable(&ctx->connect_work);
	}

	bflb_wifi_ccmp_clear();
	return bflb_wifi_disconnect_req(&bflb_wifi);
}

static int bflb_wpa_supp_set_key(void *if_priv, const unsigned char *ifname, enum wpa_alg alg,
				 const unsigned char *addr, int key_idx, int set_tx,
				 const unsigned char *seq, size_t seq_len, const unsigned char *key,
				 size_t key_len, enum key_flag key_flag)
{
	struct bflb_supp_ctx *ctx = if_priv;
	uint8_t rsc[8] = {0};
	bool pairwise;
	int ret;

	ARG_UNUSED(ifname);
	ARG_UNUSED(set_tx);

	if (ctx == NULL) {
		return -EINVAL;
	}

	pairwise = (key_flag & KEY_FLAG_PAIRWISE) != 0;

	if (alg == WPA_ALG_NONE) {
		if (pairwise) {
			bflb_wifi_ccmp_clear();
		}
		return 0;
	}

	/* IGTK (management-frame protection, mandatory for WPA3-SAE): install
	 * as a group key with the firmware's IGTK algorithm.
	 */
	if (alg == WPA_ALG_BIP_CMAC_128) {
		if (key == NULL || key_len != CCMP_TK_LEN) {
			return -ENOTSUP;
		}
		if (seq != NULL && seq_len > 0 && seq_len <= sizeof(rsc)) {
			memcpy(rsc, seq, seq_len);
		}
		LOG_INF("set_key: IGTK vif=%u key_idx=%d", bflb_wifi.vif_idx, key_idx);
		ret = bl_wifi_set_sta_key_internal(bflb_wifi.vif_idx, ctx->sta_idx,
						   BFLB_FW_ALG_IGTK, key_idx, 0, rsc, sizeof(rsc),
						   (uint8_t *)(uintptr_t)key, key_len, false);
		if (ret != 0) {
			LOG_ERR("IGTK install failed: %d", ret);
			return -EIO;
		}
		return 0;
	}

	/* CCMP only: the SW-encrypt TX workaround can't service TKIP/WEP. */
	if (alg != WPA_ALG_CCMP || key == NULL || key_len != CCMP_TK_LEN) {
		return -ENOTSUP;
	}

	if (seq != NULL && seq_len > 0 && seq_len <= sizeof(rsc)) {
		memcpy(rsc, seq, seq_len);
	}

	LOG_INF("set_key: %s vif=%u sta=%u key_idx=%d", pairwise ? "PTK" : "GTK", bflb_wifi.vif_idx,
		ctx->sta_idx, key_idx);

	if (pairwise) {
		/* The AP must receive EAPOL msg 4 unencrypted before the
		 * hardware switches to the new key.
		 */
		(void)bflb_wifi_wait_eapol_tx_done(K_MSEC(BFLB_EAPOL_TX_TIMEOUT_MS));
		k_msleep(BFLB_M4_TX_SETTLE_MS);

		ret = bl_wifi_set_sta_key_internal(bflb_wifi.vif_idx, ctx->sta_idx,
						   BFLB_FW_ALG_CCMP, key_idx, 1, rsc, sizeof(rsc),
						   (uint8_t *)(uintptr_t)key, key_len, true);
		if (ret != 0) {
			LOG_ERR("PTK install failed: %d", ret);
			return -EIO;
		}

		/* SW CCMP context for the TX-encrypt workaround. */
		if (bflb_wifi_ccmp_set_key(key) != 0) {
			return -EIO;
		}
	} else {
		ret = bl_wifi_set_sta_key_internal(
			bflb_wifi.vif_idx, ctx->sta_idx, BFLB_FW_ALG_CCMP, key_idx, 0, rsc,
			BFLB_WIFI_MAC_ADDR_LEN, (uint8_t *)(uintptr_t)key, key_len, false);
		if (ret != 0) {
			LOG_ERR("GTK install failed: %d", ret);
			return -EIO;
		}
	}

	return 0;
}

static int bflb_wpa_supp_set_supp_port(void *if_priv, int authorized, char *bssid)
{
	struct bflb_supp_ctx *ctx = if_priv;
	uint32_t rxcntrl;

	ARG_UNUSED(bssid);

	if (ctx == NULL) {
		return -EINVAL;
	}

	LOG_INF("set_supp_port: authorized=%d sta_idx=%u", authorized, ctx->sta_idx);

	if (authorized == 0) {
		return 0;
	}

	/* Link mode for `wifi status` -- legacy 802.11g (HT is disabled in
	 * ME_CONFIG; the BL602 BA reassembly path is unreliable).
	 */
	if (ctx->supp_drv_if_ctx != NULL) {
		struct zep_drv_if_ctx *zep_ctx = ctx->supp_drv_if_ctx;
		struct wpa_supplicant *wpa_s = zep_ctx->supp_if_ctx;

		if (wpa_s != NULL) {
			wpa_s->connection_set = 1;
			wpa_s->connection_g = 1;
		}
	}

	(void)bl_wifi_auth_done_internal(ctx->sta_idx, 0);

	(void)bflb_wifi_set_ps_mode_off(&bflb_wifi);

	/* Tolerate unencrypted frame RX during the key transition. */
	rxcntrl = sys_read32(NXMAC_RX_CNTRL_REG);
	rxcntrl |= NXMAC_RX_ACCEPT_DECRYPT_ERR;
	sys_write32(rxcntrl, NXMAC_RX_CNTRL_REG);

	return 0;
}

static int bflb_wpa_supp_signal_poll(void *if_priv, struct wpa_signal_info *si,
				     unsigned char *bssid)
{
	struct bflb_wifi_dev *d = &bflb_wifi;

	ARG_UNUSED(if_priv);

	if (si == NULL) {
		return -EINVAL;
	}

	memset(si, 0, sizeof(*si));

	if (!d->connected) {
		return -ENOLINK;
	}

	si->data.signal = d->connected_rssi;
	/* Data TX uses the fixed rate-retry chain installed by the TX
	 * descriptor wrap (first rate index 4 = 6 Mb/s legacy OFDM).
	 */
	si->data.current_tx_rate = 6000;
	if (d->connected_channel > 0U) {
		si->frequency = bflb_channel_to_freq(d->connected_channel);
	}

	if (bssid != NULL) {
		memcpy(bssid, d->connected_bssid, ETH_ALEN);
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

	/* SME with synthesized auth responses: the firmware performs the
	 * real auth/assoc (and the SAE exchange, with the host doing the
	 * crypto -- the SAE-offload flag keeps hostap's own SAE state
	 * machine out of the way while still allowing SAE networks).
	 */
	capa->flags = WPA_DRIVER_FLAGS_SME | WPA_DRIVER_FLAGS_SET_KEYS_AFTER_ASSOC_DONE;
	capa->flags2 = WPA_DRIVER_FLAGS2_SAE_OFFLOAD_STA;

	capa->enc = WPA_DRIVER_CAPA_ENC_CCMP;
	capa->key_mgmt = WPA_DRIVER_CAPA_KEY_MGMT_WPA2 | WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK |
			 WPA_DRIVER_CAPA_KEY_MGMT_SAE;

	capa->max_scan_ssids = 1;

	return 0;
}

static int bflb_wpa_supp_get_wiphy(void *if_priv)
{
	struct bflb_supp_ctx *ctx = if_priv;
	struct wpa_supp_event_supported_band band = {0};
	int i;

	if (ctx == NULL || ctx->supp_drv_if_ctx == NULL || supp_cb.get_wiphy_res == NULL) {
		return -EINVAL;
	}

	band.band = 0; /* 2.4 GHz */
	band.wpa_supp_n_channels = 0;

	for (i = 1;
	     i <= BFLB_24GHZ_CH_MAX && band.wpa_supp_n_channels < WPA_SUPP_SBAND_MAX_CHANNELS;
	     i++) {
		band.channels[band.wpa_supp_n_channels].center_frequency = bflb_channel_to_freq(i);
		band.channels[band.wpa_supp_n_channels].wpa_supp_max_power = BFLB_DEFAULT_MAX_DBM;
		band.channels[band.wpa_supp_n_channels].ch_valid = 1;
		band.wpa_supp_n_channels++;
	}

	band.wpa_supp_n_bitrates = ARRAY_SIZE(bg_rates_100kbps);
	for (i = 0; i < (int)ARRAY_SIZE(bg_rates_100kbps); i++) {
		band.bitrates[i].wpa_supp_bitrate = bg_rates_100kbps[i];
	}

	supp_cb.get_wiphy_res(ctx->supp_drv_if_ctx, &band);
	supp_cb.get_wiphy_res(ctx->supp_drv_if_ctx, NULL);

	return 0;
}

static int bflb_wpa_supp_set_country(void *priv, const char *alpha2)
{
	ARG_UNUSED(priv);
	ARG_UNUSED(alpha2);
	return 0;
}

static int bflb_wpa_supp_get_country(void *priv, char *alpha2)
{
	ARG_UNUSED(priv);

	alpha2[0] = '0';
	alpha2[1] = '0';
	return 0;
}

/* Full-MAC firmware handles management frames internally. */
static int bflb_wpa_supp_send_mlme(void *if_priv, const u8 *data, size_t data_len, int noack,
				   unsigned int freq, int no_cck, int offchanok,
				   unsigned int wait_time, int cookie)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(data);
	ARG_UNUSED(data_len);
	ARG_UNUSED(noack);
	ARG_UNUSED(freq);
	ARG_UNUSED(no_cck);
	ARG_UNUSED(offchanok);
	ARG_UNUSED(wait_time);
	ARG_UNUSED(cookie);
	return 0;
}

static int bflb_wpa_supp_get_conn_info(void *if_priv, struct wpa_conn_info *info)
{
	ARG_UNUSED(if_priv);

	if (info == NULL) {
		return -EINVAL;
	}

	memset(info, 0, sizeof(*info));
	info->beacon_interval = BFLB_DEFAULT_BEACON_TU;

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

/* Driver-to-supplicant event helpers (called from the event work queue). */

void bflb_wpa_supp_scan_done_event(void)
{
	struct bflb_supp_ctx *ctx = &g_supp_ctx;
	union wpa_event_data event;

	if (ctx->supp_drv_if_ctx == NULL || supp_cb.scan_done == NULL) {
		return;
	}

	memset(&event, 0, sizeof(event));
	event.scan_info.aborted = false;
	supp_cb.scan_done(ctx->supp_drv_if_ctx, &event);
}

/* Association reported as soon as the FW signals 802.11 assoc done (via
 * the wpa_sta_connect bridge) -- the FW won't send SM_CONNECT_IND until
 * the 4-way handshake completes, which hostap only starts after this.
 */
void bflb_wpa_supp_assoc_event(void)
{
	struct bflb_supp_ctx *ctx = &g_supp_ctx;
	static uint8_t req_ies[2 + BFLB_WIFI_SSID_MAX_LEN + BFLB_MAX_ASSOC_IE_LEN];
	union wpa_event_data event;
	size_t ies_len;

	if (ctx->supp_drv_if_ctx == NULL || supp_cb.assoc_resp == NULL || ctx->assoc_reported) {
		return;
	}

	ctx->assoc_reported = true;

	/* Without SME the supplicant matches the association to a network
	 * configuration via the assoc-request IEs -- reconstruct them.
	 */
	req_ies[0] = WLAN_EID_SSID;
	req_ies[1] = ctx->conn_ssid_len;
	memcpy(&req_ies[2], ctx->conn_ssid, ctx->conn_ssid_len);
	ies_len = 2 + ctx->conn_ssid_len;
	memcpy(&req_ies[ies_len], ctx->assoc_ie, ctx->assoc_ie_len);
	ies_len += ctx->assoc_ie_len;

	memset(&event, 0, sizeof(event));
	event.assoc_info.addr = ctx->assoc_bssid;
	event.assoc_info.freq = ctx->assoc_freq;
	event.assoc_info.req_ies = req_ies;
	event.assoc_info.req_ies_len = ies_len;
	LOG_INF("assoc success reported");
	supp_cb.assoc_resp(ctx->supp_drv_if_ctx, &event, WLAN_STATUS_SUCCESS);
}

/* SM_CONNECT_IND: final connect outcome (post-handshake on secured
 * networks, post-assoc on open ones).
 */
void bflb_wpa_supp_connect_result(uint16_t status)
{
	struct bflb_supp_ctx *ctx = &g_supp_ctx;
	union wpa_event_data event;

	LOG_INF("connect result status=%u", status);

	if (ctx->supp_drv_if_ctx == NULL || supp_cb.assoc_resp == NULL) {
		return;
	}

	if (status == WLAN_STATUS_SUCCESS) {
		bflb_wpa_supp_assoc_event();
		return;
	}

	ctx->assoc_reported = false;
	memset(&event, 0, sizeof(event));
	/* hostap deep-copies assoc_reject.bssid unconditionally. */
	event.assoc_reject.bssid = ctx->assoc_bssid;
	event.assoc_reject.status_code = status;
	supp_cb.assoc_resp(ctx->supp_drv_if_ctx, &event, status);
}

void bflb_wpa_supp_disconnected_event(uint16_t reason)
{
	struct bflb_supp_ctx *ctx = &g_supp_ctx;
	union wpa_event_data event;
	struct ieee80211_mgmt mgmt;

	ctx->assoc_reported = false;

	if (ctx->supp_drv_if_ctx == NULL || supp_cb.deauth == NULL) {
		return;
	}

	/* hostap's deauth proc reads mgmt->bssid and deauth_info.addr --
	 * synthesize a minimal deauth frame from the AP.
	 */
	memset(&mgmt, 0, sizeof(mgmt));
	memcpy(mgmt.da, bflb_wifi.mac_addr, ETH_ALEN);
	memcpy(mgmt.sa, ctx->assoc_bssid, ETH_ALEN);
	memcpy(mgmt.bssid, ctx->assoc_bssid, ETH_ALEN);
	mgmt.u.deauth.reason_code = host_to_le16(reason);

	LOG_INF("deauth to supplicant: reason=%u", reason);

	memset(&event, 0, sizeof(event));
	event.deauth_info.reason_code = reason;
	event.deauth_info.addr = ctx->assoc_bssid;
	supp_cb.deauth(ctx->supp_drv_if_ctx, &event, &mgmt);
}

/* Hand the SAE-derived PMK to the supplicant so its 4-way handshake can
 * run.  Called from the firmware task right after SAE confirm succeeds,
 * before the AP's EAPOL msg 1 reaches the supplicant thread.
 */
void bflb_wpa_supp_set_pmk(const uint8_t *pmk, size_t pmk_len, const uint8_t *pmkid)
{
	struct bflb_supp_ctx *ctx = &g_supp_ctx;
	struct zep_drv_if_ctx *zep_ctx = ctx->supp_drv_if_ctx;
	struct wpa_supplicant *wpa_s;

	wpa_s = zep_ctx != NULL ? zep_ctx->supp_if_ctx : NULL;
	if (wpa_s == NULL || wpa_s->wpa == NULL) {
		return;
	}

	LOG_INF("SAE PMK installed (len=%zu)", pmk_len);
	wpa_sm_set_pmk(wpa_s->wpa, pmk, pmk_len, pmkid, ctx->assoc_bssid);
}

/* EAPOL RX from the firmware (no Ethernet header).  Rebuild the Ethernet
 * frame and inject it into the net stack so the supplicant's l2_packet
 * layer receives it.
 */
void bflb_wifi_eapol_input(const uint8_t *src, const uint8_t *buf, uint32_t len)
{
	struct bflb_wifi_dev *d = &bflb_wifi;
	struct net_pkt *pkt;
	uint8_t eth_hdr[BFLB_WIFI_ETH_HDR_LEN];

	if (d->iface == NULL || buf == NULL || len > NET_ETH_MTU) {
		return;
	}

	memcpy(&eth_hdr[0], d->mac_addr, BFLB_WIFI_MAC_ADDR_LEN);
	memcpy(&eth_hdr[6], src, BFLB_WIFI_MAC_ADDR_LEN);
	sys_put_be16(BFLB_WIFI_ETH_TYPE_EAPOL, &eth_hdr[BFLB_WIFI_ETH_TYPE_OFF]);

	pkt = net_pkt_rx_alloc_with_buffer(d->iface, sizeof(eth_hdr) + len, AF_UNSPEC, 0,
					   K_NO_WAIT);
	if (pkt == NULL) {
		return;
	}

	if (net_pkt_write(pkt, eth_hdr, sizeof(eth_hdr)) != 0 ||
	    net_pkt_write(pkt, buf, len) != 0) {
		net_pkt_unref(pkt);
		return;
	}

	if (net_recv_data(d->iface, pkt) != 0) {
		net_pkt_unref(pkt);
	}
}

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
};
