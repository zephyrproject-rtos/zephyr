/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/logging/log.h>

#include "bflb_wifi.h"
#include "bflb_wifi_scan.h"
#include "bflb_wifi_mgmt.h"

LOG_MODULE_DECLARE(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

static int bflb_wifi_status(const struct device *dev, struct net_if *iface,
			    struct wifi_iface_status *status);

static int bflb_wifi_status(const struct device *dev, struct net_if *iface,
			    struct wifi_iface_status *status)
{
	struct bflb_wifi_dev *d = dev->data;

	ARG_UNUSED(iface);

	memset(status, 0, sizeof(*status));

	status->iface_mode = WIFI_MODE_INFRA;
	status->band = WIFI_FREQ_BAND_2_4_GHZ;
	status->link_mode = WIFI_3;
	status->mfp = WIFI_MFP_DISABLE;

	if (!d->connected) {
		status->state = WIFI_STATE_DISCONNECTED;
		return 0;
	}

	status->state = WIFI_STATE_COMPLETED;
	status->security = d->connected_security;
	status->rssi = d->connected_rssi;
	memcpy(status->bssid, d->connected_bssid, BFLB_WIFI_MAC_ADDR_LEN);
	memcpy(status->ssid, d->connected_ssid, d->connected_ssid_len);
	status->ssid_len = d->connected_ssid_len;
	status->channel = d->connected_channel;

	return 0;
}

const struct wifi_mgmt_ops bflb_wifi_mgmt_ops = {
	.scan = bflb_wifi_scan,
	.iface_status = bflb_wifi_status,
};
