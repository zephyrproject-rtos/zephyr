/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi.h>

#include "mgmt_api.h"
#include "internal.h"

#include "wifi_drv_api.h"
#include "wifi_drv_priv.h"

#include "common/ieee802_11_common.h"

LOG_MODULE_DECLARE(wifi_native_sim, CONFIG_WIFI_LOG_LEVEL);

static const struct zep_wpa_supp_dev_ops *get_api(const struct device *dev)
{
	return ((const struct net_wifi_mgmt_offload *)dev->api)->wifi_drv_ops;
}

static enum wifi_frequency_bands freq_to_band(unsigned int freq)
{
	if (freq <= 2484) {
		return WIFI_FREQ_BAND_2_4_GHZ;
	}

	if (freq < 5955) {
		return WIFI_FREQ_BAND_5_GHZ;
	}

	return WIFI_FREQ_BAND_6_GHZ;
}

static uint8_t band_to_op_class(enum wifi_frequency_bands band)
{
	switch (band) {
	case WIFI_FREQ_BAND_5_GHZ:
		return 128;
	case WIFI_FREQ_BAND_6_GHZ:
		return 131;
	case WIFI_FREQ_BAND_2_4_GHZ:
	default:
		return 81;
	}
}

int wifi_scan(const struct device *dev, struct net_if *iface,
	      struct wifi_scan_params *params, scan_result_cb_t cb)
{
	struct wifi_context *ctx = dev->data;
	const struct zep_wpa_supp_dev_ops *drv = get_api(dev);
	struct wpa_driver_scan_params scan_params = {0};
	int freqs[WIFI_MGMT_SCAN_CHAN_MAX_MANUAL + 1];
	int num_freqs = 0;
	int num_ssids = 0;
	int ret;
	int i;

	ARG_UNUSED(iface);

	if (ctx->scan_in_progress) {
		LOG_DBG("Scan already in progress for %s", ctx->name);
		ret = -EBUSY;
		goto out;
	}

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	for (i = 0; params != NULL && i < MIN(WIFI_MGMT_SCAN_SSID_FILT_MAX, WPAS_MAX_SCAN_SSIDS);
	     i++) {
		if (params->ssids[i] == NULL || params->ssids[i][0] == '\0') {
			continue;
		}

		scan_params.ssids[num_ssids].ssid = (const uint8_t *)params->ssids[i];
		scan_params.ssids[num_ssids].ssid_len = strlen(params->ssids[i]);
		num_ssids++;
	}

	scan_params.num_ssids = num_ssids;

	for (i = 0; params != NULL && i < WIFI_MGMT_SCAN_CHAN_MAX_MANUAL; i++) {
		int freq;

		if (params->band_chan[i].channel == 0) {
			continue;
		}

		freq = ieee80211_chan_to_freq(NULL, band_to_op_class(params->band_chan[i].band),
					      params->band_chan[i].channel);
		if (freq <= 0) {
			continue;
		}

		freqs[num_freqs++] = freq;
	}

	if (num_freqs > 0) {
		freqs[num_freqs] = 0; /* The list is zero-terminated */
		scan_params.freqs = freqs;
	}

	/* The scan_type, dwell times and band-only filtering carried by
	 * wifi_scan_params have no equivalent in the nl80211 scan parameters
	 * used here, so they are ignored.
	 */
	if (params != NULL) {
		LOG_DBG("Ignoring scan_type %d bands 0x%02x dwell active %u passive %u",
			params->scan_type, params->bands, params->dwell_time_active,
			params->dwell_time_passive);
	}

	(void)drv->scan2(ctx, &scan_params);

	/* This scan was requested through the Wi-Fi mgmt API, not by the
	 * supplicant, so report it as external when it completes.
	 */
	ctx->external_scan = true;

	ctx->scan_cb = cb;
	ctx->scan_in_progress = true;

	ret = 0;
out:
	return ret;
}

int wifi_connect(const struct device *dev, struct net_if *iface,
		 struct wifi_connect_req_params *params)
{
	struct wifi_context *ctx = dev->data;
	int ret;

	ARG_UNUSED(iface);

	/* Initiate the connect in Linux hwsim */
	ret = wifi_drv_connect(ctx, params);
	if (ret < 0) {
		LOG_DBG("Connect failed (%d)", ret);
	}

	return ret;
}

int wifi_disconnect(const struct device *dev, struct net_if *iface)
{
	struct wifi_context *ctx = dev->data;
	int ret;

	ARG_UNUSED(iface);

	ret = wifi_drv_disconnect(ctx);
	if (ret < 0) {
		LOG_DBG("Disconnect failed (%d)", ret);
	}

	return ret;
}

int wifi_iface_status(const struct device *dev, struct net_if *iface,
		      struct wifi_iface_status *status)
{
	struct wifi_context *ctx = dev->data;
	struct host_iface_status host = {0};
	uint8_t channel = 0;
	int ret;

	ARG_UNUSED(iface);

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	if (ctx->host_context == NULL) {
		status->state = WIFI_STATE_DISCONNECTED;
		return 0;
	}

	ret = host_wifi_drv_iface_status(ctx->host_context, &host);
	if (ret < 0) {
		return ret;
	}

	/* enum wpa_states and enum wifi_iface_state use the same numbering. */
	status->state = host.wpa_state;
	status->ssid_len = MIN(host.ssid_len, sizeof(status->ssid) - 1);
	memcpy(status->ssid, host.ssid, status->ssid_len);
	memcpy(status->bssid, host.bssid, sizeof(status->bssid));
	status->rssi = host.signal;
	status->beacon_interval = host.beacon_interval;
	status->dtim_period = host.dtim_period;
	status->iface_mode = WIFI_MODE_INFRA;
	status->link_mode = WIFI_LINK_MODE_UNKNOWN;
	status->security = WIFI_SECURITY_TYPE_UNKNOWN;
	status->mfp = WIFI_MFP_UNKNOWN;

	if (host.freq != 0) {
		(void)ieee80211_freq_to_chan(host.freq, &channel);
		status->channel = channel;
		status->band = freq_to_band(host.freq);
	}

	return 0;
}

int wifi_filter(const struct device *dev, struct net_if *iface,
		struct wifi_filter_info *filter)
{
	struct wifi_context *ctx = dev->data;
	int ret;

	ARG_UNUSED(iface);

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	if (filter->oper == WIFI_MGMT_GET) {
		filter->filter = ctx->promisc_mode ? WIFI_PACKET_FILTER_ALL : 0;
		return 0;
	}

	/* Map the "all packets" filter to host interface promiscuous mode. */
	ret = wifi_promisc_mode(ctx->if_name_host, (filter->filter & WIFI_PACKET_FILTER_ALL) != 0);
	if (ret == 0) {
		ctx->promisc_mode = (filter->filter & WIFI_PACKET_FILTER_ALL) != 0;
	}

	return ret;
}

int wifi_mode(const struct device *dev, struct net_if *iface, struct wifi_mode_info *mode)
{
	struct wifi_context *ctx = dev->data;

	ARG_UNUSED(iface);

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	if (mode->oper == WIFI_MGMT_GET) {
		mode->mode = WIFI_MODE_INFRA;
		return 0;
	}

	/* Only infrastructure (station) mode is supported. */
	if (mode->mode != WIFI_MODE_INFRA) {
		return -ENOTSUP;
	}

	return 0;
}

int wifi_channel(const struct device *dev, struct net_if *iface,
		 struct wifi_channel_info *channel)
{
	struct wifi_context *ctx = dev->data;
	struct host_iface_status host = {0};
	uint8_t chan = 0;

	ARG_UNUSED(iface);

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	if (channel->oper != WIFI_MGMT_GET) {
		/* Setting the channel implies monitor / TX-injection mode
		 * which is not supported by this driver.
		 */
		return -ENOTSUP;
	}

	if (ctx->host_context == NULL ||
	    host_wifi_drv_iface_status(ctx->host_context, &host) < 0 || host.freq == 0) {
		channel->channel = 0;
		return 0;
	}

	(void)ieee80211_freq_to_chan(host.freq, &chan);
	channel->channel = chan;
	channel->band = freq_to_band(host.freq);

	return 0;
}

#ifdef CONFIG_NET_STATISTICS_WIFI
int wifi_get_stats(const struct device *dev, struct net_if *iface,
		   struct net_stats_wifi *stats)
{
	struct wifi_context *ctx = dev->data;

	ARG_UNUSED(iface);

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	/* The simulated driver does not maintain detailed Wi-Fi statistics, so
	 * just return the (currently empty) cached counters.
	 */
	memcpy(stats, &ctx->stats_wifi, sizeof(*stats));

	return 0;
}
#endif /* CONFIG_NET_STATISTICS_WIFI */

int wifi_set_power_save(const struct device *dev, struct net_if *iface,
			struct wifi_ps_params *params)
{
	struct wifi_context *ctx = dev->data;

	ARG_UNUSED(iface);
	ARG_UNUSED(params);

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	return 0;
}

int wifi_set_twt(const struct device *dev, struct net_if *iface,
		 struct wifi_twt_params *twt_params)
{
	struct wifi_context *ctx = dev->data;

	ARG_UNUSED(iface);
	ARG_UNUSED(twt_params);

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	/* Target wait time scheduling is not available over the hwsim/nl80211
	 * path used by this driver.
	 */
	return -ENOTSUP;
}

int wifi_reg_domain(const struct device *dev, struct net_if *iface,
		    struct wifi_reg_domain *reg_domain)
{
	struct wifi_context *ctx = dev->data;

	ARG_UNUSED(iface);
	ARG_UNUSED(reg_domain);

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	/* Regulatory domain management is not implemented for the simulated
	 * driver.
	 */
	return -ENOTSUP;
}

int wifi_get_power_save_config(const struct device *dev, struct net_if *iface,
			       struct wifi_ps_config *ps_config)
{
	struct wifi_context *ctx = dev->data;

	ARG_UNUSED(iface);
	ARG_UNUSED(ps_config);

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	return 0;
}
