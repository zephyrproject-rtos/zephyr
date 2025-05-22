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
#include "siwx91x_wifi_ap.h"
#include "siwx91x_wifi_ps.h"
#include "siwx91x_wifi_scan.h"
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

LOG_MODULE_REGISTER(siwx91x_wifi);

NET_BUF_POOL_FIXED_DEFINE(siwx91x_tx_pool, 1, _NET_ETH_MAX_FRAME_SIZE, 0, NULL);

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

	siwx91x_apply_power_save(sidev);

	return 0;
}

int siwx91x_status(const struct device *dev, struct wifi_iface_status *status)
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

bool siwx91x_param_changed(struct wifi_iface_status *prev_params,
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

static const struct wifi_mgmt_ops siwx91x_mgmt = {
	.scan			= siwx91x_scan,
	.connect		= siwx91x_connect,
	.disconnect		= siwx91x_disconnect,
	.ap_enable		= siwx91x_ap_enable,
	.ap_disable		= siwx91x_ap_disable,
	.ap_sta_disconnect	= siwx91x_ap_sta_disconnect,
	.ap_config_params	= siwx91x_ap_config_params,
	.iface_status		= siwx91x_status,
	.mode			= siwx91x_mode,
	.set_twt		= siwx91x_set_twt,
	.set_power_save		= siwx91x_set_power_save,
	.get_power_save_config	= siwx91x_get_power_save_config,
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
