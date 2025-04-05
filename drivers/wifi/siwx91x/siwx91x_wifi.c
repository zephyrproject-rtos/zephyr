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

#define SIWX91X_DRIVER_VERSION KERNEL_VERSION_STRING

LOG_MODULE_REGISTER(siwx91x_wifi);

NET_BUF_POOL_FIXED_DEFINE(siwx91x_tx_pool, 1, _NET_ETH_MAX_FRAME_SIZE, 0, NULL);

static int siwx91x_sl_to_z_mode(sl_wifi_interface_t interface)
{
	switch (interface) {
	case SL_WIFI_CLIENT_INTERFACE:
		return WIFI_STA_MODE;
	case SL_WIFI_AP_INTERFACE:
		return WIFI_AP_MODE;
	default:
		return -EIO;
	}

	return 0;
}

static int siwx91x_bandwidth(enum wifi_frequency_bandwidths bandwidth)
{
	switch (bandwidth) {
	case WIFI_FREQ_BANDWIDTH_20MHZ:
		return SL_WIFI_BANDWIDTH_20MHz;
	case WIFI_FREQ_BANDWIDTH_40MHZ:
		return SL_WIFI_BANDWIDTH_40MHz;
	case WIFI_FREQ_BANDWIDTH_80MHZ:
		return SL_WIFI_BANDWIDTH_80MHz;
	default:
		return -EINVAL;
	}
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

	return 0;
}

static int siwx91x_ap_enable(const struct device *dev, struct wifi_connect_req_params *params)
{
	struct siwx91x_dev *sidev = dev->data;
	/* Wiseconnect requires a valid PSK even if WIFI_SECURITY_TYPE_NONE is selected */
	static const char dummy_psk[] = "dummy_value";
	sl_status_t ret;
	int sec;

	sl_wifi_ap_configuration_t siwx91x_ap_cfg = {
		.credential_id       = SL_NET_DEFAULT_WIFI_AP_CREDENTIAL_ID,
		.keepalive_type      = SL_SI91X_AP_NULL_BASED_KEEP_ALIVE,
		.rate_protocol       = SL_WIFI_RATE_PROTOCOL_AUTO,
		.encryption          = SL_WIFI_DEFAULT_ENCRYPTION,
		.ssid.length         = params->ssid_length,
		.tdi_flags           = SL_WIFI_TDI_NONE,
		.client_idle_timeout = 0xFF,
		.beacon_interval     = 100,
		.dtim_beacon_count   = 3,
		.maximum_clients     = 4,
		.beacon_stop         = 0,
		.options             = 0,
		.is_11n_enabled      = 1,
	};

	if (params->band != WIFI_FREQ_BAND_UNKNOWN && params->band != WIFI_FREQ_BAND_2_4_GHZ) {
		return -ENOTSUP;
	}

	if (params->channel == WIFI_CHANNEL_ANY) {
		siwx91x_ap_cfg.channel.channel = SL_WIFI_AUTO_CHANNEL;
	} else {
		siwx91x_ap_cfg.channel.channel = params->channel;
	}

	if (siwx91x_bandwidth(params->bandwidth) < 0) {
		return -EINVAL;
	}

	siwx91x_ap_cfg.channel.bandwidth = siwx91x_bandwidth(params->bandwidth);
	strncpy(siwx91x_ap_cfg.ssid.value, params->ssid, params->ssid_length);

	sec = siwx91x_map_ap_security(params->security);
	if (sec < 0) {
		LOG_ERR("Invalid security type");
		return -EINVAL;
	}

	siwx91x_ap_cfg.security = sec;

	if (params->security == WIFI_SECURITY_TYPE_NONE) {
		ret = sl_net_set_credential(siwx91x_ap_cfg.credential_id, SL_NET_WIFI_PSK,
					    dummy_psk, strlen(dummy_psk));
	} else {
		ret = sl_net_set_credential(siwx91x_ap_cfg.credential_id, SL_NET_WIFI_PSK,
					    params->psk, params->psk_length);
	}

	if (ret != SL_STATUS_OK) {
		return -EINVAL;
	}

	if (params->mfp != WIFI_MFP_DISABLE) {
		LOG_WRN("Needed MFP disable but got MFP %s, hence setting to MFP disable",
			wifi_mfp_txt(params->mfp));
	}

	ret = sl_wifi_start_ap(SL_WIFI_AP_INTERFACE | SL_WIFI_2_4GHZ_INTERFACE, &siwx91x_ap_cfg);
	if (ret != SL_STATUS_OK) {
		LOG_ERR("Failed to enable AP mode: 0x%x", ret);
		return -EIO;
	}

	sidev->state = WIFI_STATE_COMPLETED;
	return 0;
}

static int siwx91x_ap_disable(const struct device *dev)
{
	struct siwx91x_dev *sidev = dev->data;
	int ret;

	ret = sl_wifi_stop_ap(SL_WIFI_AP_2_4GHZ_INTERFACE);
	if (ret) {
		LOG_ERR("Failed to disable Wi-Fi AP mode: 0x%x", ret);
		return -EIO;
	}

	sidev->state = WIFI_STATE_INTERFACE_DISABLED;
	return ret;
}

static int siwx91x_ap_sta_disconnect(const struct device *dev, const uint8_t *mac_addr)
{
	ARG_UNUSED(dev);
	sl_mac_address_t mac = { };
	int ret;

	__ASSERT(mac_addr, "mac_addr cannot be NULL");

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
	struct siwx91x_dev *sidev = arg;
	struct wifi_ap_sta_info sta_info = { };

	__ASSERT(data, "data cannot be NULL");
	__ASSERT(arg, "arg cannot be NULL");

	memcpy(sta_info.mac, data, data_length);
	sta_info.mac_length = data_length;
	sta_info.link_mode = WIFI_LINK_MODE_UNKNOWN;

	wifi_mgmt_raise_ap_sta_connected_event(sidev->iface, &sta_info);

	return SL_STATUS_OK;
}

static sl_status_t siwx91x_on_ap_sta_disconnect(sl_wifi_event_t event, void *data,
						uint32_t data_length, void *arg)
{
	ARG_UNUSED(event);
	struct siwx91x_dev *sidev = arg;
	struct wifi_ap_sta_info sta_info = { };

	__ASSERT(data, "data cannot be NULL");
	__ASSERT(arg, "arg cannot be NULL");

	memcpy(sta_info.mac, data, data_length);
	sta_info.mac_length = data_length;
	wifi_mgmt_raise_ap_sta_disconnected_event(sidev->iface, &sta_info);

	return SL_STATUS_OK;
}

static int siwx91x_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	sl_wifi_client_configuration_t wifi_config = {
		.bss_type = SL_WIFI_BSS_TYPE_INFRASTRUCTURE,
		.encryption = SL_WIFI_DEFAULT_ENCRYPTION,
		.credential_id = SL_NET_DEFAULT_WIFI_CLIENT_CREDENTIAL_ID,
	};
	enum wifi_mfp_options mfp_conf;
	int ret = 0;

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
	}

	wifi_config.ssid.length = params->ssid_length,
	memcpy(wifi_config.ssid.value, params->ssid, params->ssid_length);

	ret = sl_wifi_connect(SL_WIFI_CLIENT_INTERFACE, &wifi_config, 0);
	if (ret != SL_STATUS_IN_PROGRESS) {
		return -EIO;
	}

	return 0;
}

static int siwx91x_disconnect(const struct device *dev)
{
	struct siwx91x_dev *sidev = dev->data;
	int ret;

	ret = sl_wifi_disconnect(SL_WIFI_CLIENT_INTERFACE);
	if (ret) {
		return -EIO;
	}
	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE)) {
		net_if_dormant_on(sidev->iface);
	}
	sidev->state = WIFI_STATE_DISCONNECTED;
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

static int siwx91x_scan(const struct device *dev, struct wifi_scan_params *z_scan_config,
			scan_result_cb_t cb)
{
	sl_wifi_scan_configuration_t sl_scan_config = { };
	struct siwx91x_dev *sidev = dev->data;
	sl_wifi_interface_t interface;
	sl_wifi_ssid_t ssid = { };
	int ret;

	__ASSERT(z_scan_config, "z_scan_config cannot be NULL");

	interface = sl_wifi_get_default_interface();
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

	if (z_scan_config->scan_type == WIFI_SCAN_TYPE_ACTIVE) {
		sl_scan_config.type = SL_WIFI_SCAN_TYPE_ACTIVE;
		ret = sl_si91x_configure_timeout(SL_SI91X_CHANNEL_ACTIVE_SCAN_TIMEOUT,
						 z_scan_config->dwell_time_active);
	} else {
		sl_scan_config.type = SL_WIFI_SCAN_TYPE_PASSIVE;
		ret = sl_si91x_configure_timeout(SL_SI91X_CHANNEL_PASSIVE_SCAN_TIMEOUT,
						 z_scan_config->dwell_time_passive);
	}
	if (ret) {
		return -EINVAL;
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

static int siwx91x_status(const struct device *dev, struct wifi_iface_status *status)
{
	sl_si91x_rsp_wireless_info_t wlan_info = { };
	struct siwx91x_dev *sidev = dev->data;
	uint8_t join_config;
	sl_wifi_interface_t interface;
	int32_t rssi;
	int ret;

	__ASSERT(status, "status cannot be NULL");

	memset(status, 0, sizeof(*status));

	status->state = sidev->state;
	if (sidev->state <= WIFI_STATE_INACTIVE) {
		return 0;
	}

	interface = sl_wifi_get_default_interface();
	ret = sl_wifi_get_wireless_info(&wlan_info);
	if (ret) {
		LOG_ERR("Failed to get the wireless info: 0x%x", ret);
		return -EIO;
	}

	strncpy(status->ssid, wlan_info.ssid, WIFI_SSID_MAX_LEN);
	status->ssid_len = strlen(status->ssid);
	memcpy(status->bssid, wlan_info.mac_address, WIFI_MAC_ADDR_LEN);
	status->wpa3_ent_type = WIFI_WPA3_ENTERPRISE_NA;

	ret = sl_si91x_get_join_configuration(interface, &join_config);
	if (ret != SL_STATUS_OK) {
		LOG_ERR("Failed to get join configuration: 0x%x", ret);
		return -EINVAL;
	}

	if (join_config & SL_SI91X_JOIN_FEAT_MFP_CAPABLE_REQUIRED) {
		status->mfp = WIFI_MFP_REQUIRED;
	} else if (join_config & SL_SI91X_JOIN_FEAT_MFP_CAPABLE_ONLY) {
		status->mfp = WIFI_MFP_OPTIONAL;
	} else {
		status->mfp = WIFI_MFP_DISABLE;
	}

	if (interface & SL_WIFI_2_4GHZ_INTERFACE) {
		status->band = WIFI_FREQ_BAND_2_4_GHZ;
	}

	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) == SL_WIFI_CLIENT_INTERFACE) {
		sl_wifi_operational_statistics_t operational_statistics = { };

		status->link_mode = WIFI_LINK_MODE_UNKNOWN;
		status->iface_mode = WIFI_MODE_INFRA;
		status->channel = wlan_info.channel_number;
		status->twt_capable = true;
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
		status->channel = sl_ap_cfg.channel.channel;
		status->beacon_interval = sl_ap_cfg.beacon_interval;
		status->dtim_period = sl_ap_cfg.dtim_beacon_count;
		wlan_info.sec_type = (uint8_t)sl_ap_cfg.security;
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

	return ret;
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
			ret = siwx91x_nwp_mode_switch(mode->mode);
			if (ret < 0) {
				return ret;
			}
		}
		sidev->state = WIFI_STATE_INACTIVE;
	}

	return 0;
}

#ifdef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE

static int siwx91x_send(const struct device *dev, struct net_pkt *pkt)
{
	size_t pkt_len = net_pkt_get_len(pkt);
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

	ret = sl_wifi_send_raw_data_frame(SL_WIFI_CLIENT_INTERFACE, buf->data, pkt_len);
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
	sl_wifi_interface_t interface;
	sl_wifi_statistics_t statistics = { };
	int ret;

	__ASSERT(stats, "stats cannot be NULL");

	interface = sl_wifi_get_default_interface();
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

	status = sl_wifi_get_mac_address(SL_WIFI_CLIENT_INTERFACE, &sidev->macaddr);
	if (status) {
		LOG_ERR("sl_wifi_get_mac_address(): %#04x", status);
		return;
	}
	net_if_set_link_addr(iface, sidev->macaddr.octet, sizeof(sidev->macaddr.octet),
			     NET_LINK_ETHERNET);
	siwx91x_sock_init(iface);
	siwx91x_ethernet_init(iface);

	sidev->state = WIFI_STATE_INACTIVE;
}

static int siwx91x_dev_init(const struct device *dev)
{
	return 0;
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
#if defined(CONFIG_NET_STATISTICS_WIFI)
	.get_stats		= siwx91x_stats,
#endif
	.get_version		= siwx91x_get_version,
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

static struct siwx91x_dev sidev;

#ifdef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE
ETH_NET_DEVICE_DT_INST_DEFINE(0, siwx91x_dev_init, NULL, &sidev, NULL,
			      CONFIG_WIFI_INIT_PRIORITY, &siwx91x_api, NET_ETH_MTU);
#else
NET_DEVICE_DT_INST_OFFLOAD_DEFINE(0, siwx91x_dev_init, NULL, &sidev, NULL,
				  CONFIG_WIFI_INIT_PRIORITY, &siwx91x_api, NET_ETH_MTU);
#endif
