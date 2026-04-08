/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024-2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "siwx91x_nwp_api.h"
#include "siwx91x_wifi.h"

LOG_MODULE_DECLARE(siwx91x_wifi, CONFIG_WIFI_LOG_LEVEL);

int siwx91x_wifi_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	const struct siwx91x_wifi_config *config = dev->config;
	const struct siwx91x_wifi_data *data = dev->data;
	static const struct {
		enum wifi_security_type zephyr;
		int silabs;
	} security_convert[] = {
		{ WIFI_SECURITY_TYPE_NONE,       SL_WIFI_OPEN },
		{ WIFI_SECURITY_TYPE_WPA_PSK,    SL_WIFI_WPA  },
		{ WIFI_SECURITY_TYPE_PSK,        SL_WIFI_WPA2 },
		{ WIFI_SECURITY_TYPE_PSK_SHA256, SL_WIFI_WPA2 },
		{ WIFI_SECURITY_TYPE_SAE_AUTO,   SL_WIFI_WPA3 },
		{ WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL, SL_WIFI_WPA3_TRANSITION },
	};
	int channel = 0;
	int security_type, ret, i;

	if (params->band != WIFI_FREQ_BAND_UNKNOWN && params->band != WIFI_FREQ_BAND_2_4_GHZ) {
		ret = -ENOTSUP;
		goto err;
	}
	if (strlen(params->ssid) != params->ssid_length) {
		ret = -ENOTSUP;
		goto err;
	}
	if (params->psk_length &&
	    strlen(params->psk) != params->psk_length) {
		ret = -ENOTSUP;
		goto err;
	}
	if (params->sae_password_length &&
	    strlen(params->sae_password) != params->sae_password_length) {
		ret = -ENOTSUP;
		goto err;
	}
	security_type = -1;
	for (i = 0; i < ARRAY_SIZE(security_convert); i++) {
		if (security_convert[i].zephyr == params->security) {
			security_type = security_convert[i].silabs;
			break;
		}
	}
	if (i == ARRAY_SIZE(security_convert)) {
		ret = -ENOTSUP;
		goto err;
	}
	if (params->channel != WIFI_CHANNEL_ANY) {
		channel = BIT(params->channel - 1);
	}

	/* nwp_disconnect and nwp_init are only required if already connected, but it does not hurt
	 * to always apply them.
	 */
	siwx91x_nwp_disconnect(config->nwp_dev);
	siwx91x_nwp_wifi_init(config->nwp_dev);
	if (params->psk_length) {
		siwx91x_nwp_set_psk(config->nwp_dev, params->psk);
	} else if (params->sae_password_length) {
		siwx91x_nwp_set_psk(config->nwp_dev, params->sae_password);
	}
	ret = siwx91x_nwp_scan(config->nwp_dev, channel, params->ssid, false, true);
	if (ret) {
		goto join_fail;
	}
	ret = siwx91x_nwp_join(config->nwp_dev, params->ssid, params->bssid, security_type);
	if (ret) {
		goto join_fail;
	}
	if (!IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD)) {
		net_if_dormant_off(data->iface);
	}
	wifi_mgmt_raise_connect_result_event(data->iface, WIFI_STATUS_CONN_SUCCESS);
	return 0;

join_fail:
	siwx91x_nwp_wifi_init(config->nwp_dev);
err:
	wifi_mgmt_raise_connect_result_event(data->iface, WIFI_STATUS_CONN_FAIL);
	return ret;
}

int siwx91x_wifi_disconnect(const struct device *dev)
{
	const struct siwx91x_wifi_config *config = dev->config;
	const struct siwx91x_wifi_data *data = dev->data;

	siwx91x_nwp_disconnect(config->nwp_dev);
	siwx91x_nwp_wifi_init(config->nwp_dev);
	if (!IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD)) {
		net_if_dormant_on(data->iface);
	}
	wifi_mgmt_raise_disconnect_result_event(data->iface, WIFI_REASON_DISCONN_SUCCESS);

	return 0;
}
