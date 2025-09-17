/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024-2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>

#include <nwp.h>
#include "siwx91x_wifi.h"
#include "siwx91x_wifi_socket.h"
#include "siwx91x_wifi_ps.h"

#include "sl_rsi_utility.h"
#include "sl_net_default_values.h"
#include "sl_net.h"

LOG_MODULE_DECLARE(siwx91x_wifi);

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

sl_status_t siwx91x_wifi_module_stats_event_handler(sl_wifi_event_t event, void *response,
							   uint32_t result_length, void *arg)
{
	sl_si91x_module_state_stats_response_t *notif = response;
	const char *reason_str = siwx91x_get_reason_string(notif->reason_code);
	uint8_t module_state = notif->state_code & 0xF0;
	struct siwx91x_dev *sidev = arg;

	ARG_UNUSED(event);
	ARG_UNUSED(result_length);

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

unsigned int siwx91x_on_join(sl_wifi_event_t event,
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

int siwx91x_disconnect(const struct device *dev)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	struct siwx91x_dev *sidev = dev->data;
	int ret;

	if (sidev->state != WIFI_STATE_COMPLETED) {
		LOG_ERR("Command given in invalid state");
		return -EINVAL;
	}

	ret = sl_wifi_disconnect(interface);
	if (ret) {
		wifi_mgmt_raise_disconnect_result_event(sidev->iface, ret);
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE)) {
		net_if_dormant_on(sidev->iface);
	}

	return 0;
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

int siwx91x_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	sl_wifi_client_configuration_t wifi_config = {
		.bss_type = SL_WIFI_BSS_TYPE_INFRASTRUCTURE,
		.encryption = SL_WIFI_DEFAULT_ENCRYPTION,
		.credential_id = SL_NET_DEFAULT_WIFI_CLIENT_CREDENTIAL_ID,
	};
	struct siwx91x_dev *sidev = dev->data;
	int ret;

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
	} else {
		ret = 0;
	}

	if (ret) {
		LOG_ERR("Failed to set credentials: 0x%x", ret);
		wifi_mgmt_raise_connect_result_event(sidev->iface, WIFI_STATUS_CONN_FAIL);
		return -EINVAL;
	}

	/* The Zephyr mfp values match the values expected by
	 * Wiseconnect's mfp, even though the enum type differs.
	 */
	ret = sl_wifi_set_mfp(interface, (sl_wifi_mfp_mode_t)params->mfp);
	if (ret != SL_STATUS_OK) {
		LOG_ERR("Failed to set MFP: 0x%x", ret);
		wifi_mgmt_raise_connect_result_event(sidev->iface, WIFI_STATUS_CONN_FAIL);
		return -EINVAL;
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
