/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024-2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT silabs_siwx91x_wifi

#include <zephyr/version.h>

#include <nwp.h>
#include "siwx91x_wifi.h"
#include "siwx91x_wifi_ap.h"
#include "siwx91x_wifi_ps.h"
#include "siwx91x_wifi_scan.h"
#include "siwx91x_wifi_socket.h"
#include "siwx91x_wifi_sta.h"

#include "sl_rsi_utility.h"
#include "sl_wifi_callback_framework.h"

#define SIWX91X_DRIVER_VERSION KERNEL_VERSION_STRING
#define SIWX91X_MAX_RTS_THRESHOLD 2347
#define MAX_24GHZ_CHANNELS 14

LOG_MODULE_REGISTER(siwx91x_wifi);

NET_BUF_POOL_FIXED_DEFINE(siwx91x_tx_pool, 1, _NET_ETH_MAX_FRAME_SIZE, 0, NULL);

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

int siwx91x_status(const struct device *dev, struct wifi_iface_status *status)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	sl_si91x_rsp_wireless_info_t wlan_info = { };
	struct siwx91x_dev *sidev = dev->data;
	sl_wifi_mfp_mode_t mfp;
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

		/* The Wiseconnect wireless_mode values match the values expected by
		 * Zephyr's link_mode, even though the enum type differs.
		 */
		status->link_mode = wlan_info.wireless_mode;
		status->iface_mode = WIFI_MODE_INFRA;
		status->channel = wlan_info.channel_number;
		status->twt_capable = true;

		ret = sl_wifi_get_mfp(interface, &mfp);
		if (ret) {
			LOG_ERR("Failed to get MFP configuration: 0x%x", ret);
			return -EINVAL;
		}
		/* The Wiseconnect mfp values match the values expected by
		 * Zephyr's mfp, even though the enum type differs.
		 */
		status->mfp = mfp;

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

static int siwx91x_set_max_tx_power(const struct siwx91x_config *siwx91x_cfg)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	sl_wifi_max_tx_power_t max_tx_power = {
		.scan_tx_power = siwx91x_cfg->scan_tx_power,
		.join_tx_power = siwx91x_cfg->join_tx_power,
	};

	return sl_wifi_set_max_tx_power(interface, max_tx_power);
}

static int siwx91x_mode(const struct device *dev, struct wifi_mode_info *mode)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	const struct siwx91x_config *siwx91x_cfg = dev->config;
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

			ret = siwx91x_set_max_tx_power(siwx91x_cfg);
			if (ret != SL_STATUS_OK) {
				LOG_ERR("Failed to set max tx power:%x", ret);
				return -EINVAL;
			}
		}
		sidev->state = WIFI_STATE_INACTIVE;
	}

	return 0;
}

#ifdef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE

static int siwx91x_send(const struct device *dev, struct net_pkt *pkt)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	size_t pkt_len = net_pkt_get_len(pkt);
	struct net_buf *buf;
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
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	sl_wifi_statistics_t statistics = { };
	int ret;

	ARG_UNUSED(dev);
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
	int ret;

	__ASSERT(params, "params cannot be NULL");

	if (sidev->state == WIFI_STATE_INTERFACE_DISABLED) {
		return -EIO;
	}

	ret = sl_wifi_get_firmware_version(&fw_version);
	if (ret) {
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

static int map_sdk_region_to_zephyr_channel_info(const sli_si91x_set_region_ap_request_t *sdk_reg,
						 struct wifi_reg_chan_info *z_chan_info,
						 size_t *num_channels)
{
	uint8_t first_channel = sdk_reg->channel_info[0].first_channel;
	uint8_t channel;
	uint16_t freq;

	*num_channels = sdk_reg->channel_info[0].no_of_channels;
	if (*num_channels > MAX_24GHZ_CHANNELS) {
		return -EOVERFLOW;
	}

	for (int idx = 0; idx < *num_channels; idx++) {
		channel = first_channel + idx;
		freq = 2407 + channel * 5;

		if (freq > 2472) {
			freq = 2484; /* channel 14 */
		}

		z_chan_info[idx].center_frequency = freq;
		z_chan_info[idx].max_power = sdk_reg->channel_info[0].max_tx_power;
		z_chan_info[idx].supported = 1;
		z_chan_info[idx].passive_only = 0;
		z_chan_info[idx].dfs = 0;
	}

	return 0;
}

static int siwx91x_wifi_reg_domain(const struct device *dev, struct wifi_reg_domain *reg_domain)
{
	const sli_si91x_set_region_ap_request_t *sdk_reg = NULL;
	sl_wifi_operation_mode_t oper_mode = sli_get_opermode();
	sl_wifi_region_code_t region_code;
	const char *country_code;
	int ret;

	__ASSERT(reg_domain, "reg_domain cannot be NULL");

	if (reg_domain->oper == WIFI_MGMT_SET) {
		region_code = siwx91x_map_country_code_to_region(reg_domain->country_code);
		ret = sl_si91x_set_device_region(oper_mode, SL_WIFI_BAND_MODE_2_4GHZ, region_code);
		if (ret) {
			LOG_ERR("Failed to set device region: %x", ret);
			return -EINVAL;
		}

		if (region_code == SL_WIFI_DEFAULT_REGION) {
			siwx91x_store_country_code(DEFAULT_COUNTRY_CODE);
			LOG_INF("Country code not supported, using default region");
		} else {
			siwx91x_store_country_code(reg_domain->country_code);
		}
	} else if (reg_domain->oper == WIFI_MGMT_GET) {
		country_code = siwx91x_get_country_code();
		memcpy(reg_domain->country_code, country_code, WIFI_COUNTRY_CODE_LEN);
		region_code = siwx91x_map_country_code_to_region(country_code);

		sdk_reg = siwx91x_find_sdk_region_table(region_code);
		if (!sdk_reg) {
			return -ENOENT;
		}

		ret = map_sdk_region_to_zephyr_channel_info(sdk_reg, reg_domain->chan_info,
							    &reg_domain->num_channels);
		if (ret) {
			return ret;
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static void siwx91x_iface_init(struct net_if *iface)
{
	const struct siwx91x_config *siwx91x_cfg = iface->if_dev->dev->config;
	struct siwx91x_dev *sidev = iface->if_dev->dev->data;
	int ret;

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

	ret = siwx91x_set_max_tx_power(siwx91x_cfg);
	if (ret != SL_STATUS_OK) {
		LOG_ERR("Failed to set max tx power:%x", ret);
		return;
	}

	ret = sl_wifi_get_mac_address(SL_WIFI_CLIENT_INTERFACE, &sidev->macaddr);
	if (ret) {
		LOG_ERR("sl_wifi_get_mac_address(): %#04x", ret);
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

int siwx91x_get_rts_threshold(const struct device *dev, unsigned int *rts_threshold)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	struct siwx91x_dev *sidev = dev->data;
	unsigned short rts_val;
	int ret;

	__ASSERT(rts_threshold, "rts_threshold cannot be NULL");

	if (sidev->state == WIFI_STATE_INTERFACE_DISABLED) {
		LOG_ERR("Command given in invalid state");
		return -EINVAL;
	}

	ret = sl_wifi_get_rts_threshold(interface, &rts_val);
	if (ret) {
		LOG_ERR("Failed to get RTS threshold: 0x%x", ret);
		return -EIO;
	}
	*rts_threshold = rts_val;

	return 0;
}

int siwx91x_set_rts_threshold(const struct device *dev, unsigned int rts_threshold)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	struct siwx91x_dev *sidev = dev->data;
	int ret;

	__ASSERT(sidev, "sidev cannot be NULL");

	if (sidev->state == WIFI_STATE_INTERFACE_DISABLED) {
		LOG_ERR("Command given in invalid state");
		return -EINVAL;
	}

	if (rts_threshold > SIWX91X_MAX_RTS_THRESHOLD) {
		LOG_ERR("RTS threshold out of range: %u", rts_threshold);
		return -EINVAL;
	}

	ret = sl_wifi_set_rts_threshold(interface, rts_threshold);
	if (ret) {
		LOG_ERR("Failed to set RTS threshold: 0x%x", ret);
		return -EIO;
	}

	return 0;
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
	.get_rts_threshold	= siwx91x_get_rts_threshold,
	.set_rts_threshold	= siwx91x_set_rts_threshold,
#if defined(CONFIG_NET_STATISTICS_WIFI)
	.get_stats		= siwx91x_stats,
#endif
	.get_version		= siwx91x_get_version,
	.reg_domain		= siwx91x_wifi_reg_domain,
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

static const struct siwx91x_config siwx91x_cfg = {
	.scan_tx_power = DT_INST_PROP(0, wifi_max_tx_pwr_scan),
	.join_tx_power = DT_INST_PROP(0, wifi_max_tx_pwr_join),
};

static struct siwx91x_dev sidev = {
	.ps_params.enabled = WIFI_PS_DISABLED,
	.ps_params.exit_strategy = WIFI_PS_EXIT_EVERY_TIM,
	.ps_params.wakeup_mode = WIFI_PS_WAKEUP_MODE_DTIM,
	.max_num_sta = CONFIG_WIFI_MGMT_AP_MAX_NUM_STA,
};

#ifdef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE
ETH_NET_DEVICE_DT_INST_DEFINE(0, siwx91x_dev_init, NULL, &sidev, &siwx91x_cfg,
			      CONFIG_WIFI_INIT_PRIORITY, &siwx91x_api, NET_ETH_MTU);
#else
NET_DEVICE_DT_INST_OFFLOAD_DEFINE(0, siwx91x_dev_init, NULL, &sidev, &siwx91x_cfg,
				  CONFIG_WIFI_INIT_PRIORITY, &siwx91x_api, NET_ETH_MTU);
#endif
