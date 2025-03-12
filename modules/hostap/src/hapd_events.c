/**
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "includes.h"
#include "common.h"
#include "ap/sta_info.h"
#include "ap/ieee802_11.h"
#include "ap/hostapd.h"
#include "wpa_supplicant_i.h"
#include <zephyr/net/wifi_mgmt.h>
#include "hapd_events.h"
#include "supp_events.h"

static enum wifi_link_mode hapd_get_sta_link_mode(struct hostapd_iface *iface,
						  struct sta_info *sta)
{
	if (sta->flags & WLAN_STA_HE) {
		return WIFI_6;
	} else if (sta->flags & WLAN_STA_VHT) {
		return WIFI_5;
	} else if (sta->flags & WLAN_STA_HT) {
		return WIFI_4;
	} else if ((sta->flags & WLAN_STA_NONERP) ||
		   (iface->current_mode->mode == HOSTAPD_MODE_IEEE80211B)) {
		return WIFI_1;
	} else if (iface->freq > 4000) {
		return WIFI_2;
	} else if (iface->freq > 2000) {
		return WIFI_3;
	} else {
		return WIFI_LINK_MODE_UNKNOWN;
	}
}

static bool hapd_is_twt_capable(struct hostapd_iface *iface, struct sta_info *sta)
{
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_11AX
	return hostapd_get_he_twt_responder(iface->bss[0], IEEE80211_MODE_AP);
#else
	return false;
#endif
}

int hostapd_send_wifi_mgmt_ap_status(struct hostapd_iface *iface,
				     enum net_event_wifi_cmd event,
				     enum wifi_ap_status ap_status)
{
	char *ifname = iface->conf->bss[0]->iface;
	int status = ap_status;

	return supplicant_send_wifi_mgmt_event(ifname,
					       event,
					       (void *)&status,
					       sizeof(int));
}

int hostapd_send_wifi_mgmt_ap_sta_event(struct hostapd_iface *ap_ctx,
					enum net_event_wifi_cmd event,
					void *data)
{
	struct sta_info *sta = data;
	char *ifname = ap_ctx->bss[0]->conf->iface;
	struct wifi_ap_sta_info sta_info = { 0 };

	if (!ap_ctx || !sta) {
		return -EINVAL;
	}

	memcpy(sta_info.mac, sta->addr, sizeof(sta_info.mac));

	if (event == NET_EVENT_WIFI_CMD_AP_STA_CONNECTED) {
		sta_info.link_mode = hapd_get_sta_link_mode(ap_ctx, sta);
		sta_info.twt_capable = hapd_is_twt_capable(ap_ctx, sta);
	}

	return supplicant_send_wifi_mgmt_event(ifname,
					       event,
					       (void *)&sta_info,
					       sizeof(sta_info));
}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP
void hostapd_handle_dpp_event(void *ctx, char *buf, size_t len)
{
	struct hostapd_data *hapd = (struct hostapd_data *)ctx;

	if (hapd == NULL) {
		return;
	}

	struct hostapd_bss_config *conf = hapd->conf;

	if (conf == NULL || !(conf->wpa_key_mgmt & WPA_KEY_MGMT_DPP)) {
		return;
	}

	/* check hostapd */
	if (!strncmp(buf, DPP_EVENT_CONNECTOR, sizeof(DPP_EVENT_CONNECTOR) - 1)) {
		if (conf->dpp_connector) {
			os_free(conf->dpp_connector);
		}

		conf->dpp_connector = os_strdup(buf + sizeof(DPP_EVENT_CONNECTOR) - 1);
	} else if (!strncmp(buf, DPP_EVENT_C_SIGN_KEY, sizeof(DPP_EVENT_C_SIGN_KEY) - 1)) {
		if (conf->dpp_csign) {
			wpabuf_free(conf->dpp_csign);
		}

		conf->dpp_csign = wpabuf_parse_bin(buf + sizeof(DPP_EVENT_C_SIGN_KEY) - 1);
	} else if (!strncmp(buf, DPP_EVENT_NET_ACCESS_KEY, sizeof(DPP_EVENT_NET_ACCESS_KEY) - 1)) {
		if (conf->dpp_netaccesskey) {
			wpabuf_free(conf->dpp_netaccesskey);
		}

		conf->dpp_netaccesskey =
			wpabuf_parse_bin(buf + sizeof(DPP_EVENT_NET_ACCESS_KEY) - 1);
	}
}
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP */
