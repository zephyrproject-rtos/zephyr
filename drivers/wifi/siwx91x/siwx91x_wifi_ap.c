/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024-2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "siwx91x_nwp.h"
#include "siwx91x_nwp_api.h"
#include "siwx91x_wifi.h"
#include "siwx91x_wifi_ap.h"

#define SIWX91X_AP_BEACON_INTERVAL_MS 100
#define SIWX91X_AP_DTIM_INTERVAL 3
#define SIWX91X_AP_MAX_STA 4

LOG_MODULE_DECLARE(siwx91x_wifi, CONFIG_WIFI_LOG_LEVEL);

void siwx91x_on_ap_sta_connect(const struct siwx91x_nwp_wifi_cb *ctxt,
			       uint8_t remote_addr[WIFI_MAC_ADDR_LEN])
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_ap_sta_info sta_info = {
		.mac_length = WIFI_MAC_ADDR_LEN,
		.link_mode = WIFI_LINK_MODE_UNKNOWN,
	};

	memcpy(sta_info.mac, remote_addr, WIFI_MAC_ADDR_LEN);
	wifi_mgmt_raise_ap_sta_connected_event(iface, &sta_info);
}

void siwx91x_on_ap_sta_disconnect(const struct siwx91x_nwp_wifi_cb *ctxt,
				  uint8_t remote_addr[WIFI_MAC_ADDR_LEN])
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_ap_sta_info sta_info = {
		.mac_length = WIFI_MAC_ADDR_LEN
	};

	memcpy(sta_info.mac, remote_addr, WIFI_MAC_ADDR_LEN);
	wifi_mgmt_raise_ap_sta_disconnected_event(iface, &sta_info);
}

int siwx91x_ap_sta_disconnect(const struct device *dev, const uint8_t *remote_addr)
{
	const struct siwx91x_wifi_config *config = dev->config;

	siwx91x_nwp_sta_disconnect(config->nwp_dev, remote_addr);
	return 0;
}

int siwx91x_ap_disable(const struct device *dev)
{
	const struct siwx91x_wifi_config *config = dev->config;
	struct siwx91x_wifi_data *data = dev->data;

	siwx91x_nwp_ap_stop(config->nwp_dev);
	if (!IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD)) {
		net_if_dormant_on(data->iface);
	}
	wifi_mgmt_raise_ap_disable_result_event(data->iface, WIFI_STATUS_AP_SUCCESS);
	return 0;
}

#define SIWX91X_AP_CONFIG_KEEP_ALIVE_METHOD 0x03
#define SIWX91X_AP_CONFIG_HIDE_SSID         0x08
int siwx91x_ap_enable(const struct device *dev, struct wifi_connect_req_params *params)
{
	const struct siwx91x_wifi_config *config = dev->config;
	struct siwx91x_wifi_data *data = dev->data;
	sli_wifi_ap_config_request ap_config = {
		/* FIXME: make these configurable */
		.beacon_interval = SIWX91X_AP_BEACON_INTERVAL_MS,
		.dtim_period = SIWX91X_AP_DTIM_INTERVAL,
		.options = FIELD_PREP(SIWX91X_AP_CONFIG_KEEP_ALIVE_METHOD,
				      SL_WIFI_AP_NULL_BASED_KEEP_ALIVE),
		.max_sta_support = data->ap_max_num_sta,
		.ap_keepalive_period = data->ap_idle_timeout,
	};
	int ret;
	int result;

	__ASSERT(params, "params cannot be NULL");
	 /* FIXME: Support Hidden SSID */
	__ASSERT(!params->ignore_broadcast_ssid, "Not supported");

	if (params->band != WIFI_FREQ_BAND_UNKNOWN && params->band != WIFI_FREQ_BAND_2_4_GHZ) {
		result = WIFI_STATUS_AP_OP_NOT_SUPPORTED;
		ret = -ENOTSUP;
		goto err;
	}

	if (params->bandwidth != WIFI_FREQ_BANDWIDTH_20MHZ) {
		result = WIFI_STATUS_AP_OP_NOT_SUPPORTED;
		ret = -ENOTSUP;
		goto err;
	}

	if (params->psk_length != strlen(params->psk)) {
		result = WIFI_STATUS_AP_AUTH_TYPE_NOT_SUPPORTED;
		ret = -EINVAL;
		goto err;
	}
	strcpy(ap_config.psk, params->psk);

	/* FIXME: Is params->ssid always NULL terminated ? */
	if (params->ssid_length == 0 || params->ssid_length != strlen(params->ssid)) {
		result = WIFI_STATUS_AP_SSID_NOT_ALLOWED;
		ret = -EINVAL;
		goto err;
	}
	strcpy(ap_config.ssid, params->ssid);

	if (params->channel == WIFI_CHANNEL_ANY) {
		ap_config.channel = SL_WIFI_AUTO_CHANNEL;
	} else {
		ap_config.channel = params->channel;
	}

	if (params->ignore_broadcast_ssid) {
		ap_config.options |= SIWX91X_AP_CONFIG_HIDE_SSID;
	}

	 /* FIXME: Provide support for other authentication scheemes. MFP
	  * requires additionnal steps (and require to be enabled in opermode)
	  */
	switch (params->security) {
	case WIFI_SECURITY_TYPE_NONE:
		ap_config.security_type = SL_WIFI_OPEN;
		ap_config.encryption_mode = SLI_WIFI_NO_ENCRYPTION;
		break;
	case WIFI_SECURITY_TYPE_PSK_SHA256:
		ap_config.security_type = SL_WIFI_WPA2;
		ap_config.encryption_mode = SLI_WIFI_CCMP_ENCRYPTION;
		break;
	default:
		result = WIFI_STATUS_AP_AUTH_TYPE_NOT_SUPPORTED;
		ret = -EINVAL;
		goto err;
	}

	/* FIXME: Should we switch automatically between AP and sta? */
	if (data->operating_mode != WIFI_SOFTAP_MODE) {
		result = WIFI_STATUS_AP_FAIL;
		ret = -EINVAL;
		goto err;
	}

	siwx91x_nwp_ap_config(config->nwp_dev, &ap_config);
	/* FIXME: Check if Zephyr provide a way to configure/limit ht-caps */
	siwx91x_nwp_set_ht_caps(config->nwp_dev, true);
	/* FIXME: Why ssid and security_type is passed to siwx91x_nwp_ap_config() and to
	 * siwx91x_nwp_ap_start()?
	 */
	ret = siwx91x_nwp_ap_start(config->nwp_dev, ap_config.ssid, ap_config.security_type);
	if (ret) {
		result = WIFI_STATUS_AP_FAIL;
		goto err;
	}
	if (!IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD)) {
		net_if_dormant_off(data->iface);
	}
	result = WIFI_STATUS_AP_SUCCESS;

err:
	wifi_mgmt_raise_ap_enable_result_event(data->iface, result);
	return ret;
}

int siwx91x_ap_config_params(const struct device *dev, struct wifi_ap_config_params *params)
{
	struct siwx91x_wifi_data *data = dev->data;

	__ASSERT(params, "params cannot be NULL");
	if (params->type & WIFI_AP_CONFIG_PARAM_BANDWIDTH &&
	    params->bandwidth != WIFI_FREQ_BANDWIDTH_20MHZ) {
		return -ENOTSUP;
	}

	if (params->type & WIFI_AP_CONFIG_PARAM_MAX_NUM_STA) {
		if (params->max_num_sta > SIWX91X_AP_MAX_STA) {
			return -ENOTSUP;
		}
		data->ap_max_num_sta = params->max_num_sta;
	}

	if (params->type & WIFI_AP_CONFIG_PARAM_MAX_INACTIVITY) {
		/*
		 * Firmware requires idle timeout as a count of 32-beacon intervals (~3.2s). The
		 * result has to fit on uint8_t (roughtly 816s).
		 */
		data->ap_idle_timeout = DIV_ROUND_UP(params->max_inactivity * MSEC_PER_SEC,
						     SIWX91X_AP_BEACON_INTERVAL_MS * 32);
		if (data->ap_idle_timeout > UINT8_MAX) {
			LOG_INF("clamping \"max_inactivity\" parameter");
		}
		data->ap_idle_timeout = MIN(data->ap_idle_timeout, UINT8_MAX);
	}

	return 0;
}
