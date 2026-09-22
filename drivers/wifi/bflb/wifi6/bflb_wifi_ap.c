/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_AP

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/queue.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "driver_zephyr.h"
#include "common/ieee802_11_defs.h"
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
#include "bflb_wifi_wpa_supp.h"
#include "bflb_wifi_ap.h"

LOG_MODULE_DECLARE(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

#include "bflb_wifi_blob_api.h"

#define BFLB_AP_MAX_STA         8
#define BFLB_DEFAULT_MAX_DBM    20
#define BFLB_DEFAULT_BEACON_TU  100
#define BFLB_DEFAULT_AP_CHANNEL 6

struct bflb_ap_sta_ctx {
	uint8_t mac[ETH_ALEN];
	uint8_t sta_idx;
	uint8_t wpa_ie[MAX_ASSOC_IE_LEN];
	uint8_t wpa_ie_len;
	bool in_use;
};

static struct bflb_ap_sta_ctx ap_sta_table[BFLB_AP_MAX_STA];

uint8_t bflb_ap_find_sta(const uint8_t *mac)
{
	int i;

	for (i = 0; i < BFLB_AP_MAX_STA; i++) {
		if (ap_sta_table[i].in_use && memcmp(ap_sta_table[i].mac, mac, ETH_ALEN) == 0) {
			return ap_sta_table[i].sta_idx;
		}
	}
	return STA_IDX_INVALID;
}

/* AP bridge callbacks (struct wpa_funcs) */

void *bflb_ap_bridge_init(wifi_ap_parm_t *parm)
{
	if (parm == NULL) {
		LOG_DBG("AP init: parm is NULL");
		return NULL;
	}

	LOG_DBG("AP init: mac=%02x:%02x:%02x:%02x:%02x:%02x auth=%d cipher=%d", parm->mac[0],
		parm->mac[1], parm->mac[2], parm->mac[3], parm->mac[4], parm->mac[5],
		parm->auth_mode, parm->pairwise_cipher);

	parm->mac[0] |= ETH_ADDR_LOCAL_ADMIN_BIT;

	memset(ap_sta_table, 0, sizeof(ap_sta_table));
	wl80211_supplicant_set_hostap_private(parm);
	return parm;
}

bool bflb_ap_bridge_deinit(void *data)
{
	LOG_DBG("AP deinit: data=%p", data);
	ARG_UNUSED(data);
	memset(ap_sta_table, 0, sizeof(ap_sta_table));
	memset(wl80211_glb.aid_list, 0, sizeof(wl80211_glb.aid_list));
	wl80211_supplicant_set_hostap_private(NULL);
	return true;
}

bool bflb_ap_bridge_join(void **sm, uint8_t *mac, uint8_t *wpa_ie, uint8_t wpa_ie_len)
{
	struct bflb_ap_sta_ctx *sta = NULL;
	int i;

	for (i = 0; i < BFLB_AP_MAX_STA; i++) {
		if (!ap_sta_table[i].in_use) {
			sta = &ap_sta_table[i];
			break;
		}
	}
	if (sta == NULL) {
		LOG_ERR("AP join: no free station slots");
		return false;
	}

	memset(sta, 0, sizeof(*sta));
	memcpy(sta->mac, mac, ETH_ALEN);
	if (wpa_ie != NULL && wpa_ie_len > 0) {
		size_t copy_len = MIN((size_t)wpa_ie_len, sizeof(sta->wpa_ie));

		memcpy(sta->wpa_ie, wpa_ie, copy_len);
		sta->wpa_ie_len = copy_len;
	}
	sta->in_use = true;

	if (sm != NULL) {
		*sm = sta;
	}

	LOG_DBG("AP join: mac=%02x:%02x:%02x:%02x:%02x:%02x ie=%u", mac[0], mac[1], mac[2], mac[3],
		mac[4], mac[5], wpa_ie_len);
	return true;
}

void bflb_ap_bridge_sta_associated(void *wpa_sm, uint8_t sta_idx)
{
	struct bflb_ap_sta_ctx *sta = wpa_sm;
	struct bflb_supp_ctx *ctx = &g_supp_ctx;
	struct zep_drv_if_ctx *drv_ctx;
	union wpa_event_data event;

	if (sta == NULL) {
		return;
	}
	sta->sta_idx = sta_idx;

	if (sta_idx < ARRAY_SIZE(wl80211_glb.aid_list)) {
		struct aid_list_entry *aid = &wl80211_glb.aid_list[sta_idx];

		memcpy(aid->mac, sta->mac, ETH_ALEN);
		aid->wpa_sm = sta;
		aid->used = 1;
		aid->sta_idx = sta_idx;
		aid->last_seq_ctrl = 0;
	}

	drv_ctx = ctx->supp_drv_if_ctx;
	if (drv_ctx == NULL || drv_ctx->supp_if_ctx == NULL) {
		return;
	}

	memset(&event, 0, sizeof(event));
	event.assoc_info.addr = sta->mac;
	event.assoc_info.req_ies = sta->wpa_ie_len > 0 ? sta->wpa_ie : NULL;
	event.assoc_info.req_ies_len = sta->wpa_ie_len;
	event.assoc_info.reassoc = 0;
	event.assoc_info.assoc_link_id = -1;

	LOG_DBG("AP sta associated: sta=%u mac=%02x:%02x:%02x:%02x:%02x:%02x ie=%u", sta_idx,
		sta->mac[0], sta->mac[1], sta->mac[2], sta->mac[3], sta->mac[4], sta->mac[5],
		sta->wpa_ie_len);

	wpa_supplicant_event_wrapper(drv_ctx->supp_if_ctx, EVENT_ASSOC, &event);
}

bool bflb_ap_bridge_remove(void *sm)
{
	struct bflb_ap_sta_ctx *sta = sm;
	struct bflb_supp_ctx *ctx = &g_supp_ctx;
	struct zep_drv_if_ctx *drv_ctx;
	union wpa_event_data event;

	if (sta == NULL || !sta->in_use) {
		return true;
	}

	LOG_DBG("AP bridge remove: sta=%u mac=%02x:%02x:%02x:%02x:%02x:%02x", sta->sta_idx,
		sta->mac[0], sta->mac[1], sta->mac[2], sta->mac[3], sta->mac[4], sta->mac[5]);

	if (sta->sta_idx < ARRAY_SIZE(wl80211_glb.aid_list)) {
		wl80211_glb.aid_list[sta->sta_idx].used = 0;
	}

	drv_ctx = ctx->supp_drv_if_ctx;
	if (drv_ctx != NULL && drv_ctx->supp_if_ctx != NULL) {
		memset(&event, 0, sizeof(event));
		event.disassoc_info.addr = sta->mac;
		event.disassoc_info.reason_code = WLAN_REASON_DISASSOC_STA_HAS_LEFT;
		wpa_supplicant_event_wrapper(drv_ctx->supp_if_ctx, EVENT_DISASSOC, &event);
	}

	sta->in_use = false;
	return true;
}

bool bflb_ap_bridge_rx_eapol(void *hapd_data, void *sm, uint8_t *data, size_t data_len)
{
	struct bflb_ap_sta_ctx *sta = sm;
	struct bflb_wifi_dev *wdev = &wl80211_dev;
	const struct net_linkaddr *link_addr;
	struct net_pkt *pkt;
	uint8_t eth_hdr[14];

	ARG_UNUSED(hapd_data);

	if (sta == NULL || data == NULL || data_len == 0 || wdev->iface == NULL) {
		return false;
	}

	link_addr = net_if_get_link_addr(wdev->iface);
	if (link_addr == NULL || link_addr->len < ETH_ALEN) {
		return false;
	}

	memcpy(eth_hdr, link_addr->addr, ETH_ALEN);
	memcpy(eth_hdr + ETH_ALEN, sta->mac, ETH_ALEN);
	sys_put_be16(NET_ETH_PTYPE_EAPOL, eth_hdr + 12);

	pkt = net_pkt_rx_alloc_with_buffer(wdev->iface, sizeof(eth_hdr) + data_len, AF_UNSPEC, 0,
					   K_NO_WAIT);
	if (pkt == NULL) {
		LOG_ERR("AP EAPOL RX: alloc failed");
		return false;
	}

	if (net_pkt_write(pkt, eth_hdr, sizeof(eth_hdr)) != 0 ||
	    net_pkt_write(pkt, data, data_len) != 0) {
		net_pkt_unref(pkt);
		return false;
	}

	LOG_DBG("AP EAPOL RX: len=%zu from %02x:%02x:%02x:%02x:%02x:%02x", data_len, sta->mac[0],
		sta->mac[1], sta->mac[2], sta->mac[3], sta->mac[4], sta->mac[5]);

	if (net_recv_data(wdev->iface, pkt) != 0) {
		net_pkt_unref(pkt);
	}

	return true;
}

/* AP driver ops */

void *bflb_hapd_init(void *hapd_drv_if_ctx, const char *iface_name,
		     struct zep_hostapd_dev_callbk_fns *callbk_fns)
{
	struct bflb_supp_ctx *ctx = &g_supp_ctx;

	ARG_UNUSED(iface_name);

	ctx->hapd_drv_if_ctx = hapd_drv_if_ctx;
	memcpy(&ctx->hapd_cb, callbk_fns, sizeof(ctx->hapd_cb));

	return ctx;
}

void bflb_hapd_deinit(void *priv)
{
	struct bflb_supp_ctx *ctx = priv;

	if (ctx != NULL) {
		memset(&ctx->hapd_cb, 0, sizeof(ctx->hapd_cb));
		ctx->hapd_drv_if_ctx = NULL;
	}
}

int bflb_set_ap(void *priv, int beacon_set, struct wpa_driver_ap_params *params)
{
	ARG_UNUSED(priv);
	ARG_UNUSED(beacon_set);
	ARG_UNUSED(params);
	return 0;
}

int bflb_sta_set_flags(void *if_priv, const u8 *addr, unsigned int total_flags,
		       unsigned int flags_or, unsigned int flags_and)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(total_flags);
	ARG_UNUSED(flags_and);

	if (addr == NULL) {
		return -EINVAL;
	}

	if (flags_or & WPA_STA_AUTHORIZED) {
		uint8_t idx = bflb_aid_list_find_sta(addr);

		if (idx == STA_IDX_INVALID) {
			idx = bflb_ap_find_sta(addr);
		}
		if (idx != STA_IDX_INVALID) {
			struct me_set_control_port_req *req;

			req = ke_msg_alloc(ME_SET_CONTROL_PORT_REQ, TASK_ME, TASK_API,
					   sizeof(*req));
			if (req != NULL) {
				req->sta_idx = idx;
				req->control_port_open = true;
				ke_msg_send(req);
			}
			return 0;
		}
	}

	return 0;
}

int bflb_sta_add(void *if_priv, struct hostapd_sta_add_params *params)
{
	struct me_sta_add_req *req;

	ARG_UNUSED(if_priv);

	if (params == NULL || params->addr == NULL) {
		return -EINVAL;
	}

	req = ke_msg_alloc(ME_STA_ADD_REQ, TASK_ME, TASK_API, sizeof(*req));
	if (req == NULL) {
		return -ENOMEM;
	}

	memset(req, 0, sizeof(*req));
	memcpy(&req->mac_addr, params->addr, sizeof(req->mac_addr));
	req->aid = params->aid;
	req->vif_idx = wl80211_mac_vif[WL80211_VIF_AP];

	if (params->supp_rates != NULL && params->supp_rates_len > 0) {
		req->rate_set.length = MIN(params->supp_rates_len, MAC_RATESET_LEN);
		memcpy(req->rate_set.array, params->supp_rates, req->rate_set.length);
	}

	if (params->capability & WLAN_CAPABILITY_SHORT_PREAMBLE) {
		req->flags |= STA_SHORT_PREAMBLE_CAPA;
	}
	if (params->ht_capabilities != NULL) {
		req->flags |= STA_HT_CAPA;
		memcpy(&req->ht_cap, params->ht_capabilities, sizeof(req->ht_cap));
	}
	if (params->vht_capabilities != NULL) {
		req->flags |= STA_VHT_CAPA;
		memcpy(&req->vht_cap, params->vht_capabilities, sizeof(req->vht_cap));
	}
	if (params->vht_opmode_enabled != 0) {
		req->flags |= STA_OPMOD_NOTIF;
		req->opmode = params->vht_opmode;
	}
	if (params->flags & WPA_STA_WMM) {
		req->flags |= STA_QOS_CAPA;
	}
	if (params->flags & WPA_STA_MFP) {
		req->flags |= STA_MFP_CAPA;
	}

	req->uapsd_queues = params->qosinfo & 0xF;
	req->max_sp_len = (params->qosinfo >> 4) & 0x3;

	ke_msg_send(req);
	return 0;
}

int bflb_sta_remove(void *if_priv, const u8 *addr)
{
	struct me_sta_del_req *req;
	uint8_t sta_idx;

	ARG_UNUSED(if_priv);

	if (addr == NULL) {
		return -EINVAL;
	}

	sta_idx = bflb_aid_list_find_sta(addr);
	if (sta_idx == STA_IDX_INVALID) {
		sta_idx = bflb_ap_find_sta(addr);
	}
	if (sta_idx == STA_IDX_INVALID) {
		return 0;
	}

	req = ke_msg_alloc(ME_STA_DEL_REQ, TASK_ME, TASK_API, sizeof(*req));
	if (req == NULL) {
		return -ENOMEM;
	}

	req->sta_idx = sta_idx;
	req->tdls_sta = false;

	ke_msg_send(req);
	return 0;
}

int bflb_get_inact_sec(void *if_priv, const u8 *addr)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(addr);
	return 0;
}

int bflb_wpa_supp_init_ap(void *if_priv, struct wpa_driver_associate_params *params)
{
	struct bflb_supp_ctx *ctx = if_priv;
	struct wl80211_ap_settings ap = {0};
	struct wpa_supplicant *wpa_s = NULL;
	struct net_linkaddr *la;
	int ret;

	if (ctx == NULL || params == NULL) {
		return -EINVAL;
	}

	if (params->ssid != NULL && params->ssid_len > 0) {
		memcpy(ap.ssid, params->ssid, MIN(params->ssid_len, sizeof(ap.ssid) - 1));
	}

	if (ctx->supp_drv_if_ctx != NULL) {
		struct zep_drv_if_ctx *zep_ctx = ctx->supp_drv_if_ctx;
		struct wpa_ssid *ssid;

		wpa_s = zep_ctx->supp_if_ctx;
		ssid = wpa_s != NULL ? wpa_s->conf->ssid : NULL;

		for (; ssid != NULL; ssid = ssid->next) {
			if (ssid->mode == WPAS_MODE_AP && ssid->passphrase != NULL) {
				size_t plen = strlen(ssid->passphrase);

				memcpy(ap.password, ssid->passphrase,
				       MIN(plen, sizeof(ap.password) - 1));

				ssid->pairwise_cipher = WPA_CIPHER_CCMP;
				ssid->group_cipher = WPA_CIPHER_CCMP;
				ssid->ieee80211w = NO_MGMT_FRAME_PROTECTION;
				ssid->ht = 0;
				break;
			}
		}
	}

	ap.channel_width = WL80211_CHAN_WIDTH_20;
	ap.beacon_interval = params->beacon_int != 0 ? params->beacon_int : BFLB_DEFAULT_BEACON_TU;
	ap.max_power = BFLB_DEFAULT_MAX_DBM;
	ap.auth_type = WL80211_AUTHTYPE_OPEN_SYSTEM;

	ap.flags = CONTROL_PORT_HOST;
	if (ap.password[0] != '\0') {
		ap.flags |= WPA_WPA2_IN_USE;
		ctx->wdev->ap_security = WIFI_SECURITY_TYPE_PSK;
	} else {
		ap.flags |= CONTROL_PORT_NO_ENC;
		ctx->wdev->ap_security = WIFI_SECURITY_TYPE_NONE;
	}

	if (params->freq.freq != 0) {
		ap.center_freq1 = params->freq.freq;
	} else {
		ap.center_freq1 = wl80211_channel_to_freq(BFLB_DEFAULT_AP_CHANNEL);
	}

	if (wl80211_ap_status() != 0) {
		wl80211_ap_stop();
	}

	ret = wl80211_ap_start(&ap);
	if (ret != 0) {
		LOG_ERR("wl80211_ap_start failed: %d", ret);
		return -EIO;
	}

	if (wpa_s != NULL) {
		wpa_s->own_addr[0] |= ETH_ADDR_LOCAL_ADMIN_BIT;
	}

	la = net_if_get_link_addr(ctx->wdev->iface);

	if (la != NULL && la->len >= ETH_ALEN) {
		la->addr[0] |= ETH_ADDR_LOCAL_ADMIN_BIT;
	}

	LOG_DBG("AP started: ssid='%s' freq=%u flags=0x%x", ap.ssid, ap.center_freq1, ap.flags);
	return 0;
}

int bflb_wpa_supp_deinit_ap(void *if_priv)
{
	struct bflb_supp_ctx *ctx = if_priv;
	struct net_linkaddr *la;
	int ret;

	if (ctx == NULL) {
		return -EINVAL;
	}

	ret = wl80211_ap_stop();
	if (ret != 0) {
		LOG_ERR("wl80211_ap_stop failed: %d", ret);
		return -EIO;
	}

	if (ctx->supp_drv_if_ctx != NULL) {
		struct zep_drv_if_ctx *zep_ctx = ctx->supp_drv_if_ctx;
		struct wpa_supplicant *wpa_s = zep_ctx->supp_if_ctx;

		if (wpa_s != NULL) {
			wpa_s->own_addr[0] &= ~ETH_ADDR_LOCAL_ADMIN_BIT;
		}
	}

	la = net_if_get_link_addr(ctx->wdev->iface);

	if (la != NULL && la->len >= ETH_ALEN) {
		la->addr[0] &= ~ETH_ADDR_LOCAL_ADMIN_BIT;
	}

	ctx->wdev->ap_enabled = false;
	net_if_carrier_off(ctx->wdev->iface);

	LOG_DBG("AP stopped");
	return 0;
}

int bflb_wpa_supp_register_mgmt_frame(void *if_priv, u16 frame_type, size_t match_len,
				      const u8 *match)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(frame_type);
	ARG_UNUSED(match_len);
	ARG_UNUSED(match);
	return 0;
}

int bflb_wpa_supp_start_ap(void *if_priv, struct wpa_driver_ap_params *params)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(params);
	return 0;
}

int bflb_wpa_supp_stop_ap(void *if_priv)
{
	return bflb_wpa_supp_deinit_ap(if_priv);
}

int bflb_wpa_supp_change_beacon(void *if_priv, struct wpa_driver_ap_params *params)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(params);
	return 0;
}

#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_AP */
