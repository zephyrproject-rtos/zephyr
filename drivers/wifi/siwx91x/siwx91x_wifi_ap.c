/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024-2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <nwp.h>
#include "siwx91x_wifi.h"

#include "sl_rsi_utility.h"
#include "sl_net.h"

LOG_MODULE_DECLARE(siwx91x_wifi);

static int siwx91x_nwp_reboot_if_required(const struct device *dev, uint8_t oper_mode)
{
	struct siwx91x_dev *sidev = dev->data;
	int ret;

	if (sidev->reboot_needed) {
		ret = siwx91x_nwp_mode_switch(oper_mode, sidev->hidden_ssid, sidev->max_num_sta);
		if (ret < 0) {
			LOG_ERR("Failed to reboot the device: %d", ret);
			return ret;
		}

		sidev->reboot_needed = false;
	}

	return 0;
}

static int siwx91x_set_hidden_ssid(const struct device *dev, bool enable)
{
	struct siwx91x_dev *sidev = dev->data;

	if (sidev->hidden_ssid != enable) {
		sidev->hidden_ssid = enable;
		sidev->reboot_needed = true;
	}

	return 0;
}

static int siwx91x_set_max_sta(const struct device *dev, uint8_t max_sta)
{
	struct siwx91x_dev *sidev = dev->data;

	if (sidev->max_num_sta != max_sta) {
		sidev->max_num_sta = max_sta;
		sidev->reboot_needed = true;
	}

	return 0;
}

static int siwx91x_map_ap_security(enum wifi_security_type security)
{
	switch (security) {
	case WIFI_SECURITY_TYPE_NONE:
		return SL_WIFI_OPEN;
	case WIFI_SECURITY_TYPE_WPA_PSK:
		return SL_WIFI_WPA;
	case WIFI_SECURITY_TYPE_PSK:
		return SL_WIFI_WPA2;
	case WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL:
		return SL_WIFI_WPA_WPA2_MIXED;
	default:
		return -EINVAL;
	}
}

int siwx91x_ap_disable(const struct device *dev)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	struct siwx91x_dev *sidev = dev->data;
	int ret;

	ret = sl_wifi_stop_ap(interface);
	if (ret) {
		LOG_ERR("Failed to disable Wi-Fi AP mode: 0x%x", ret);
		wifi_mgmt_raise_ap_disable_result_event(sidev->iface, WIFI_STATUS_AP_FAIL);
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE)) {
		net_if_dormant_on(sidev->iface);
	}
	wifi_mgmt_raise_ap_disable_result_event(sidev->iface, WIFI_STATUS_AP_SUCCESS);
	sidev->state = WIFI_STATE_INTERFACE_DISABLED;
	return 0;
}

static int siwx91x_ap_disable_if_required(const struct device *dev,
					  struct wifi_connect_req_params *new_params)
{
	struct wifi_iface_status prev_params = { };
	uint32_t prev_psk_length = WIFI_PSK_MAX_LEN;
	struct siwx91x_dev *sidev = dev->data;
	uint8_t prev_psk[WIFI_PSK_MAX_LEN];
	sl_net_credential_type_t psk_type;
	int ret;

	ret = siwx91x_status(dev, &prev_params);
	if (ret < 0) {
		return ret;
	}

	if (sidev->reboot_needed || siwx91x_param_changed(&prev_params, new_params)) {
		return siwx91x_ap_disable(dev);
	}

	if (new_params->security != WIFI_SECURITY_TYPE_NONE) {
		ret = sl_net_get_credential(SL_NET_DEFAULT_WIFI_AP_CREDENTIAL_ID, &psk_type,
					    prev_psk, &prev_psk_length);
		if (ret < 0) {
			LOG_ERR("Failed to get credentials: 0x%x", ret);
			return -EIO;
		}

		if (new_params->psk_length != prev_psk_length ||
		    memcmp(new_params->psk, prev_psk, prev_psk_length) != 0) {
			return siwx91x_ap_disable(dev);
		}
	}

	LOG_ERR("Device already in active state");
	return -EALREADY;
}

int siwx91x_ap_enable(const struct device *dev, struct wifi_connect_req_params *params)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	struct siwx91x_dev *sidev = dev->data;
	sl_wifi_ap_configuration_t siwx91x_ap_cfg = {
		.credential_id       = SL_NET_DEFAULT_WIFI_AP_CREDENTIAL_ID,
		.keepalive_type      = SL_SI91X_AP_NULL_BASED_KEEP_ALIVE,
		.rate_protocol       = SL_WIFI_RATE_PROTOCOL_AUTO,
		.encryption          = SL_WIFI_DEFAULT_ENCRYPTION,
		.channel.bandwidth   = SL_WIFI_BANDWIDTH_20MHz,
		.maximum_clients     = sidev->max_num_sta,
		.tdi_flags           = SL_WIFI_TDI_NONE,
		.client_idle_timeout = 0xFF,
		.beacon_interval     = 100,
		.dtim_beacon_count   = 3,
		.beacon_stop         = 0,
		.options             = 0,
		.is_11n_enabled      = 1,
	};
	sl_wifi_ap_configuration_t saved_ap_cfg;
	int ret;

	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_AP_INTERFACE) {
		LOG_ERR("Interface not in AP mode");
		wifi_mgmt_raise_ap_enable_result_event(sidev->iface,
						       WIFI_STATUS_AP_OP_NOT_PERMITTED);
		return -EINVAL;
	}

	/* Device hiddes both length and ssid at same time */
	siwx91x_set_hidden_ssid(dev, params->ignore_broadcast_ssid);
	if (sidev->state == WIFI_STATE_COMPLETED) {
		ret = siwx91x_ap_disable_if_required(dev, params);
		if (ret < 0) {
			wifi_mgmt_raise_ap_enable_result_event(sidev->iface, WIFI_STATUS_AP_FAIL);
			return ret;
		}
	}

	if (params->band != WIFI_FREQ_BAND_UNKNOWN && params->band != WIFI_FREQ_BAND_2_4_GHZ) {
		LOG_ERR("Unsupported band");
		wifi_mgmt_raise_ap_enable_result_event(sidev->iface,
						       WIFI_STATUS_AP_OP_NOT_SUPPORTED);
		return -ENOTSUP;
	}

	if (params->bandwidth != WIFI_FREQ_BANDWIDTH_20MHZ) {
		LOG_ERR("Unsupported bandwidth");
		wifi_mgmt_raise_ap_enable_result_event(sidev->iface,
						       WIFI_STATUS_AP_OP_NOT_SUPPORTED);
		return -ENOTSUP;
	}

	if (params->ssid_length == 0 || params->ssid_length > WIFI_SSID_MAX_LEN) {
		LOG_ERR("Invalid ssid length");
		wifi_mgmt_raise_ap_enable_result_event(sidev->iface,
						       WIFI_STATUS_AP_SSID_NOT_ALLOWED);
		return -EINVAL;
	}

	ret = siwx91x_map_ap_security(params->security);
	if (ret < 0) {
		LOG_ERR("Invalid security type");
		wifi_mgmt_raise_ap_enable_result_event(sidev->iface,
						       WIFI_STATUS_AP_AUTH_TYPE_NOT_SUPPORTED);
		return -EINVAL;
	}
	siwx91x_ap_cfg.security = ret;

	if (params->security != WIFI_SECURITY_TYPE_NONE) {
		ret = sl_net_set_credential(siwx91x_ap_cfg.credential_id, SL_NET_WIFI_PSK,
					    params->psk, params->psk_length);
		if (ret) {
			LOG_ERR("Failed to set credentials: 0x%x", ret);
			wifi_mgmt_raise_ap_enable_result_event(sidev->iface, WIFI_STATUS_AP_FAIL);
			return -EINVAL;
		}
	}

	ret = siwx91x_nwp_reboot_if_required(dev, WIFI_SOFTAP_MODE);
	if (ret < 0) {
		wifi_mgmt_raise_ap_enable_result_event(sidev->iface, WIFI_STATUS_AP_FAIL);
		return ret;
	}

	sli_get_saved_ap_configuration(&saved_ap_cfg);
	if (saved_ap_cfg.client_idle_timeout != 0) {
		siwx91x_ap_cfg.client_idle_timeout = saved_ap_cfg.client_idle_timeout;
	}

	siwx91x_ap_cfg.ssid.length = params->ssid_length;
	strncpy(siwx91x_ap_cfg.ssid.value, params->ssid, params->ssid_length);
	if (params->mfp != WIFI_MFP_DISABLE) {
		LOG_WRN("Needed MFP disable but got MFP %s, hence setting to MFP disable",
			wifi_mfp_txt(params->mfp));
	}

	if (params->channel == WIFI_CHANNEL_ANY) {
		siwx91x_ap_cfg.channel.channel = SL_WIFI_AUTO_CHANNEL;
	} else {
		siwx91x_ap_cfg.channel.channel = params->channel;
	}

	if (params->channel == 14) {
		/* Disable HT on channel 14 */
		siwx91x_ap_cfg.is_11n_enabled = 0;
	}

	ret = sl_wifi_start_ap(SL_WIFI_AP_INTERFACE | SL_WIFI_2_4GHZ_INTERFACE, &siwx91x_ap_cfg);
	if (ret) {
		LOG_ERR("Failed to enable AP mode: 0x%x", ret);
		wifi_mgmt_raise_ap_enable_result_event(sidev->iface, WIFI_STATUS_AP_FAIL);
		return -EIO;
	}
	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE)) {
		net_if_dormant_off(sidev->iface);
	}

	wifi_mgmt_raise_ap_enable_result_event(sidev->iface, WIFI_STATUS_AP_SUCCESS);
	sidev->state = WIFI_STATE_COMPLETED;

	return 0;
}

int siwx91x_ap_sta_disconnect(const struct device *dev, const uint8_t *mac_addr)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	sl_mac_address_t mac = { };
	int ret;

	ARG_UNUSED(dev);
	__ASSERT(mac_addr, "mac_addr cannot be NULL");

	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_AP_INTERFACE) {
		LOG_ERR("Interface not in AP mode");
		return -EINVAL;
	}

	memcpy(mac.octet, mac_addr, WIFI_MAC_ADDR_LEN);

	ret = sl_wifi_disconnect_ap_client(SL_WIFI_AP_INTERFACE | SL_WIFI_2_4GHZ_INTERFACE,
					   &mac, SL_WIFI_DEAUTH);
	if (ret) {
		LOG_ERR("Failed	to disconnect: 0x%x", ret);
		return -EIO;
	}

	return ret;
}

sl_status_t siwx91x_on_ap_sta_connect(sl_wifi_event_t event, void *data,
				      uint32_t data_length, void *arg)
{
	struct siwx91x_dev *sidev = arg;
	struct wifi_ap_sta_info sta_info = { };

	ARG_UNUSED(data_length);
	ARG_UNUSED(event);
	__ASSERT(data, "data cannot be NULL");
	__ASSERT(arg, "arg cannot be NULL");

	memcpy(sta_info.mac, data, WIFI_MAC_ADDR_LEN);
	sta_info.mac_length = WIFI_MAC_ADDR_LEN;
	sta_info.link_mode = WIFI_LINK_MODE_UNKNOWN;

	wifi_mgmt_raise_ap_sta_connected_event(sidev->iface, &sta_info);

	return SL_STATUS_OK;
}

sl_status_t siwx91x_on_ap_sta_disconnect(sl_wifi_event_t event, void *data,
					 uint32_t data_length, void *arg)
{
	struct siwx91x_dev *sidev = arg;
	struct wifi_ap_sta_info sta_info = { };

	ARG_UNUSED(data_length);
	ARG_UNUSED(event);
	__ASSERT(data, "data cannot be NULL");
	__ASSERT(arg, "arg cannot be NULL");

	memcpy(sta_info.mac, data, WIFI_MAC_ADDR_LEN);
	sta_info.mac_length = WIFI_MAC_ADDR_LEN;
	wifi_mgmt_raise_ap_sta_disconnected_event(sidev->iface, &sta_info);

	return SL_STATUS_OK;
}

int siwx91x_ap_config_params(const struct device *dev, struct wifi_ap_config_params *params)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	sl_wifi_ap_configuration_t siwx91x_ap_cfg;

	__ASSERT(params, "params cannot be NULL");

	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_AP_INTERFACE) {
		LOG_ERR("Wi-Fi not in AP mode");
		return -EINVAL;
	}

	if ((params->type & WIFI_AP_CONFIG_PARAM_BANDWIDTH) &&
	    params->bandwidth != WIFI_FREQ_BANDWIDTH_20MHZ) {
		return -ENOTSUP;
	}

	sli_get_saved_ap_configuration(&siwx91x_ap_cfg);
	siwx91x_ap_cfg.channel.bandwidth = SL_WIFI_BANDWIDTH_20MHz;
	if (params->type & WIFI_AP_CONFIG_PARAM_MAX_INACTIVITY) {
		siwx91x_ap_cfg.client_idle_timeout = params->max_inactivity * 1000;
	}

	if (params->type & WIFI_AP_CONFIG_PARAM_MAX_NUM_STA) {
		siwx91x_set_max_sta(dev, params->max_num_sta);
	}

	sli_save_ap_configuration(&siwx91x_ap_cfg);

	return 0;
}
