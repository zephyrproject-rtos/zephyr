/*
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_w91_wifi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(w91_wifi, CONFIG_WIFI_LOG_LEVEL);

#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/device.h>

static struct wifi_w91_runtime wifi_w91_data;

struct wifi_w91_runtime {};

static int wifi_w91_init(const struct device *dev)
{
	return 0;
}

static const struct net_wifi_mgmt_offload wifi_w91_driver_api = {};

NET_DEVICE_DT_INST_DEFINE(0,
		wifi_w91_init, NULL,
		&wifi_w91_data, NULL, CONFIG_ETH_INIT_PRIORITY,
		&wifi_w91_driver_api, ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);
