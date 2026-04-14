/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024-2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT silabs_siwx91x_wifi

#include <zephyr/net/wifi_mgmt.h>
#include "siwx91x_nwp_bus.h"
#include "siwx91x_nwp_api.h"
#include "siwx91x_wifi.h"
#include "siwx91x_wifi_data.h"
#include "siwx91x_wifi_ps.h"
#include "siwx91x_wifi_ap.h"
#include "siwx91x_wifi_sta.h"
#include "siwx91x_wifi_scan.h"
#include "siwx91x_wifi_socket.h"

LOG_MODULE_REGISTER(siwx91x_wifi, CONFIG_WIFI_LOG_LEVEL);

BUILD_ASSERT(SLI_WIFI_SSID_LEN >= WIFI_SSID_MAX_LEN,
	     "SSID lengths mismatch");
BUILD_ASSERT(SLI_WIFI_PSK_LEN == WIFI_PSK_MAX_LEN,
	     "PSK lengths mismatch");
BUILD_ASSERT(SLI_WIFI_HARDWARE_ADDRESS_LENGTH == WIFI_MAC_ADDR_LEN,
	     "Hardware address lengths mismatch");
#ifndef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD

static int siwx91x_wifi_get_config(const struct device *dev,
				   enum ethernet_config_type type,
				   struct ethernet_config *config)
{
	if (type == ETHERNET_CONFIG_TYPE_EXTRA_TX_PKT_HEADROOM) {
		config->extra_tx_pkt_headroom = sizeof(struct siwx91x_frame_desc);
	}
	/* FIXME we could also manage ETHERNET_CONFIG_TYPE_RX_CHECKSUM_SUPPORT and
	 * ETHERNET_CONFIG_TYPE_TX_CHECKSUM_SUPPORT
	 */
	return 0;
}
#endif

static int siwx91x_wifi_mode(const struct device *dev, struct wifi_mode_info *mode)
{
	const struct siwx91x_wifi_config *config = dev->config;
	struct siwx91x_wifi_data *data = dev->data;
	int ret;

	switch (mode->oper) {
	case WIFI_MGMT_GET:
		mode->mode = data->operating_mode;
	case WIFI_MGMT_SET:
		ret = siwx91x_nwp_reset(config->nwp_dev, mode->mode, false, 0);
		if (ret) {
			return ret;
		}
		siwx91x_nwp_set_band(config->nwp_dev, SL_WIFI_BAND_MODE_2_4GHZ);
		siwx91x_nwp_wifi_init(config->nwp_dev);
		if (mode->mode == WIFI_SOFTAP_MODE) {
			siwx91x_nwp_set_region_ap(config->nwp_dev);
		} else { /* WIFI_STA_MODE */
			siwx91x_nwp_set_region_sta(config->nwp_dev, SL_WIFI_DEFAULT_REGION);
		}
		siwx91x_nwp_set_config(config->nwp_dev, SLI_WIFI_CONFIG_RTS_THRESHOLD, 2346);
		siwx91x_nwp_set_sta_config(config->nwp_dev);
		/* FIXME: Set max Tx Power for scan and join */
		data->operating_mode = mode->mode;
	default:
		__ASSERT(0, "Corrupted argument");
	}
	return 0;
}

static void siwx91x_wifi_ethernet_init(struct net_if *iface)
{
	struct ethernet_context *eth_ctx;

	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD)) {
		return;
	}
	eth_ctx = net_if_l2_data(iface);
	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;
	ethernet_init(iface);
	net_if_dormant_on(iface);
}

static void siwx91x_wifi_iface_init(struct net_if *iface)
{
	const struct siwx91x_wifi_config *config = net_if_get_device(iface)->config;
	struct siwx91x_wifi_data *data = net_if_get_device(iface)->data;
	uint8_t mac_addr[NET_ETH_ADDR_LEN];

	data->iface = iface;
	siwx91x_nwp_set_band(config->nwp_dev, SL_WIFI_BAND_MODE_2_4GHZ);
	siwx91x_nwp_wifi_init(config->nwp_dev);
	siwx91x_nwp_set_region_sta(config->nwp_dev, SL_WIFI_DEFAULT_REGION);
	siwx91x_nwp_set_config(config->nwp_dev, SLI_WIFI_CONFIG_RTS_THRESHOLD, 2346);
	siwx91x_nwp_set_sta_config(config->nwp_dev);
	siwx91x_nwp_get_mac_address(config->nwp_dev, mac_addr);
	net_if_set_link_addr(iface, mac_addr, sizeof(mac_addr), NET_LINK_ETHERNET);
	siwx91x_sock_init(iface);
	siwx91x_wifi_ethernet_init(iface);
}

static int siwx91x_wifi_init(const struct device *dev)
{
	const struct siwx91x_wifi_config *config = dev->config;
	struct siwx91x_wifi_data *data =  dev->data;

	if (!device_is_ready(config->nwp_dev)) {
		return -ENODEV;
	}
	siwx91x_nwp_register_wifi(config->nwp_dev, &data->nwp_ops);

	return 0;
}

static const struct wifi_mgmt_ops siwx91x_wifi_mgmt = {
	.mode = siwx91x_wifi_mode,
	.scan = siwx91x_wifi_scan,
	.connect = siwx91x_wifi_connect,
	.disconnect = siwx91x_wifi_disconnect,
	.ap_enable = siwx91x_ap_enable,
	.ap_disable = siwx91x_ap_disable,
	.ap_sta_disconnect = siwx91x_ap_sta_disconnect,
	.ap_config_params = siwx91x_ap_config_params,
	.set_twt = siwx91x_wifi_set_twt,
	.set_power_save	= siwx91x_wifi_set_power_save,
	.get_power_save_config = siwx91x_wifi_get_power_save_config,
};

static const struct net_wifi_mgmt_offload siwx91x_wifi_api = {
	.wifi_iface.iface_api.init = siwx91x_wifi_iface_init,
#ifdef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD
	.wifi_iface.get_type = siwx91x_sock_get_type,
	.wifi_iface.alloc = siwx91x_sock_alloc,
#else
	.wifi_iface.get_config = siwx91x_wifi_get_config,
	.wifi_iface.send = siwx91x_wifi_send,
#endif
	.wifi_mgmt_api = &siwx91x_wifi_mgmt,
};

static const struct siwx91x_wifi_config siwx91x_wifi_config = {
	.nwp_dev = DEVICE_DT_GET(DT_INST_PARENT(0)),
	.scan_tx_power = DT_INST_PROP(0, wifi_max_tx_pwr_scan),
	.join_tx_power = DT_INST_PROP(0, wifi_max_tx_pwr_join),
};

static struct siwx91x_wifi_data siwx91x_wifi_data = {
	.nwp_ops.on_scan_results = siwx91x_wifi_on_scan_results,
#ifndef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD
	.nwp_ops.on_rx = siwx91x_wifi_on_rx,
#endif
	.nwp_ops.on_sock_select = siwx91x_sock_on_select,
	.ap_idle_timeout = UINT8_MAX,
	.ap_max_num_sta = 4,
	.ps_exit_strategy = WIFI_PS_EXIT_EVERY_TIM,
	.ps_wakeup_mode = WIFI_PS_WAKEUP_MODE_DTIM,
};

#ifdef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD
NET_DEVICE_DT_INST_OFFLOAD_DEFINE(0, siwx91x_wifi_init, NULL, &siwx91x_wifi_data,
				  &siwx91x_wifi_config, CONFIG_WIFI_INIT_PRIORITY,
				  &siwx91x_wifi_api, NET_ETH_MTU);
#else
ETH_NET_DEVICE_DT_INST_DEFINE(0, siwx91x_wifi_init, NULL, &siwx91x_wifi_data,
			      &siwx91x_wifi_config, CONFIG_WIFI_INIT_PRIORITY,
			      &siwx91x_wifi_api, NET_ETH_MTU);
#endif
