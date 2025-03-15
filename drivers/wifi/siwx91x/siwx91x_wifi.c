/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT silabs_siwx91x_wifi

#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "siwx91x_wifi.h"
#include "siwx91x_wifi_socket.h"

#include "sl_rsi_utility.h"
#include "sl_net_constants.h"
#include "sl_wifi_types.h"
#include "sl_wifi_callback_framework.h"
#include "sl_wifi.h"
#include "sl_net.h"

#define SIWX91X_MAX_MONITOR_INTERVAL_MS    1000
#define SIWX91X_DEFAULT_BEACON_INTERVAL_MS 100
#define SIWX91X_LISTEN_INTERVAL_MAX        10

/* TODO Remove it once AP PR merged */
#define SIWX91X_INTERFACE_MASK (0x03)

LOG_MODULE_REGISTER(siwx91x_wifi);

NET_BUF_POOL_FIXED_DEFINE(siwx91x_tx_pool, 1, _NET_ETH_MAX_FRAME_SIZE, 0, NULL);

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

static int siwx91x_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	sl_wifi_client_configuration_t wifi_config = {
		.bss_type = SL_WIFI_BSS_TYPE_INFRASTRUCTURE,
		.encryption = SL_WIFI_DEFAULT_ENCRYPTION,
		.credential_id = SL_NET_DEFAULT_WIFI_CLIENT_CREDENTIAL_ID,
	};
	int ret;

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
		sl_net_set_credential(SL_NET_DEFAULT_WIFI_CLIENT_CREDENTIAL_ID, SL_NET_WIFI_PSK,
				      params->psk, params->psk_length);
	}

	if (params->sae_password_length) {
		sl_net_set_credential(SL_NET_DEFAULT_WIFI_CLIENT_CREDENTIAL_ID, SL_NET_WIFI_PSK,
				      params->sae_password, params->sae_password_length);
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
	sl_wifi_ssid_t ssid = {};
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

		if (!z_scan_config->dwell_time_active) {
			ret = sl_si91x_configure_timeout(SL_SI91X_CHANNEL_ACTIVE_SCAN_TIMEOUT,
							 SL_WIFI_DEFAULT_ACTIVE_CHANNEL_SCAN_TIME);
		} else {
			ret = sl_si91x_configure_timeout(SL_SI91X_CHANNEL_ACTIVE_SCAN_TIMEOUT,
							 z_scan_config->dwell_time_active);
		}

		if (ret != SL_STATUS_OK) {
			return -EINVAL;
		}
	} else {
		sl_scan_config.type = SL_WIFI_SCAN_TYPE_PASSIVE;
		ret = sl_si91x_configure_timeout(SL_SI91X_CHANNEL_PASSIVE_SCAN_TIMEOUT,
						 z_scan_config->dwell_time_passive);
		if (ret != SL_STATUS_OK) {
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

static int siwx91x_status(const struct device *dev, struct wifi_iface_status *status)
{
	struct siwx91x_dev *sidev = dev->data;
	int32_t rssi = -1;

	memset(status, 0, sizeof(*status));
	status->state = sidev->state;
	sl_wifi_get_signal_strength(SL_WIFI_CLIENT_INTERFACE, &rssi);
	status->rssi = rssi;
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

static void siwx91x_iface_init(struct net_if *iface)
{
	struct siwx91x_dev *sidev = iface->if_dev->dev->data;
	sl_status_t status;

	sidev->state = WIFI_STATE_INTERFACE_DISABLED;
	sidev->iface = iface;

	sl_wifi_set_scan_callback(siwx91x_on_scan, sidev);
	sl_wifi_set_join_callback(siwx91x_on_join, sidev);

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

struct siwx91x_ps_config {
	bool is_enabled;
	sl_si91x_performance_profile_t exit_strategy;
	sl_si91x_performance_profile_t mode;
} ps_config;

static int siwx91x_validate_ps_state(bool state, sl_si91x_performance_profile_t mode)
{
	if (mode > ASSOCIATED_POWER_SAVE_LOW_LATENCY) {
		return -EINVAL;
	}

	if ((state != ps_config.is_enabled) || (mode != ps_config.mode)) {
		ps_config.is_enabled = state;
		ps_config.mode = mode;
	} else if (state == WIFI_PS_ENABLED) {
		return -EAGAIN;
	}

	return 0;
}

static int siwx91x_set_power_save(const struct device *dev, struct wifi_ps_params *params)
{
	sl_wifi_performance_profile_t sl_ps_profile;
	sl_wifi_interface_t interface;
	sl_status_t status;
	int ret;

	__ASSERT(params, "params cannot be NULL");

	interface = sl_wifi_get_default_interface();
	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_CLIENT_INTERFACE) {
		LOG_ERR("Wi-Fi not in station mode");
		return -EIO;
	}

	get_wifi_current_performance_profile(&sl_ps_profile);

	switch (params->type) {
	case WIFI_PS_PARAM_STATE:
		if (params->enabled) {
			if (ps_config.exit_strategy == HIGH_PERFORMANCE) {
				sl_ps_profile.profile = ASSOCIATED_POWER_SAVE_LOW_LATENCY;
			} else {
				sl_ps_profile.profile = ps_config.exit_strategy;
			}
		} else {
			if (sl_ps_profile.profile == HIGH_PERFORMANCE) {
				return -EAGAIN;
			}

			sl_ps_profile.profile = HIGH_PERFORMANCE;
		}

		ret = siwx91x_validate_ps_state(params->enabled, sl_ps_profile.profile);
		if (ret < 0) {
			return ret;
		}

		break;
	case WIFI_PS_PARAM_MODE:
		if (params->mode != WIFI_PS_MODE_LEGACY) {
			params->fail_reason = WIFI_PS_PARAM_FAIL_OPERATION_NOT_SUPPORTED;
			/* Only legacy mode is supported */
			return -ENOTSUP;
		}
		break;
	case WIFI_PS_PARAM_LISTEN_INTERVAL:
		if (params->listen_interval < WIFI_LISTEN_INTERVAL_MIN ||
		    params->listen_interval > SIWX91X_LISTEN_INTERVAL_MAX) {
			params->fail_reason = WIFI_PS_PARAM_LISTEN_INTERVAL_RANGE_INVALID;
			return -EINVAL;
		}

		/* Converts the beacon unit to TU */
		sl_ps_profile.listen_interval =
			(params->listen_interval * SIWX91X_DEFAULT_BEACON_INTERVAL_MS);
		break;
	case WIFI_PS_PARAM_WAKEUP_MODE:
		if (params->wakeup_mode == WIFI_PS_WAKEUP_MODE_DTIM) {
			sl_ps_profile.dtim_aligned_type = 1;
			sl_ps_profile.listen_interval = 0;
		} else if (params->wakeup_mode == WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL) {
			sl_ps_profile.dtim_aligned_type = 0;
		}

		break;
	case WIFI_PS_PARAM_TIMEOUT:
		if (params->timeout_ms < DEFAULT_MONITOR_INTERVAL ||
		    params->timeout_ms > SIWX91X_MAX_MONITOR_INTERVAL_MS) {
			params->fail_reason = WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
			return -EINVAL;
		}

		sl_ps_profile.monitor_interval = (uint16_t)params->timeout_ms;
		break;

	case WIFI_PS_PARAM_EXIT_STRATEGY:
		if (params->exit_strategy == WIFI_PS_EXIT_EVERY_TIM) {
			ps_config.exit_strategy = ASSOCIATED_POWER_SAVE_LOW_LATENCY;
		} else if (params->exit_strategy == WIFI_PS_EXIT_CUSTOM_ALGO) {
			ps_config.exit_strategy = ASSOCIATED_POWER_SAVE;
		} else {
			params->fail_reason = WIFI_PS_PARAM_FAIL_INVALID_EXIT_STRATEGY;
			return -EINVAL;
		}

		if (sl_ps_profile.profile != HIGH_PERFORMANCE) {
			ret = siwx91x_validate_ps_state(WIFI_PS_ENABLED, ps_config.exit_strategy);
			if (ret < 0) {
				return ret;
			}

			sl_ps_profile.profile = ps_config.exit_strategy;
		} else {
			siwx91x_validate_ps_state(WIFI_PS_DISABLED, ps_config.exit_strategy);
			return 0;
		}

		break;
	default:
		params->fail_reason = WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
		return -ENOTSUP;
	}

	status = sl_wifi_set_performance_profile(&sl_ps_profile);
	if (status != SL_STATUS_OK) {
		params->fail_reason = WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
		return -EINVAL;
	}

	return 0;
}

static int siwx91x_get_power_save_config(const struct device *dev, struct wifi_ps_config *config)
{
	sl_wifi_performance_profile_t sl_ps_profile;
	sl_wifi_interface_t interface;
	sl_status_t status;

	__ASSERT(config, "config cannot be NULL");

	interface = sl_wifi_get_default_interface();
	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_CLIENT_INTERFACE) {
		LOG_ERR("Wi-Fi not in station mode");
		return -EIO;
	}

	status = sl_wifi_get_performance_profile(&sl_ps_profile);
	if (status != SL_STATUS_OK) {
		return -EINVAL;
	}

	switch (sl_ps_profile.profile) {
	case HIGH_PERFORMANCE:
		config->ps_params.enabled = WIFI_PS_DISABLED;
		/* Default power save state is FAST PSP */
		// config->ps_params.exit_strategy = WIFI_PS_EXIT_EVERY_TIM;
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

	if (sl_ps_profile.dtim_aligned_type == 1) {
		config->ps_params.wakeup_mode = WIFI_PS_WAKEUP_MODE_DTIM;
	} else if (sl_ps_profile.dtim_aligned_type == 0) {
		config->ps_params.wakeup_mode = WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL;
		/* converts TU to beacon unit */
		config->ps_params.listen_interval =
			(sl_ps_profile.listen_interval / SIWX91X_DEFAULT_BEACON_INTERVAL_MS);
	}

	/* Device supports only legacy power-save mode */
	config->ps_params.mode = WIFI_PS_MODE_LEGACY;
	config->ps_params.timeout_ms = sl_ps_profile.monitor_interval;

	return 0;
}

static const struct wifi_mgmt_ops siwx91x_mgmt = {
	.scan         = siwx91x_scan,
	.connect      = siwx91x_connect,
	.disconnect   = siwx91x_disconnect,
	.iface_status = siwx91x_status,
	.set_power_save = siwx91x_set_power_save,
	.get_power_save_config = siwx91x_get_power_save_config,
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
