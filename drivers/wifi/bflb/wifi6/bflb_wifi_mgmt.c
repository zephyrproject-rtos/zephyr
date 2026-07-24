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
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/logging/log.h>

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

#include "bflb_wifi.h"
#include "bflb_wifi_scan.h"
#include "bflb_wifi_mgmt.h"

LOG_MODULE_DECLARE(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

#define WIFI_2G4_CH14 14
#define DFS_CH_START  52
#define DFS_CH_END    144

#if defined(CONFIG_NET_STATISTICS_WIFI)
static int bflb_wifi_get_stats(const struct device *dev, struct net_if *iface,
			       struct net_stats_wifi *stats)
{
	struct bflb_wifi_dev *wdev = dev->data;

	ARG_UNUSED(iface);

	if (wdev == NULL || stats == NULL) {
		return -EINVAL;
	}

	*stats = wdev->stats;
	return 0;
}

static int bflb_wifi_reset_stats(const struct device *dev, struct net_if *iface)
{
	struct bflb_wifi_dev *wdev = dev->data;

	ARG_UNUSED(iface);

	if (wdev == NULL) {
		return -EINVAL;
	}

	memset(&wdev->stats, 0, sizeof(wdev->stats));
	return 0;
}
#endif

static void bflb_wifi_monitor_rx_cb(void *ctx, void *pkt, size_t len, size_t mac_hdr_len, int rssi)
{
	struct bflb_wifi_dev *wdev = ctx;
	struct net_pkt *npkt;

	ARG_UNUSED(mac_hdr_len);
	ARG_UNUSED(rssi);

	if (wdev == NULL || wdev->iface == NULL || pkt == NULL || len == 0) {
		return;
	}

	npkt = net_pkt_rx_alloc_with_buffer(wdev->iface, len, AF_UNSPEC, 0, K_NO_WAIT);
	if (npkt == NULL) {
		return;
	}

	if (net_pkt_write(npkt, pkt, len) != 0) {
		net_pkt_unref(npkt);
		return;
	}

	if (net_recv_data(wdev->iface, npkt) != 0) {
		net_pkt_unref(npkt);
	}
}

static int bflb_wifi_mode(const struct device *dev, struct net_if *iface,
			  struct wifi_mode_info *mode)
{
	struct bflb_wifi_dev *wdev = dev->data;

	ARG_UNUSED(iface);

	if (mode == NULL) {
		return -EINVAL;
	}

	if (mode->oper == WIFI_MGMT_GET) {
		mode->mode = WIFI_STA_MODE;
		if (wdev->ap_enabled) {
			mode->mode |= WIFI_AP_MODE;
		}
		if (wdev->monitor_enabled) {
			mode->mode |= WIFI_MONITOR_MODE;
		}
		return 0;
	}

	if (mode->mode & WIFI_MONITOR_MODE) {
		if (!wdev->monitor_enabled) {
			struct wl80211_monitor_settings mon = {0};
			int ch = wl80211_sta_get_channel();

			if (ch <= 0) {
				ch = 1;
			}
			mon.center_freq1 = _channel_to_freq(WL80211_BAND_2G4, ch);
			mon.channel_width = WL80211_CHAN_WIDTH_20;
			mon.recv = bflb_wifi_monitor_rx_cb;
			mon.recv_ctx = wdev;
			if (wl80211_monitor_start(&mon) != 0) {
				return -EIO;
			}
			wdev->monitor_enabled = true;
		}
	} else {
		if (wdev->monitor_enabled) {
			wl80211_monitor_stop();
			wdev->monitor_enabled = false;
		}
	}

	return 0;
}

static int bflb_wifi_filter(const struct device *dev, struct net_if *iface,
			    struct wifi_filter_info *filter)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	if (filter == NULL) {
		return -EINVAL;
	}

	if (filter->oper == WIFI_MGMT_GET) {
		filter->filter = 0;
		return 0;
	}

	return 0;
}

static int bflb_wifi_channel(const struct device *dev, struct net_if *iface,
			     struct wifi_channel_info *channel)
{
	struct bflb_wifi_dev *wdev = dev->data;

	ARG_UNUSED(iface);

	if (channel == NULL) {
		return -EINVAL;
	}

	if (channel->oper == WIFI_MGMT_GET) {
		int ch = wl80211_sta_get_channel();

		if (ch > 0) {
			channel->channel = ch;
			channel->band = (ch <= 14) ? WIFI_FREQ_BAND_2_4_GHZ : WIFI_FREQ_BAND_5_GHZ;
		}
		return 0;
	}

	if (wdev->monitor_enabled) {
		struct wl80211_monitor_settings mon = {0};
		uint8_t band = (channel->band == WIFI_FREQ_BAND_5_GHZ) ? WL80211_BAND_5G
								       : WL80211_BAND_2G4;

		mon.center_freq1 = _channel_to_freq(band, channel->channel);
		mon.channel_width = WL80211_CHAN_WIDTH_20;
		mon.recv = bflb_wifi_monitor_rx_cb;
		mon.recv_ctx = wdev;

		wl80211_monitor_stop();
		if (wl80211_monitor_start(&mon) != 0) {
			wdev->monitor_enabled = false;
			return -EIO;
		}
	}

	return 0;
}

static enum wifi_security_type map_auth_type(uint8_t auth_type, bool has_password)
{
	if (auth_type == WL80211_AUTHTYPE_SAE) {
		return WIFI_SECURITY_TYPE_SAE;
	}
	if (has_password) {
		return WIFI_SECURITY_TYPE_PSK;
	}
	return WIFI_SECURITY_TYPE_NONE;
}

static enum wifi_mfp_options map_mfp(uint8_t mfp)
{
	switch (mfp) {
	case WL80211_MFP_REQUIRED:
		return WIFI_MFP_REQUIRED;
	case WL80211_MFP_OPTIONAL:
		return WIFI_MFP_OPTIONAL;
	default:
		return WIFI_MFP_DISABLE;
	}
}

static float bflb_wifi_current_tx_rate_mbps(enum wifi_link_mode link_mode)
{
	uint8_t mcs = export_stats_get_tx_mcs();
	uint16_t r100kbps = 0;

	switch (link_mode) {
	case WIFI_6:
		if (mcs < ARRAY_SIZE(HE_RATE_KBPS_BW20)) {
			r100kbps = HE_RATE_KBPS_BW20[mcs];
		}
		break;
	case WIFI_5:
		if (mcs < ARRAY_SIZE(VHT_RATE_KBPS_BW20)) {
			r100kbps = VHT_RATE_KBPS_BW20[mcs];
		}
		break;
	case WIFI_4:
		if (mcs < ARRAY_SIZE(HT_RATE_KBPS_BW20)) {
			r100kbps = HT_RATE_KBPS_BW20[mcs];
		}
		break;
	case WIFI_3:
		if (mcs < ARRAY_SIZE(NONHT_OFDM_KBPS)) {
			r100kbps = NONHT_OFDM_KBPS[mcs];
		}
		break;
	default:
		break;
	}
	return (float)r100kbps / 10.0f;
}

static enum wifi_link_mode bflb_wifi_link_mode_get(void)
{
	const char *fmt = export_stats_get_tx_format();

	if (fmt == NULL || fmt[0] == '\0') {
		return WIFI_LINK_MODE_UNKNOWN;
	}
	if (fmt[0] == 'H' && fmt[1] == 'E') {
		return WIFI_6;
	}
	if (fmt[0] == 'V') {
		return WIFI_5;
	}
	if (fmt[0] == 'H' && fmt[1] == 'T') {
		return WIFI_4;
	}
	if (fmt[0] == 'N') {
		return WIFI_3;
	}
	return WIFI_LINK_MODE_UNKNOWN;
}

static void fill_chan_info(struct wifi_reg_chan_info *info, uint16_t freq, uint8_t channel,
			   bool is_5g)
{
	info->center_frequency = freq;
	info->max_power = 0;
	info->supported = 1;
	info->passive_only = (!is_5g && channel == WIFI_2G4_CH14) ? 1 : 0;
	info->dfs = (is_5g && channel >= DFS_CH_START && channel <= DFS_CH_END) ? 1 : 0;
}

static int bflb_wifi_status(const struct device *dev, struct net_if *iface,
			    struct wifi_iface_status *status)
{
	char ssid[WIFI_SSID_MAX_LEN + 1];
	struct mac_addr mac_bssid;
	struct wl80211_connect_params *cp;
	int8_t rssi;
	uint16_t aid;
	size_t ssid_len;
	void *vif;
	int ch;

	ARG_UNUSED(dev);

	memset(status, 0, sizeof(*status));

	if (wl80211_ap_status() != 0) {
		status->state = WIFI_STATE_COMPLETED;
		status->iface_mode = WIFI_MODE_AP;

		memcpy(status->ssid, wl80211_glb.ap_ssid,
		       MIN(strlen((const char *)wl80211_glb.ap_ssid), (size_t)WIFI_SSID_MAX_LEN));
		status->ssid_len = strlen((const char *)wl80211_glb.ap_ssid);
		if (status->ssid_len > WIFI_SSID_MAX_LEN) {
			status->ssid_len = WIFI_SSID_MAX_LEN;
		}

		if (wl80211_glb.ap_chan_freq > 0) {
			status->channel = wl80211_freq_to_channel(wl80211_glb.ap_chan_freq);
			status->band = (status->channel <= 14) ? WIFI_FREQ_BAND_2_4_GHZ
							       : WIFI_FREQ_BAND_5_GHZ;
		}
		status->beacon_interval = wl80211_glb.ap_beacon_interval;

		memcpy(status->bssid, net_if_get_link_addr(iface)->addr, WIFI_MAC_ADDR_LEN);

		status->security = wl80211_dev.ap_security;

		return 0;
	}

	if (wl80211_sta_is_connected() == 0) {
		status->state = WIFI_STATE_DISCONNECTED;
		return 0;
	}

	status->state = WIFI_STATE_COMPLETED;
	status->iface_mode = WIFI_MODE_INFRA;

	if (wl80211_sta_get_ssid(ssid) == 0) {
		ssid_len = MIN(strlen(ssid), (size_t)WIFI_SSID_MAX_LEN);
		memcpy(status->ssid, ssid, ssid_len);
		status->ssid_len = ssid_len;
	}

	ch = wl80211_sta_get_channel();
	if (ch > 0) {
		status->channel = ch;
		status->band = (ch <= 14) ? WIFI_FREQ_BAND_2_4_GHZ : WIFI_FREQ_BAND_5_GHZ;
	} else {
		status->band = WIFI_FREQ_BAND_UNKNOWN;
	}

	status->link_mode = bflb_wifi_link_mode_get();
	status->twt_capable = (status->link_mode == WIFI_6);
	status->current_phy_tx_rate = bflb_wifi_current_tx_rate_mbps(status->link_mode);

	vif = vif_info_get_vif(WL80211_VIF_STA);
	if (vif != NULL) {
		mac_vif_get_sta_status(vif, &mac_bssid, &aid, &rssi);
		memcpy(status->bssid, &mac_bssid, WIFI_MAC_ADDR_LEN);
		status->rssi = rssi;
		status->beacon_interval = mac_vif_get_bcn_int(vif);
	}

	cp = wl80211_glb.last_connect_params;
	if (cp != NULL) {
		status->security = map_auth_type(cp->auth_type, cp->password[0] != '\0');
		status->mfp = map_mfp(cp->mfp);
	}

	return 0;
}

static int bflb_wifi_reg_domain(const struct device *dev, struct net_if *iface,
				struct wifi_reg_domain *reg_domain)
{
	const struct ieee80211_dot_d *country;
	unsigned int written;
	char code[WIFI_COUNTRY_CODE_LEN + 1];
	int i;

	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	if (reg_domain == NULL) {
		return -EINVAL;
	}

	if (reg_domain->oper == WIFI_MGMT_SET) {
		memcpy(code, reg_domain->country_code, WIFI_COUNTRY_CODE_LEN);
		code[WIFI_COUNTRY_CODE_LEN] = '\0';
		if (wl80211_set_country_code(code) != 0) {
			return -EINVAL;
		}
		return 0;
	}

	if (reg_domain->oper != WIFI_MGMT_GET) {
		return -EINVAL;
	}

	country = wl80211_get_country();
	if (country == NULL) {
		return -ENOENT;
	}

	memcpy(reg_domain->country_code, country->code, WIFI_COUNTRY_CODE_LEN);

	if (reg_domain->chan_info == NULL) {
		reg_domain->num_channels = country->channel24G_num + country->channel5G_num;
		return 0;
	}

	written = 0;
	for (i = 0; i < country->channel24G_num; i++) {
		uint8_t ch = country->channel24G_chan[i];

		fill_chan_info(&reg_domain->chan_info[written++],
			       _channel_to_freq(WL80211_BAND_2G4, ch), ch, false);
	}
	for (i = 0; i < country->channel5G_num; i++) {
		uint8_t ch = country->channel5G_chan[i];

		fill_chan_info(&reg_domain->chan_info[written++],
			       _channel_to_freq(WL80211_BAND_5G, ch), ch, true);
	}
	reg_domain->num_channels = written;

	return 0;
}

static int bflb_wifi_get_version(const struct device *dev, struct net_if *iface,
				 struct wifi_version *params)
{
	static char phy_ver[24];
	uint32_t v1 = 0, v2 = 0;

	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	if (params == NULL) {
		return -EINVAL;
	}

	phy_get_version(&v1, &v2);
	snprintk(phy_ver, sizeof(phy_ver), "phy:%08x/%08x", v1, v2);
	params->drv_version = phy_ver;
	params->fw_version = wl_get_version();
	return 0;
}

static int bflb_wifi_set_power_save(const struct device *dev, struct net_if *iface,
				    struct wifi_ps_params *params)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	if (params == NULL) {
		return -EINVAL;
	}

	if (params->type == WIFI_PS_PARAM_STATE) {
		int ret = wl80211_mac_set_ps_mode(params->enabled == WIFI_PS_ENABLED ? 1 : 0);

		if (ret != 0) {
			params->fail_reason = WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
			return -EIO;
		}
	}

	return 0;
}

const struct wifi_mgmt_ops bflb_wifi_mgmt_ops = {
	.scan = bflb_wifi_scan,
	.iface_status = bflb_wifi_status,
#if defined(CONFIG_NET_STATISTICS_WIFI)
	.get_stats = bflb_wifi_get_stats,
	.reset_stats = bflb_wifi_reset_stats,
#endif
	.reg_domain = bflb_wifi_reg_domain,
	.get_version = bflb_wifi_get_version,
	.set_power_save = bflb_wifi_set_power_save,
	.mode = bflb_wifi_mode,
	.filter = bflb_wifi_filter,
	.channel = bflb_wifi_channel,
};
