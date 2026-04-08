/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024-2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "ieeee80211.h"
#include "siwx91x_nwp_api.h"
#include "siwx91x_wifi.h"

LOG_MODULE_DECLARE(siwx91x_wifi, CONFIG_WIFI_LOG_LEVEL);

void siwx91x_wifi_on_scan_results(const struct siwx91x_nwp_wifi_cb *ctxt, struct net_buf *buf)
{
	struct net_if *iface = net_if_get_first_wifi();
	struct siwx91x_wifi_data *data = net_if_get_device(iface)->data;
	struct siwx91x_frame_desc *frame = (struct siwx91x_frame_desc *)buf->data;
	struct ieee80211_beacon *payload = (struct ieee80211_beacon *)frame->data;
	struct wifi_scan_result tmp = {
		.mfp = WIFI_MFP_UNKNOWN,
		.band = WIFI_FREQ_BAND_2_4_GHZ,
		.mac_length = WIFI_MAC_ADDR_LEN,
		.channel = frame->reserved[7],
		.rssi = (uint8_t)~frame->reserved[6],
		.security = WIFI_SECURITY_TYPE_NONE,
	};
	struct ieee80211_element *elt;
	int ies_len = buf->len -
		      sizeof(struct siwx91x_frame_desc) -
		      sizeof(struct ieee80211_beacon);

	__ASSERT(sizeof(payload->bssid) == WIFI_MAC_ADDR_LEN, "Corrupted build");
	__ASSERT(sizeof(tmp.mac) == WIFI_MAC_ADDR_LEN, "Corrupted build");
	if (!ieee80211_is_beacon(payload->frame_control) &&
	    !ieee80211_is_probe_resp(payload->frame_control)) {
		return;
	}

	memcpy(tmp.mac, payload->bssid, WIFI_MAC_ADDR_LEN);
	tmp.mac_length = WIFI_MAC_ADDR_LEN;

	if (payload->capab_info & WLAN_CAPABILITY_PRIVACY) {
		tmp.security = WIFI_SECURITY_TYPE_WEP;
	}

	elt = (struct ieee80211_element *)payload->ies;
	while (1) {
		if (payload->ies + ies_len - (uint8_t *)elt < sizeof(struct ieee80211_element)) {
			break;
		}
		if (payload->ies + ies_len - (uint8_t *)elt <
		    sizeof(struct ieee80211_element) + elt->datalen) {
			break;
		}
		if (elt->id == WLAN_EID_SSID) {
			tmp.ssid_length = elt->datalen;
			memcpy(tmp.ssid, elt->data, tmp.ssid_length);
		}
		if (elt->id == WLAN_EID_RSN) {
			/* FIXME: parse RSN */
			tmp.security = WIFI_SECURITY_TYPE_PSK;
		}
		elt = (struct ieee80211_element *)(elt->data + elt->datalen);
	}
	if (data->zephyr_scan_cb) {
		data->zephyr_scan_cb(iface, 0, &tmp);
	}
}

int siwx91x_wifi_scan(const struct device *dev,
		      struct wifi_scan_params *params, scan_result_cb_t cb)
{
	struct net_if *iface = net_if_get_first_wifi();
	const struct siwx91x_wifi_config *config = dev->config;
	struct siwx91x_wifi_data *data = dev->data;
	uint16_t chan_mask;
	int ret;

	/* FIXME: add support or dwell interval */

	data->zephyr_scan_cb = cb;
	chan_mask = 0;
	for (int i = 0; i < ARRAY_SIZE(params->band_chan) && params->band_chan[i].channel; i++) {
		if (params->band_chan[i].band != WIFI_FREQ_BAND_2_4_GHZ) {
			continue;
		}
		chan_mask |= BIT(params->band_chan[i].channel - 1);
	}
	ret = siwx91x_nwp_scan(config->nwp_dev, chan_mask, params->ssids[0],
			       params->scan_type == WIFI_SCAN_TYPE_PASSIVE, false);
	if (data->zephyr_scan_cb) {
		data->zephyr_scan_cb(iface, ret, NULL);
	}
	return ret;
}
