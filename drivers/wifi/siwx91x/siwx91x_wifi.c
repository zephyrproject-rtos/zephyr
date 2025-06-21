/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT silabs_siwx91x_wifi

#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/version.h>

#include <nwp.h>
#include "siwx91x_wifi.h"
#include "siwx91x_wifi_socket.h"

#include "sl_rsi_utility.h"
#include "sl_net_constants.h"
#include "sl_wifi_types.h"
#include "sl_wifi_callback_framework.h"
#include "sl_net_default_values.h"
#include "sl_wifi.h"
#include "sl_net.h"
#include "sl_wifi_constants.h"

#define SIWX91X_DRIVER_VERSION KERNEL_VERSION_STRING
#define SIWX91X_DEFAULT_PASSIVE_SCAN_DWELL_TIME 400

LOG_MODULE_REGISTER(siwx91x_wifi);

NET_BUF_POOL_FIXED_DEFINE(siwx91x_tx_pool, 1, _NET_ETH_MAX_FRAME_SIZE, 0, NULL);

enum {
	REQUEST_TWT = 0,
	SUGGEST_TWT = 1,
	DEMAND_TWT = 2,
};

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

enum {
	STATE_IDLE = 0x00,
	/* Failover Roam */
	STATE_BEACON_LOSS = 0x10,
	/* AP induced Roam/Deauth from supplicant */
	STATE_DEAUTHENTICATION = 0x20,
	STATE_CURRENT_AP_BEST = 0x50,
	/* While roaming */
	STATE_BETTER_AP_FOUND = 0x60,
	STATE_NO_AP_FOUND = 0x70,
	STATE_ASSOCIATED = 0x80,
	STATE_UNASSOCIATED = 0x90
};

enum {
	WLAN_REASON_NO_REASON = 0x00,
	WLAN_REASON_AUTH_DENIAL,
	WLAN_REASON_ASSOC_DENIAL,
	WLAN_REASON_AP_NOT_PRESENT,
	WLAN_REASON_UNKNOWN,
	WLAN_REASON_HANDSHAKE_FAILURE,
	WLAN_REASON_USER_DEAUTH,
	WLAN_REASON_PSK_NOT_CONFIGURED,
	WLAN_REASON_AP_INITIATED_DEAUTH,
	WLAN_REASON_ROAMING_DISABLED,
	WLAN_REASON_MAX
};

static const char *siwx91x_get_reason_string(uint8_t reason_code)
{
	static const struct {
		uint8_t code;
		const char *str;
	} reason_strings[] = {
		{ WLAN_REASON_NO_REASON,            "No reason specified" },
		{ WLAN_REASON_AUTH_DENIAL,          "Authentication denial" },
		{ WLAN_REASON_ASSOC_DENIAL,         "Association denial" },
		{ WLAN_REASON_AP_NOT_PRESENT,       "AP not present" },
		{ WLAN_REASON_UNKNOWN,              "Unknown" },
		{ WLAN_REASON_HANDSHAKE_FAILURE,    "Four way handshake failure" },
		{ WLAN_REASON_USER_DEAUTH,          "Deauthentication from User" },
		{ WLAN_REASON_PSK_NOT_CONFIGURED,   "PSK not configured" },
		{ WLAN_REASON_AP_INITIATED_DEAUTH,
		 "De-authentication (AP induced Roam/Deauth from supplicant)" },
		{ WLAN_REASON_ROAMING_DISABLED,     "Roaming not enabled" },
	};

	ARRAY_FOR_EACH(reason_strings, i) {
		if (reason_strings[i].code == reason_code) {
			return reason_strings[i].str;
		}
	}

	return "Unknown";
}

static sl_status_t siwx91x_wifi_module_stats_event_handler(sl_wifi_event_t event, void *response,
							   uint32_t result_length, void *arg)
{
	ARG_UNUSED(event);
	ARG_UNUSED(result_length);
	sl_si91x_module_state_stats_response_t *notif = response;
	uint8_t module_state = notif->state_code & 0xF0;
	struct siwx91x_dev *sidev = arg;
	const char *reason_str;

	reason_str = siwx91x_get_reason_string(notif->reason_code);

	switch (module_state) {
	case STATE_BEACON_LOSS:
		LOG_WRN("Beacon Loss");
		break;
	case STATE_BETTER_AP_FOUND:
		LOG_DBG("Better AP found while roaming");
		break;
	case STATE_ASSOCIATED:
		sidev->state = WIFI_STATE_COMPLETED;
		break;
	case STATE_UNASSOCIATED:
		wifi_mgmt_raise_disconnect_result_event(sidev->iface,
							WIFI_REASON_DISCONN_SUCCESS);

		sidev->state = WIFI_STATE_DISCONNECTED;
		break;
	default:
		return 0;
	}

	LOG_DBG("Reason: %s", reason_str);
	return 0;
}

static int siwx91x_sl_to_z_mode(sl_wifi_interface_t interface)
{
	switch (interface) {
	case SL_WIFI_CLIENT_INTERFACE:
		return WIFI_STA_MODE;
	case SL_WIFI_AP_INTERFACE:
		return WIFI_SOFTAP_MODE;
	default:
		return -EIO;
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

static enum wifi_mfp_options siwx91x_set_sta_mfp_option(sl_wifi_security_t security,
							enum wifi_mfp_options mfp_conf)
{
	uint8_t join_config;

	switch (security) {
	case SL_WIFI_OPEN:
	case SL_WIFI_WPA:
		return WIFI_MFP_DISABLE;
	case SL_WIFI_WPA2:
	case SL_WIFI_WPA_WPA2_MIXED:
		if (mfp_conf == WIFI_MFP_REQUIRED) {
			/* Handling the case for WPA2_SHA256 security type */
			/* Directly enabling the MFP Required bit in the Join Feature
			 * bitmap. This ensures that MFP is enforced for connections using
			 * WPA2_SHA256.
			 *
			 * Note: This is a workaround to configure MFP as the current SDK
			 * does not provide a dedicated API to configure MFP settings.
			 * By manipulating the join feature bitmap directly, we achieve
			 * the desired MFP configuration for enhanced security.
			 *
			 * This case will be updated in the future when the SDK adds
			 * dedicated support for configuring MFP.
			 */
			sl_si91x_get_join_configuration(SL_WIFI_CLIENT_INTERFACE, &join_config);
			join_config |= SL_SI91X_JOIN_FEAT_MFP_CAPABLE_REQUIRED;
			sl_si91x_set_join_configuration(SL_WIFI_CLIENT_INTERFACE, join_config);
			return WIFI_MFP_REQUIRED;
		}
		/* Handling the case for WPA2 security type */
		/* Ensuring the connection happened in WPA2-PSK
		 * by clearing the MFP Required bit in the Join Feature bitmap.
		 */
		sl_si91x_get_join_configuration(SL_WIFI_CLIENT_INTERFACE, &join_config);
		join_config &= ~(SL_SI91X_JOIN_FEAT_MFP_CAPABLE_REQUIRED);
		sl_si91x_set_join_configuration(SL_WIFI_CLIENT_INTERFACE, join_config);
		return WIFI_MFP_OPTIONAL;
	case SL_WIFI_WPA3:
		return WIFI_MFP_REQUIRED;
	case SL_WIFI_WPA3_TRANSITION:
		return WIFI_MFP_OPTIONAL;
	default:
		return WIFI_MFP_DISABLE;
	}

	return WIFI_MFP_UNKNOWN;
}

static int siwx91x_get_connected_ap_beacon_interval_ms(void)
{
	sl_wifi_operational_statistics_t sl_stat;
	sl_wifi_interface_t interface;
	int status;

	interface = sl_wifi_get_default_interface();
	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_CLIENT_INTERFACE) {
		return 0;
	}

	status = sl_wifi_get_operational_statistics(SL_WIFI_CLIENT_INTERFACE, &sl_stat);
	if (status) {
		return 0;
	}

	return sys_get_le16(sl_stat.beacon_interval) * 1024 / 1000;
}

static int siwx91x_apply_power_save(struct siwx91x_dev *sidev)
{
	sl_wifi_performance_profile_t sl_ps_profile;
	sl_wifi_interface_t interface;
	int beacon_interval;
	int status;

	interface = sl_wifi_get_default_interface();
	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_CLIENT_INTERFACE) {
		LOG_ERR("Wi-Fi not in station mode");
		return -EINVAL;
	}

	if (sidev->state == WIFI_STATE_INTERFACE_DISABLED) {
		LOG_ERR("Command given in invalid state");
		return -EINVAL;
	}

	sl_wifi_get_performance_profile(&sl_ps_profile);

	if (sidev->ps_params.enabled == WIFI_PS_DISABLED) {
		sl_ps_profile.profile = HIGH_PERFORMANCE;
		goto out;
	}
	if (sidev->ps_params.exit_strategy == WIFI_PS_EXIT_EVERY_TIM) {
		sl_ps_profile.profile = ASSOCIATED_POWER_SAVE_LOW_LATENCY;
	} else if (sidev->ps_params.exit_strategy == WIFI_PS_EXIT_CUSTOM_ALGO) {
		sl_ps_profile.profile = ASSOCIATED_POWER_SAVE;
	} else {
		/* Already sanitized by siwx91x_set_power_save() */
		return -EINVAL;
	}

	sl_ps_profile.monitor_interval = sidev->ps_params.timeout_ms;

	beacon_interval = siwx91x_get_connected_ap_beacon_interval_ms();
	/* 1000ms is arbitrary sane value */
	sl_ps_profile.listen_interval = MIN(beacon_interval * sidev->ps_params.listen_interval,
					    1000);

	if (sidev->ps_params.wakeup_mode == WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL &&
	    !sidev->ps_params.listen_interval) {
		LOG_INF("Disabling listen interval based wakeup until connection establishes");
	}
	if (sidev->ps_params.wakeup_mode == WIFI_PS_WAKEUP_MODE_DTIM ||
	    !sidev->ps_params.listen_interval) {
		sl_ps_profile.dtim_aligned_type = 1;
	} else if (sidev->ps_params.wakeup_mode == WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL) {
		sl_ps_profile.dtim_aligned_type = 0;
	} else {
		/* Already sanitized by siwx91x_set_power_save() */
		return -EINVAL;
	}

out:
	status = sl_wifi_set_performance_profile(&sl_ps_profile);
	return status ? -EIO : 0;
}

static int siwx91x_set_power_save(const struct device *dev, struct wifi_ps_params *params)
{
	struct siwx91x_dev *sidev = dev->data;
	int status;

	__ASSERT(params, "params cannot be NULL");

	switch (params->type) {
	case WIFI_PS_PARAM_STATE:
		sidev->ps_params.enabled = params->enabled;
		break;
	case WIFI_PS_PARAM_MODE:
		if (params->mode != WIFI_PS_MODE_LEGACY) {
			params->fail_reason = WIFI_PS_PARAM_FAIL_OPERATION_NOT_SUPPORTED;
			return -ENOTSUP;
		}
		break;
	case WIFI_PS_PARAM_LISTEN_INTERVAL:
		sidev->ps_params.listen_interval = params->listen_interval;
		break;
	case WIFI_PS_PARAM_WAKEUP_MODE:
		if (params->wakeup_mode != WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL &&
		    params->wakeup_mode != WIFI_PS_WAKEUP_MODE_DTIM) {
			params->fail_reason = WIFI_PS_PARAM_FAIL_OPERATION_NOT_SUPPORTED;
			return -ENOTSUP;
		}
		sidev->ps_params.wakeup_mode = params->wakeup_mode;
		break;
	case WIFI_PS_PARAM_TIMEOUT:
		/* 1000ms is arbitrary sane value */
		if (params->timeout_ms < SLI_DEFAULT_MONITOR_INTERVAL ||
		    params->timeout_ms > 1000) {
			params->fail_reason = WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
			return -EINVAL;
		}
		sidev->ps_params.timeout_ms = params->timeout_ms;
		break;
	case WIFI_PS_PARAM_EXIT_STRATEGY:
		if (params->exit_strategy != WIFI_PS_EXIT_EVERY_TIM &&
		    params->exit_strategy != WIFI_PS_EXIT_CUSTOM_ALGO) {
			params->fail_reason = WIFI_PS_PARAM_FAIL_OPERATION_NOT_SUPPORTED;
			return -ENOTSUP;
		}
		sidev->ps_params.exit_strategy = params->exit_strategy;
		break;
	default:
		params->fail_reason = WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
		return -EINVAL;
	}
	status = siwx91x_apply_power_save(sidev);
	if (status) {
		params->fail_reason = WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
		return status;
	}
	return 0;
}

static int siwx91x_get_power_save_config(const struct device *dev, struct wifi_ps_config *config)
{
	sl_wifi_performance_profile_t sl_ps_profile;
	struct siwx91x_dev *sidev = dev->data;
	sl_wifi_interface_t interface;
	uint16_t beacon_interval;
	sl_status_t status;

	__ASSERT(config, "config cannot be NULL");

	interface = sl_wifi_get_default_interface();
	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_CLIENT_INTERFACE) {
		LOG_ERR("Wi-Fi not in station mode");
		return -EINVAL;
	}

	if (sidev->state == WIFI_STATE_INTERFACE_DISABLED) {
		LOG_ERR("Command given in invalid state");
		return -EINVAL;
	}

	status = sl_wifi_get_performance_profile(&sl_ps_profile);
	if (status != SL_STATUS_OK) {
		LOG_ERR("Failed to get power save profile: 0x%x", status);
		return -EIO;
	}

	switch (sl_ps_profile.profile) {
	case HIGH_PERFORMANCE:
		config->ps_params.enabled = WIFI_PS_DISABLED;
		break;
	case ASSOCIATED_POWER_SAVE_LOW_LATENCY:
		config->ps_params.enabled = WIFI_PS_ENABLED;
		config->ps_params.exit_strategy = WIFI_PS_EXIT_EVERY_TIM;
		break;
	case ASSOCIATED_POWER_SAVE:
		config->ps_params.enabled = WIFI_PS_ENABLED;
		config->ps_params.exit_strategy = WIFI_PS_EXIT_CUSTOM_ALGO;
		break;
	default:
		break;
	}

	if (sl_ps_profile.dtim_aligned_type) {
		config->ps_params.wakeup_mode = WIFI_PS_WAKEUP_MODE_DTIM;
	} else {
		config->ps_params.wakeup_mode = WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL;

		beacon_interval = siwx91x_get_connected_ap_beacon_interval_ms();
		if (beacon_interval > 0) {
			config->ps_params.listen_interval =
				sl_ps_profile.listen_interval / beacon_interval;
		}
	}

	/* Device supports only legacy power-save mode */
	config->ps_params.mode = WIFI_PS_MODE_LEGACY;
	config->ps_params.timeout_ms = sl_ps_profile.monitor_interval;

	return 0;
}

static unsigned int siwx91x_on_join(sl_wifi_event_t event,
				    char *result, uint32_t result_size, void *arg)
{
	struct siwx91x_dev *sidev = arg;

	if (*result != 'C') {
		/* TODO: report the real reason of failure */
		wifi_mgmt_raise_connect_result_event(sidev->iface, WIFI_STATUS_CONN_FAIL);
		sidev->state = WIFI_STATE_INACTIVE;
		return 0;
	}

	wifi_mgmt_raise_connect_result_event(sidev->iface, WIFI_STATUS_CONN_SUCCESS);
	sidev->state = WIFI_STATE_COMPLETED;

	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE)) {
		net_if_dormant_off(sidev->iface);
	}

	siwx91x_on_join_ipv4(sidev);
	siwx91x_on_join_ipv6(sidev);

	siwx91x_apply_power_save(sidev);

	return 0;
}

static int siwx91x_status(const struct device *dev, struct wifi_iface_status *status)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	sl_si91x_rsp_wireless_info_t wlan_info = { };
	struct siwx91x_dev *sidev = dev->data;
	uint8_t join_config;
	int32_t rssi;
	int ret;

	__ASSERT(status, "status cannot be NULL");

	memset(status, 0, sizeof(*status));

	status->state = sidev->state;
	if (sidev->state <= WIFI_STATE_INACTIVE) {
		return 0;
	}

	ret = sl_wifi_get_wireless_info(&wlan_info);
	if (ret) {
		LOG_ERR("Failed to get the wireless info: 0x%x", ret);
		return -EIO;
	}

	strncpy(status->ssid, wlan_info.ssid, WIFI_SSID_MAX_LEN);
	status->ssid_len = strlen(status->ssid);
	status->wpa3_ent_type = WIFI_WPA3_ENTERPRISE_NA;

	if (interface & SL_WIFI_2_4GHZ_INTERFACE) {
		status->band = WIFI_FREQ_BAND_2_4_GHZ;
	}

	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) == SL_WIFI_CLIENT_INTERFACE) {
		sl_wifi_operational_statistics_t operational_statistics = { };

		status->link_mode = WIFI_LINK_MODE_UNKNOWN;
		status->iface_mode = WIFI_MODE_INFRA;
		status->channel = wlan_info.channel_number;
		status->twt_capable = true;

		ret = sl_si91x_get_join_configuration(interface, &join_config);
		if (ret != SL_STATUS_OK) {
			LOG_ERR("Failed to get join configuration: 0x%x", ret);
			return -EINVAL;
		}

		if (wlan_info.sec_type == SL_WIFI_WPA3) {
			status->mfp = WIFI_MFP_REQUIRED;
		} else if (wlan_info.sec_type == SL_WIFI_WPA3_TRANSITION) {
			status->mfp = WIFI_MFP_OPTIONAL;
		} else if (wlan_info.sec_type == SL_WIFI_WPA2) {
			if (join_config & SL_SI91X_JOIN_FEAT_MFP_CAPABLE_REQUIRED) {
				status->mfp = WIFI_MFP_REQUIRED;
			} else {
				status->mfp = WIFI_MFP_OPTIONAL;
			}
		} else {
			status->mfp = WIFI_MFP_DISABLE;
		}

		ret = sl_wifi_get_signal_strength(SL_WIFI_CLIENT_INTERFACE, &rssi);
		if (ret) {
			LOG_ERR("Failed to get signal strength: 0x%x", ret);
			return -EINVAL;
		}
		status->rssi = rssi;

		ret = sl_wifi_get_operational_statistics(SL_WIFI_CLIENT_INTERFACE,
							 &operational_statistics);
		if (ret) {
			LOG_ERR("Failed to get operational statistics: 0x%x", ret);
			return -EINVAL;
		}

		status->beacon_interval = sys_get_le16(operational_statistics.beacon_interval);
		status->dtim_period = operational_statistics.dtim_period;
		memcpy(status->bssid, wlan_info.bssid, WIFI_MAC_ADDR_LEN);
	} else if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) == SL_WIFI_AP_INTERFACE) {
		sl_wifi_ap_configuration_t sl_ap_cfg = { };

		ret = sl_wifi_get_ap_configuration(SL_WIFI_AP_INTERFACE, &sl_ap_cfg);
		if (ret) {
			LOG_ERR("Failed to get the AP configuration: 0x%x", ret);
			return -EINVAL;
		}
		status->twt_capable = false;
		status->link_mode = WIFI_4;
		status->iface_mode = WIFI_MODE_AP;
		status->mfp = WIFI_MFP_DISABLE;
		status->channel = sl_ap_cfg.channel.channel;
		status->beacon_interval = sl_ap_cfg.beacon_interval;
		status->dtim_period = sl_ap_cfg.dtim_beacon_count;
		wlan_info.sec_type = (uint8_t)sl_ap_cfg.security;
		memcpy(status->bssid, wlan_info.mac_address, WIFI_MAC_ADDR_LEN);
	} else {
		status->link_mode = WIFI_LINK_MODE_UNKNOWN;
		status->iface_mode = WIFI_MODE_UNKNOWN;
		status->channel = 0;

		return -EINVAL;
	}

	switch (wlan_info.sec_type) {
	case SL_WIFI_OPEN:
		status->security = WIFI_SECURITY_TYPE_NONE;
		break;
	case SL_WIFI_WPA:
		status->security = WIFI_SECURITY_TYPE_WPA_PSK;
		break;
	case SL_WIFI_WPA2:
		status->security = WIFI_SECURITY_TYPE_PSK;
		break;
	case SL_WIFI_WPA_WPA2_MIXED:
		status->security = WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL;
		break;
	case SL_WIFI_WPA3:
		status->security = WIFI_SECURITY_TYPE_SAE;
		break;
	default:
		status->security = WIFI_SECURITY_TYPE_UNKNOWN;
	}

	wifi_mgmt_raise_iface_status_event(sidev->iface, status);
	return ret;
}

static int siwx91x_disconnect(const struct device *dev)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	struct siwx91x_dev *sidev = dev->data;
	int ret;

	if (sidev->state != WIFI_STATE_COMPLETED) {
		LOG_ERR("Command given in invalid state");
		return -EINVAL;
	}

	ret = sl_wifi_disconnect(interface);
	if (ret != SL_STATUS_OK) {
		wifi_mgmt_raise_disconnect_result_event(sidev->iface, ret);
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE)) {
		net_if_dormant_on(sidev->iface);
	}

	return 0;
}

static int siwx91x_ap_disable(const struct device *dev)
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
	return ret;
}

static bool siwx91x_param_changed(struct wifi_iface_status *prev_params,
				     struct wifi_connect_req_params *new_params)
{
	__ASSERT(prev_params, "prev params cannot be NULL");
	__ASSERT(new_params, "new params cannot be NULL");

	if (new_params->ssid_length != prev_params->ssid_len ||
	    memcmp(new_params->ssid, prev_params->ssid, prev_params->ssid_len) != 0 ||
	    new_params->security != prev_params->security) {
		return true;
	} else if (new_params->channel != WIFI_CHANNEL_ANY &&
		   new_params->channel != prev_params->channel) {
		return true;
	}

	return false;
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

static int siwx91x_disconnect_if_required(const struct device *dev,
					  struct wifi_connect_req_params *new_params)
{
	struct wifi_iface_status prev_params = { };
	uint32_t prev_psk_length = WIFI_PSK_MAX_LEN;
	uint8_t prev_psk[WIFI_PSK_MAX_LEN];
	sl_net_credential_type_t psk_type;
	int ret;

	ret = siwx91x_status(dev, &prev_params);
	if (ret < 0) {
		return ret;
	}

	if (siwx91x_param_changed(&prev_params, new_params)) {
		return siwx91x_disconnect(dev);
	}

	if (new_params->security != WIFI_SECURITY_TYPE_NONE) {
		ret = sl_net_get_credential(SL_NET_DEFAULT_WIFI_CLIENT_CREDENTIAL_ID, &psk_type,
					    prev_psk, &prev_psk_length);
		if (ret < 0) {
			LOG_ERR("Failed to get credentials: 0x%x", ret);
			return -EIO;
		}

		if (new_params->psk_length != prev_psk_length ||
		    memcmp(new_params->psk, prev_psk, prev_psk_length) != 0) {
			return siwx91x_disconnect(dev);
		}
	}

	LOG_ERR("Device already in active state");
	return -EALREADY;
}

static int siwx91x_ap_enable(const struct device *dev, struct wifi_connect_req_params *params)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	struct siwx91x_dev *sidev = dev->data;
	sl_wifi_ap_configuration_t saved_ap_cfg;
	int ret;
	int sec;

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

	sec = siwx91x_map_ap_security(params->security);
	if (sec < 0) {
		LOG_ERR("Invalid security type");
		wifi_mgmt_raise_ap_enable_result_event(sidev->iface,
						       WIFI_STATUS_AP_AUTH_TYPE_NOT_SUPPORTED);
		return -EINVAL;
	}

	siwx91x_ap_cfg.security = sec;
	if (params->security != WIFI_SECURITY_TYPE_NONE) {
		ret = sl_net_set_credential(siwx91x_ap_cfg.credential_id, SL_NET_WIFI_PSK,
					    params->psk, params->psk_length);
		if (ret != SL_STATUS_OK) {
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

	ret = sl_wifi_start_ap(SL_WIFI_AP_INTERFACE | SL_WIFI_2_4GHZ_INTERFACE, &siwx91x_ap_cfg);
	if (ret != SL_STATUS_OK) {
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

static int siwx91x_ap_sta_disconnect(const struct device *dev, const uint8_t *mac_addr)
{
	ARG_UNUSED(dev);
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	sl_mac_address_t mac = { };
	int ret;

	__ASSERT(mac_addr, "mac_addr cannot be NULL");

	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_AP_INTERFACE) {
		LOG_ERR("Interface not in AP mode");
		return -EINVAL;
	}

	memcpy(mac.octet, mac_addr, ARRAY_SIZE(mac.octet));

	ret = sl_wifi_disconnect_ap_client(SL_WIFI_AP_INTERFACE | SL_WIFI_2_4GHZ_INTERFACE,
					  &mac, SL_WIFI_DEAUTH);
	if (ret) {
		LOG_ERR("Failed	to disconnect: 0x%x", ret);
		return -EIO;
	}

	return ret;
}

static sl_status_t siwx91x_on_ap_sta_connect(sl_wifi_event_t event, void *data,
					     uint32_t data_length, void *arg)
{
	ARG_UNUSED(event);
	ARG_UNUSED(data_length);
	struct siwx91x_dev *sidev = arg;
	struct wifi_ap_sta_info sta_info = { };

	__ASSERT(data, "data cannot be NULL");
	__ASSERT(arg, "arg cannot be NULL");

	memcpy(sta_info.mac, data, WIFI_MAC_ADDR_LEN);
	sta_info.mac_length = WIFI_MAC_ADDR_LEN;
	sta_info.link_mode = WIFI_LINK_MODE_UNKNOWN;

	wifi_mgmt_raise_ap_sta_connected_event(sidev->iface, &sta_info);

	return SL_STATUS_OK;
}

static sl_status_t siwx91x_on_ap_sta_disconnect(sl_wifi_event_t event, void *data,
						uint32_t data_length, void *arg)
{
	ARG_UNUSED(event);
	ARG_UNUSED(data_length);
	struct siwx91x_dev *sidev = arg;
	struct wifi_ap_sta_info sta_info = { };

	__ASSERT(data, "data cannot be NULL");
	__ASSERT(arg, "arg cannot be NULL");

	memcpy(sta_info.mac, data, WIFI_MAC_ADDR_LEN);
	sta_info.mac_length = WIFI_MAC_ADDR_LEN;
	wifi_mgmt_raise_ap_sta_disconnected_event(sidev->iface, &sta_info);

	return SL_STATUS_OK;
}

static int siwx91x_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	sl_wifi_client_configuration_t wifi_config = {
		.bss_type = SL_WIFI_BSS_TYPE_INFRASTRUCTURE,
		.encryption = SL_WIFI_DEFAULT_ENCRYPTION,
		.credential_id = SL_NET_DEFAULT_WIFI_CLIENT_CREDENTIAL_ID,
	};
	struct siwx91x_dev *sidev = dev->data;
	enum wifi_mfp_options mfp_conf;
	int ret = 0;

	if (sidev->state == WIFI_STATE_COMPLETED) {
		ret = siwx91x_disconnect_if_required(dev, params);
		if (ret < 0) {
			wifi_mgmt_raise_connect_result_event(sidev->iface, WIFI_STATUS_CONN_FAIL);
			return ret;
		}
	}

	switch (params->security) {
	case WIFI_SECURITY_TYPE_NONE:
		wifi_config.security = SL_WIFI_OPEN;
		break;
	case WIFI_SECURITY_TYPE_WPA_PSK:
		wifi_config.security = SL_WIFI_WPA;
		break;
	case WIFI_SECURITY_TYPE_PSK:
		/* This case is meant to fall through to the next */
	case WIFI_SECURITY_TYPE_PSK_SHA256:
		/* Use WPA2 security as the device supports only SHA256
		 * key derivation for WPA2-PSK
		 */
		wifi_config.security = SL_WIFI_WPA2;
		break;
	case WIFI_SECURITY_TYPE_SAE_AUTO:
		/* Use WPA3 security as the device supports only HNP and H2E
		 * methods for SAE
		 */
		wifi_config.security = SL_WIFI_WPA3;
		break;
	case WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL:
		/* Use WPA2/WPA3 security as the device supports both */
		wifi_config.security = SL_WIFI_WPA3_TRANSITION;
		break;
	/* Zephyr WiFi shell doesn't specify how to pass credential for these
	 * key managements.
	 */
	case WIFI_SECURITY_TYPE_WEP: /* SL_WIFI_WEP/SL_WIFI_WEP_ENCRYPTION */
	case WIFI_SECURITY_TYPE_EAP: /* SL_WIFI_WPA2_ENTERPRISE/<various> */
	case WIFI_SECURITY_TYPE_WAPI:
	default:
		return -ENOTSUP;
	}

	if (params->band != WIFI_FREQ_BAND_UNKNOWN && params->band != WIFI_FREQ_BAND_2_4_GHZ) {
		wifi_mgmt_raise_connect_result_event(sidev->iface, WIFI_STATUS_CONN_FAIL);
		return -ENOTSUP;
	}

	if (params->psk_length) {
		ret = sl_net_set_credential(SL_NET_DEFAULT_WIFI_CLIENT_CREDENTIAL_ID,
					    SL_NET_WIFI_PSK, params->psk, params->psk_length);
	} else if (params->sae_password_length) {
		ret = sl_net_set_credential(SL_NET_DEFAULT_WIFI_CLIENT_CREDENTIAL_ID,
					    SL_NET_WIFI_PSK, params->sae_password,
					    params->sae_password_length);
	}

	if (ret != SL_STATUS_OK) {
		LOG_ERR("Failed to set credentials: 0x%x", ret);
		wifi_mgmt_raise_connect_result_event(sidev->iface, WIFI_STATUS_CONN_FAIL);
		return -EINVAL;
	}

	if (params->security == WIFI_SECURITY_TYPE_PSK_SHA256) {
		mfp_conf = siwx91x_set_sta_mfp_option(wifi_config.security, WIFI_MFP_REQUIRED);
	} else {
		mfp_conf = siwx91x_set_sta_mfp_option(wifi_config.security, params->mfp);
	}

	if (params->mfp != mfp_conf) {
		LOG_WRN("Needed MFP %s but got MFP %s, hence setting to MFP %s",
			wifi_mfp_txt(mfp_conf), wifi_mfp_txt(params->mfp), wifi_mfp_txt(mfp_conf));
	}

	if (params->channel != WIFI_CHANNEL_ANY) {
		wifi_config.channel.channel = params->channel;
	} else {
		wifi_config.channel.channel = 0;
	}

	wifi_config.ssid.length = params->ssid_length,
	memcpy(wifi_config.ssid.value, params->ssid, params->ssid_length);

	ret = sl_wifi_connect(interface, &wifi_config, 0);
	if (ret != SL_STATUS_IN_PROGRESS) {
		wifi_mgmt_raise_connect_result_event(sidev->iface, WIFI_STATUS_CONN_FAIL);
		return -EIO;
	}

	return 0;
}

static void siwx91x_report_scan_res(struct siwx91x_dev *sidev, sl_wifi_scan_result_t *result,
				    int item)
{
	static const struct {
		int sl_val;
		int z_val;
	} security_convert[] = {
		{ SL_WIFI_OPEN,            WIFI_SECURITY_TYPE_NONE    },
		{ SL_WIFI_WEP,             WIFI_SECURITY_TYPE_WEP     },
		{ SL_WIFI_WPA,             WIFI_SECURITY_TYPE_WPA_PSK },
		{ SL_WIFI_WPA2,            WIFI_SECURITY_TYPE_PSK     },
		{ SL_WIFI_WPA3,            WIFI_SECURITY_TYPE_SAE     },
		{ SL_WIFI_WPA3_TRANSITION, WIFI_SECURITY_TYPE_SAE     },
		{ SL_WIFI_WPA_ENTERPRISE,  WIFI_SECURITY_TYPE_EAP     },
		{ SL_WIFI_WPA2_ENTERPRISE, WIFI_SECURITY_TYPE_EAP     },
	};

	struct wifi_scan_result tmp = {
		.channel = result->scan_info[item].rf_channel,
		.rssi = result->scan_info[item].rssi_val,
		.ssid_length = strlen(result->scan_info[item].ssid),
		.mac_length = sizeof(result->scan_info[item].bssid),
		.security = WIFI_SECURITY_TYPE_UNKNOWN,
		.mfp = WIFI_MFP_UNKNOWN,
		.band = WIFI_FREQ_BAND_2_4_GHZ,
	};

	if (result->scan_count == 0) {
		return;
	}

	if (result->scan_info[item].rf_channel <= 0 || result->scan_info[item].rf_channel > 14) {
		LOG_WRN("Unexpected scan result");
		tmp.band = WIFI_FREQ_BAND_UNKNOWN;
	}

	memcpy(tmp.ssid, result->scan_info[item].ssid, tmp.ssid_length);
	memcpy(tmp.mac, result->scan_info[item].bssid, tmp.mac_length);

	ARRAY_FOR_EACH(security_convert, i) {
		if (security_convert[i].sl_val == result->scan_info[item].security_mode) {
			tmp.security = security_convert[i].z_val;
		}
	}

	sidev->scan_res_cb(sidev->iface, 0, &tmp);
}

static unsigned int siwx91x_on_scan(sl_wifi_event_t event, sl_wifi_scan_result_t *result,
				    uint32_t result_size, void *arg)
{
	struct siwx91x_dev *sidev = arg;
	int i, scan_count;

	if (!sidev->scan_res_cb) {
		return -EFAULT;
	}

	if (event & SL_WIFI_EVENT_FAIL_INDICATION) {
		memset(result, 0, sizeof(*result));
	}

	if (sidev->scan_max_bss_cnt) {
		scan_count = MIN(result->scan_count, sidev->scan_max_bss_cnt);
	} else {
		scan_count = result->scan_count;
	}

	for (i = 0; i < scan_count; i++) {
		siwx91x_report_scan_res(sidev, result, i);
	}

	sidev->scan_res_cb(sidev->iface, 0, NULL);
	sidev->state = sidev->scan_prev_state;

	return 0;
}

static int
siwx91x_configure_scan_dwell_time(sl_wifi_scan_type_t scan_type, uint16_t dwell_time_active,
				  uint16_t dwell_time_passive,
				  sl_wifi_advanced_scan_configuration_t *advanced_scan_config)
{
	int ret = 0;

	if (dwell_time_active && (dwell_time_active < 5 || dwell_time_active > 1000)) {
		LOG_ERR("Invalid active scan dwell time");
		return -EINVAL;
	}

	if (dwell_time_passive && (dwell_time_passive < 10 || dwell_time_passive > 1000)) {
		LOG_ERR("Invalid passive scan dwell time");
		return -EINVAL;
	}

	switch (scan_type) {
	case SL_WIFI_SCAN_TYPE_ACTIVE:
		ret = sl_si91x_configure_timeout(SL_SI91X_CHANNEL_ACTIVE_SCAN_TIMEOUT,
						 dwell_time_active);
		break;
	case SL_WIFI_SCAN_TYPE_PASSIVE:
		if (!dwell_time_passive) {
			dwell_time_passive = SIWX91X_DEFAULT_PASSIVE_SCAN_DWELL_TIME;
		}
		ret = sl_si91x_configure_timeout(SL_SI91X_CHANNEL_PASSIVE_SCAN_TIMEOUT,
						 dwell_time_passive);
		break;
	case SL_WIFI_SCAN_TYPE_ADV_SCAN:
		__ASSERT(advanced_scan_config, "advanced_scan_config cannot be NULL");

		if (!dwell_time_active) {
			dwell_time_active = CONFIG_WIFI_SILABS_SIWX91X_ADV_ACTIVE_SCAN_DURATION;
		}
		advanced_scan_config->active_channel_time = dwell_time_active;

		if (!dwell_time_passive) {
			dwell_time_passive = CONFIG_WIFI_SILABS_SIWX91X_ADV_PASSIVE_SCAN_DURATION;
		}
		advanced_scan_config->passive_channel_time = dwell_time_passive;
		break;
	default:
		break;
	}

	return ret;
}

static int siwx91x_scan(const struct device *dev, struct wifi_scan_params *z_scan_config,
			scan_result_cb_t cb)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	sl_wifi_scan_configuration_t sl_scan_config = { };
	sl_wifi_advanced_scan_configuration_t advanced_scan_config = {
		.trigger_level = CONFIG_WIFI_SILABS_SIWX91X_ADV_SCAN_THRESHOLD,
		.trigger_level_change = CONFIG_WIFI_SILABS_SIWX91X_ADV_RSSI_TOLERANCE_THRESHOLD,
		.enable_multi_probe = CONFIG_WIFI_SILABS_SIWX91X_ADV_MULTIPROBE,
		.enable_instant_scan = CONFIG_WIFI_SILABS_SIWX91X_ENABLE_INSTANT_SCAN,
	};
	sl_wifi_roam_configuration_t roam_configuration = {
#ifdef CONFIG_WIFI_SILABS_SIWX91X_ENABLE_ROAMING
		.trigger_level = CONFIG_WIFI_SILABS_SIWX91X_ROAMING_TRIGGER_LEVEL,
		.trigger_level_change = CONFIG_WIFI_SILABS_SIWX91X_ROAMING_TRIGGER_LEVEL_CHANGE,
#else
		.trigger_level = SL_WIFI_NEVER_ROAM,
		.trigger_level_change = 0,
#endif
	};
	struct siwx91x_dev *sidev = dev->data;
	sl_wifi_ssid_t ssid = { };
	int ret;

	__ASSERT(z_scan_config, "z_scan_config cannot be NULL");

	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_CLIENT_INTERFACE) {
		LOG_ERR("Interface not in STA mode");
		return -EINVAL;
	}

	if (sidev->state != WIFI_STATE_DISCONNECTED && sidev->state != WIFI_STATE_INACTIVE &&
	    sidev->state != WIFI_STATE_COMPLETED) {
		LOG_ERR("Command given in invalid state");
		return -EBUSY;
	}

	if (z_scan_config->bands & ~(BIT(WIFI_FREQ_BAND_UNKNOWN) | BIT(WIFI_FREQ_BAND_2_4_GHZ))) {
		LOG_ERR("Invalid band entered");
		return -EINVAL;
	}

	if (sidev->state == WIFI_STATE_COMPLETED) {
		siwx91x_configure_scan_dwell_time(SL_WIFI_SCAN_TYPE_ADV_SCAN,
						  z_scan_config->dwell_time_active,
						  z_scan_config->dwell_time_passive,
						  &advanced_scan_config);

		ret = sl_wifi_set_advanced_scan_configuration(&advanced_scan_config);
		if (ret != SL_STATUS_OK) {
			LOG_ERR("advanced scan configuration failed with status %x", ret);
			return -EINVAL;
		}

		ret = sl_wifi_set_roam_configuration(interface, &roam_configuration);
		if (ret != SL_STATUS_OK) {
			LOG_ERR("roaming configuration failed with status %x", ret);
			return -EINVAL;
		}

		sl_scan_config.type = SL_WIFI_SCAN_TYPE_ADV_SCAN;
		sl_scan_config.periodic_scan_interval =
			CONFIG_WIFI_SILABS_SIWX91X_ADV_SCAN_PERIODICITY;
	} else {
		if (z_scan_config->scan_type == WIFI_SCAN_TYPE_ACTIVE) {
			sl_scan_config.type = SL_WIFI_SCAN_TYPE_ACTIVE;
			ret = siwx91x_configure_scan_dwell_time(SL_WIFI_SCAN_TYPE_ACTIVE,
								z_scan_config->dwell_time_active,
								z_scan_config->dwell_time_passive,
								NULL);
		} else {
			sl_scan_config.type = SL_WIFI_SCAN_TYPE_PASSIVE;
			ret = siwx91x_configure_scan_dwell_time(SL_WIFI_SCAN_TYPE_PASSIVE,
								z_scan_config->dwell_time_active,
								z_scan_config->dwell_time_passive,
								NULL);
		}
		if (ret != SL_STATUS_OK) {
			LOG_ERR("Failed to configure timeout");
			return -EINVAL;
		}
	}

	for (int i = 0; i < ARRAY_SIZE(z_scan_config->band_chan); i++) {
		/* End of channel list */
		if (z_scan_config->band_chan[i].channel == 0) {
			break;
		}

		if (z_scan_config->band_chan[i].band == WIFI_FREQ_BAND_2_4_GHZ) {
			sl_scan_config.channel_bitmap_2g4 |=
				BIT(z_scan_config->band_chan[i].channel - 1);
		}
	}

	if (z_scan_config->band_chan[0].channel && !sl_scan_config.channel_bitmap_2g4) {
		LOG_ERR("No supported channels in the request");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_WIFI_MGMT_SCAN_SSID_FILT_MAX)) {
		if (z_scan_config->ssids[0]) {
			strncpy(ssid.value, z_scan_config->ssids[0], WIFI_SSID_MAX_LEN);
			ssid.length = strlen(z_scan_config->ssids[0]);
		}
	}

	sidev->scan_max_bss_cnt = z_scan_config->max_bss_cnt;
	sidev->scan_res_cb = cb;
	ret = sl_wifi_start_scan(SL_WIFI_CLIENT_2_4GHZ_INTERFACE, (ssid.length > 0) ? &ssid : NULL,
				 &sl_scan_config);
	if (ret != SL_STATUS_IN_PROGRESS) {
		return -EIO;
	}
	sidev->scan_prev_state = sidev->state;
	sidev->state = WIFI_STATE_SCANNING;

	return 0;
}

static int siwx91x_mode(const struct device *dev, struct wifi_mode_info *mode)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	struct siwx91x_dev *sidev = dev->data;
	int cur_mode;
	int ret = 0;

	__ASSERT(mode, "mode cannot be NULL");

	cur_mode = siwx91x_sl_to_z_mode(FIELD_GET(SIWX91X_INTERFACE_MASK, interface));
	if (cur_mode < 0) {
		return -EIO;
	}

	if (mode->oper == WIFI_MGMT_GET) {
		mode->mode = cur_mode;
	} else if (mode->oper == WIFI_MGMT_SET) {
		if (cur_mode != mode->mode) {
			ret = siwx91x_nwp_mode_switch(mode->mode, false, 0);
			if (ret < 0) {
				return ret;
			}
		}
		sidev->state = WIFI_STATE_INACTIVE;
	}

	return 0;
}

static int siwx91x_ap_config_params(const struct device *dev, struct wifi_ap_config_params *params)
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

#ifdef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE

static int siwx91x_send(const struct device *dev, struct net_pkt *pkt)
{
	size_t pkt_len = net_pkt_get_len(pkt);
	sl_wifi_interface_t interface;
	struct net_buf *buf = NULL;
	int ret;

	if (net_pkt_get_len(pkt) > _NET_ETH_MAX_FRAME_SIZE) {
		LOG_ERR("unexpected buffer size");
		return -ENOBUFS;
	}
	buf = net_buf_alloc(&siwx91x_tx_pool, K_FOREVER);
	if (!buf) {
		return -ENOBUFS;
	}
	if (net_pkt_read(pkt, buf->data, pkt_len)) {
		net_buf_unref(buf);
		return -ENOBUFS;
	}
	net_buf_add(buf, pkt_len);
	interface = sl_wifi_get_default_interface();
	ret = sl_wifi_send_raw_data_frame(FIELD_GET(SIWX91X_INTERFACE_MASK, interface),
					  buf->data, pkt_len);
	if (ret) {
		net_buf_unref(buf);
		return -EIO;
	}

	net_pkt_unref(pkt);
	net_buf_unref(buf);

	return 0;
}

/* Receive callback. Keep the name as it is declared weak in WiseConnect */
sl_status_t sl_si91x_host_process_data_frame(sl_wifi_interface_t interface,
					     sl_wifi_buffer_t *buffer)
{
	sl_si91x_packet_t *si_pkt = sl_si91x_host_get_buffer_data(buffer, 0, NULL);
	struct net_if *iface = net_if_get_first_wifi();
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_rx_alloc_with_buffer(iface, buffer->length, AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("net_pkt_rx_alloc_with_buffer() failed");
		return SL_STATUS_FAIL;
	}
	ret = net_pkt_write(pkt, si_pkt->data, si_pkt->length);
	if (ret < 0) {
		LOG_ERR("net_pkt_write(): %d", ret);
		goto unref;
	}
	ret = net_recv_data(iface, pkt);
	if (ret < 0) {
		LOG_ERR("net_recv_data((): %d", ret);
		goto unref;
	}
	return 0;

unref:
	net_pkt_unref(pkt);
	return SL_STATUS_FAIL;
}

#endif

static void siwx91x_ethernet_init(struct net_if *iface)
{
	struct ethernet_context *eth_ctx;

	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE)) {
		eth_ctx = net_if_l2_data(iface);
		eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;
		ethernet_init(iface);
	}
}

#if defined(CONFIG_NET_STATISTICS_WIFI)
static int siwx91x_stats(const struct device *dev, struct net_stats_wifi *stats)
{
	ARG_UNUSED(dev);
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	sl_wifi_statistics_t statistics = { };
	int ret;

	__ASSERT(stats, "stats cannot be NULL");

	ret = sl_wifi_get_statistics(FIELD_GET(SIWX91X_INTERFACE_MASK, interface), &statistics);
	if (ret) {
		LOG_ERR("Failed to get stat: 0x%x", ret);
		return -EINVAL;
	}

	stats->multicast.rx = statistics.mcast_rx_count;
	stats->multicast.tx = statistics.mcast_tx_count;
	stats->unicast.rx = statistics.ucast_rx_count;
	stats->unicast.tx = statistics.ucast_tx_count;
	stats->sta_mgmt.beacons_rx = statistics.beacon_rx_count;
	stats->sta_mgmt.beacons_miss = statistics.beacon_lost_count;
	stats->overrun_count = statistics.overrun_count;

	return ret;
}
#endif

static int siwx91x_get_version(const struct device *dev, struct wifi_version *params)
{
	sl_wifi_firmware_version_t fw_version = { };
	struct siwx91x_dev *sidev = dev->data;
	static char fw_version_str[32];
	sl_status_t status;

	__ASSERT(params, "params cannot be NULL");

	if (sidev->state == WIFI_STATE_INTERFACE_DISABLED) {
		return -EIO;
	}

	status = sl_wifi_get_firmware_version(&fw_version);
	if (status != SL_STATUS_OK) {
		return -EINVAL;
	}

	snprintf(fw_version_str, sizeof(fw_version_str), "%02x%02x.%d.%d.%d.%d.%d.%d",
		 fw_version.chip_id,     fw_version.rom_id,
		 fw_version.major,       fw_version.minor,
		 fw_version.security_version, fw_version.patch_num,
		 fw_version.customer_id, fw_version.build_num);

	params->fw_version = fw_version_str;
	params->drv_version = SIWX91X_DRIVER_VERSION;

	return 0;
}

static void siwx91x_iface_init(struct net_if *iface)
{
	struct siwx91x_dev *sidev = iface->if_dev->dev->data;
	sl_status_t status;

	sidev->state = WIFI_STATE_INTERFACE_DISABLED;
	sidev->iface = iface;

	sl_wifi_set_callback(SL_WIFI_SCAN_RESULT_EVENTS,
			     (sl_wifi_callback_function_t)siwx91x_on_scan, sidev);
	sl_wifi_set_callback(SL_WIFI_JOIN_EVENTS, (sl_wifi_callback_function_t)siwx91x_on_join,
			     sidev);
	sl_wifi_set_callback(SL_WIFI_CLIENT_CONNECTED_EVENTS, siwx91x_on_ap_sta_connect, sidev);
	sl_wifi_set_callback(SL_WIFI_CLIENT_DISCONNECTED_EVENTS, siwx91x_on_ap_sta_disconnect,
			     sidev);
	sl_wifi_set_callback(SL_WIFI_STATS_RESPONSE_EVENTS, siwx91x_wifi_module_stats_event_handler,
			     sidev);

	status = sl_wifi_get_mac_address(SL_WIFI_CLIENT_INTERFACE, &sidev->macaddr);
	if (status) {
		LOG_ERR("sl_wifi_get_mac_address(): %#04x", status);
		return;
	}
	net_if_set_link_addr(iface, sidev->macaddr.octet, sizeof(sidev->macaddr.octet),
			     NET_LINK_ETHERNET);
	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE)) {
		net_if_dormant_on(sidev->iface);
	}
	siwx91x_sock_init(iface);
	siwx91x_ethernet_init(iface);

	sidev->state = WIFI_STATE_INACTIVE;
}

static int siwx91x_dev_init(const struct device *dev)
{
	return 0;
}

static int siwx91x_convert_z_sl_twt_req_type(enum wifi_twt_setup_cmd z_req_cmd)
{
	switch (z_req_cmd) {
	case WIFI_TWT_SETUP_CMD_REQUEST:
		return REQUEST_TWT;
	case WIFI_TWT_SETUP_CMD_SUGGEST:
		return SUGGEST_TWT;
	case WIFI_TWT_SETUP_CMD_DEMAND:
		return DEMAND_TWT;
	default:
		return -EINVAL;
	}
}

static int siwx91x_set_twt_setup(struct wifi_twt_params *params)
{
	sl_status_t status;
	int twt_req_type = siwx91x_convert_z_sl_twt_req_type(params->setup_cmd);

	sl_wifi_twt_request_t twt_req = {
		.twt_retry_interval = 5,
		.wake_duration_unit = 0,
		.wake_int_mantissa = params->setup.twt_mantissa,
		.un_announced_twt = !params->setup.announce,
		.wake_duration = params->setup.twt_wake_interval,
		.triggered_twt = params->setup.trigger,
		.wake_int_exp = params->setup.twt_exponent,
		.implicit_twt = 1,
		.twt_flow_id = params->flow_id,
		.twt_enable = 1,
		.req_type = twt_req_type,
	};

	if (twt_req_type < 0) {
		params->fail_reason = WIFI_TWT_FAIL_CMD_EXEC_FAIL;
		return -EINVAL;
	}

	if (!params->setup.twt_info_disable) {
		params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	if (params->setup.responder) {
		params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	/* implicit -> won't do renegotiation
	 * explicit -> must do renegotiation for each session
	 */
	if (!params->setup.implicit) {
		/* explicit twt is not supported */
		params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	if (params->setup.twt_wake_interval > 255 * 256) {
		twt_req.wake_duration_unit = 1;
		twt_req.wake_duration = params->setup.twt_wake_interval / 1024;
	} else {
		twt_req.wake_duration_unit = 0;
		twt_req.wake_duration = params->setup.twt_wake_interval / 256;
	}

	status = sl_wifi_enable_target_wake_time(&twt_req);
	if (status != SL_STATUS_OK) {
		params->fail_reason = WIFI_TWT_FAIL_CMD_EXEC_FAIL;
		params->resp_status = WIFI_TWT_RESP_NOT_RECEIVED;
		return -EINVAL;
	}

	return 0;
}

static int siwx91x_set_twt_teardown(struct wifi_twt_params *params)
{
	sl_status_t status;
	sl_wifi_twt_request_t twt_req = { };

	twt_req.twt_enable = 0;

	if (params->teardown.teardown_all) {
		twt_req.twt_flow_id = 0xFF;
	} else {
		twt_req.twt_flow_id = params->flow_id;
	}

	status = sl_wifi_disable_target_wake_time(&twt_req);
	if (status != SL_STATUS_OK) {
		params->fail_reason = WIFI_TWT_FAIL_CMD_EXEC_FAIL;
		params->teardown_status = WIFI_TWT_TEARDOWN_FAILED;
		return -EINVAL;
	}

	params->teardown_status = WIFI_TWT_TEARDOWN_SUCCESS;

	return 0;
}

static int siwx91x_set_twt(const struct device *dev, struct wifi_twt_params *params)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	struct siwx91x_dev *sidev = dev->data;

	__ASSERT(params, "params cannot be a NULL");

	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_CLIENT_INTERFACE) {
		params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	if (sidev->state != WIFI_STATE_DISCONNECTED && sidev->state != WIFI_STATE_INACTIVE &&
	    sidev->state != WIFI_STATE_COMPLETED) {
		LOG_ERR("Command given in invalid state");
		return -EBUSY;
	}

	if (params->negotiation_type != WIFI_TWT_INDIVIDUAL) {
		params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	if (params->operation == WIFI_TWT_SETUP) {
		return siwx91x_set_twt_setup(params);
	} else if (params->operation == WIFI_TWT_TEARDOWN) {
		return siwx91x_set_twt_teardown(params);
	}
	params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
	return -ENOTSUP;
}

static const struct wifi_mgmt_ops siwx91x_mgmt = {
	.scan			= siwx91x_scan,
	.connect		= siwx91x_connect,
	.disconnect		= siwx91x_disconnect,
	.ap_enable		= siwx91x_ap_enable,
	.ap_disable		= siwx91x_ap_disable,
	.ap_sta_disconnect	= siwx91x_ap_sta_disconnect,
	.iface_status		= siwx91x_status,
	.mode			= siwx91x_mode,
	.set_twt		= siwx91x_set_twt,
	.ap_config_params	= siwx91x_ap_config_params,
#if defined(CONFIG_NET_STATISTICS_WIFI)
	.get_stats		= siwx91x_stats,
#endif
	.get_version		= siwx91x_get_version,
	.set_power_save		= siwx91x_set_power_save,
	.get_power_save_config	= siwx91x_get_power_save_config,
};

static const struct net_wifi_mgmt_offload siwx91x_api = {
	.wifi_iface.iface_api.init = siwx91x_iface_init,
#ifdef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE
	.wifi_iface.send = siwx91x_send,
#else
	.wifi_iface.get_type = siwx91x_get_type,
#endif
	.wifi_mgmt_api = &siwx91x_mgmt,
};

static struct siwx91x_dev sidev = {
	.ps_params.enabled = WIFI_PS_DISABLED,
	.ps_params.exit_strategy = WIFI_PS_EXIT_EVERY_TIM,
	.ps_params.wakeup_mode = WIFI_PS_WAKEUP_MODE_DTIM,
	.max_num_sta = CONFIG_WIFI_MGMT_AP_MAX_NUM_STA,
};

#ifdef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE
ETH_NET_DEVICE_DT_INST_DEFINE(0, siwx91x_dev_init, NULL, &sidev, NULL,
			      CONFIG_WIFI_INIT_PRIORITY, &siwx91x_api, NET_ETH_MTU);
#else
NET_DEVICE_DT_INST_OFFLOAD_DEFINE(0, siwx91x_dev_init, NULL, &sidev, NULL,
				  CONFIG_WIFI_INIT_PRIORITY, &siwx91x_api, NET_ETH_MTU);
#endif
