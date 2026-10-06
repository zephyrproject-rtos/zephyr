/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Operations the Zephyr supplicant calls on the network co-processor Wi-Fi driver.
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>

#include "wifi_ncp.h"

LOG_MODULE_DECLARE(wifi_ncp, CONFIG_WIFI_LOG_LEVEL);

#define WIFI_NCP_2_4GHZ_CHANNELS 13
#define WIFI_NCP_2_4GHZ_BASE_MHZ 2407

void *wifi_ncp_supp_init(void *supp_drv_if_ctx, const char *iface_name,
			 struct zep_wpa_supp_dev_callbk_fns *callbk_fns)
{
	struct zep_drv_if_ctx *if_ctx = supp_drv_if_ctx;
	const struct device *dev = net_if_get_device(if_ctx->iface);
	struct wifi_ncp_data *data = dev->data;

	ARG_UNUSED(iface_name);

	data->supp.if_ctx = if_ctx;
	data->supp.cb = *callbk_fns;

	return (void *)dev;
}

void wifi_ncp_supp_deinit(void *if_priv)
{
	const struct device *dev = if_priv;
	struct wifi_ncp_data *data = dev->data;

	data->supp.if_ctx = NULL;
}

int wifi_ncp_supp_get_capa(void *if_priv, struct wpa_driver_capa *capa)
{
	ARG_UNUSED(if_priv);

	memset(capa, 0, sizeof(*capa));
	capa->key_mgmt = WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK |
			 WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK |
			 WPA_DRIVER_CAPA_KEY_MGMT_SAE;
	capa->enc = WPA_DRIVER_CAPA_ENC_CCMP;
	/* The co-processor authenticates and associates, the supplicant runs
	 * the SAE exchange and the EAPOL handshake.
	 */
	capa->flags = WPA_DRIVER_FLAGS_SAE | WPA_DRIVER_FLAGS_CONTROL_PORT;
	capa->max_scan_ssids = 1;

	return 0;
}

/* The radio capabilities are not reported by the co-processor yet */
int wifi_ncp_supp_get_wiphy(void *if_priv)
{
	const struct device *dev = if_priv;
	struct wifi_ncp_data *data = dev->data;
	struct wpa_supp_event_supported_band band;
	static const uint16_t bitrates[] = {
		10, 20, 55, 110, 60, 90, 120, 180, 240, 360, 480, 540
	};

	if (!data->supp.if_ctx || !data->supp.cb.get_wiphy_res) {
		return -EINVAL;
	}

	memset(&band, 0, sizeof(band));
	band.band = WIFI_FREQ_BAND_2_4_GHZ;
	band.wpa_supp_n_channels = WIFI_NCP_2_4GHZ_CHANNELS;
	for (size_t i = 0; i < WIFI_NCP_2_4GHZ_CHANNELS; i++) {
		band.channels[i].ch_valid = 1;
		band.channels[i].center_frequency = WIFI_NCP_2_4GHZ_BASE_MHZ + (i + 1) * 5;
		band.channels[i].wpa_supp_max_power = 20;
	}
	band.wpa_supp_n_bitrates = ARRAY_SIZE(bitrates);
	for (size_t i = 0; i < ARRAY_SIZE(bitrates); i++) {
		band.bitrates[i].wpa_supp_bitrate = bitrates[i];
	}

	data->supp.cb.get_wiphy_res(data->supp.if_ctx, &band);
	k_sem_give(&data->supp.if_ctx->drv_resp_sem);

	return 0;
}
