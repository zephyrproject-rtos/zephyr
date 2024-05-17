/*
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT         telink_w91_wifi

#include "wifi_w91.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(w91_wifi, CONFIG_WIFI_LOG_LEVEL);

#include <zephyr/net/wifi_mgmt.h>

static int wifi_w91_init(const struct device *dev)
{
	struct wifi_w91_data *data = dev->data;

	ipc_based_driver_init(&data->ipc);

	return 0;
}

static void wifi_w91_init_if(struct net_if *iface)
{
	LOG_INF("%s", __func__);
}

static int wifi_w91_scan(const struct device *dev, scan_result_cb_t cb)
{
	LOG_INF("%s", __func__);
	return 0;
}

static int wifi_w91_connect(const struct device *dev,
	struct wifi_connect_req_params *params)
{
	LOG_INF("%s", __func__);
	ARG_UNUSED(dev);
	ARG_UNUSED(params);

	return 0;
}

static int wifi_w91_disconnect(const struct device *dev)
{
	LOG_INF("%s", __func__);
	ARG_UNUSED(dev);

	return 0;
}

static int wifi_w91_ap_enable(const struct device *dev,
	struct wifi_connect_req_params *params)
{
	LOG_INF("%s", __func__);
	ARG_UNUSED(dev);
	ARG_UNUSED(params);

	return 0;
}

static int wifi_w91_ap_disable(const struct device *dev)
{
	LOG_INF("%s", __func__);
	ARG_UNUSED(dev);

	return 0;
}

static const struct net_wifi_mgmt_offload wifi_w91_driver_api = {
	.wifi_iface.iface_api.init  = wifi_w91_init_if,
	.scan                       = wifi_w91_scan,
	.connect                    = wifi_w91_connect,
	.disconnect                 = wifi_w91_disconnect,
	.ap_enable                  = wifi_w91_ap_enable,
	.ap_disable                 = wifi_w91_ap_disable,
};

#define NET_W91_DEFINE(n)                                       \
                                                                \
	static const struct wifi_w91_config wifi_config_##n = {     \
		.instance_id = n,                                       \
	};                                                          \
                                                                \
	static struct wifi_w91_data wifi_data_##n;                  \
                                                                \
	NET_DEVICE_DT_INST_DEFINE(n, wifi_w91_init,                 \
		NULL, &wifi_data_##n, &wifi_config_##n,                 \
		CONFIG_TELINK_W91_IPC_DRIVERS_INIT_PRIORITY,            \
		&wifi_w91_driver_api, W91_WIFI_L2,                      \
		NET_L2_GET_CTX_TYPE(W91_WIFI_L2), NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(NET_W91_DEFINE)
