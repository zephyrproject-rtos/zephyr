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

/* Dummy IP addresses */
#define INADDR_W91_WIFI_INIT           {{{1, 0, 0, 1}}}
#define IN6ADDR_W91_WIFI_INIT          \
	{{{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}}}

static int wifi_w91_init(const struct device *dev)
{
	LOG_INF("%s", __func__);
	ARG_UNUSED(dev);

	return 0;
}

static void wifi_w91_init_if(struct net_if *iface)
{
	LOG_INF("%s", __func__);

	/* For now set dummy address here to allow data propagation to L2 */
#if CONFIG_NET_IPV4
		struct in_addr ipv4 = INADDR_W91_WIFI_INIT;
		struct net_if_addr *ifaddr_ipv4 = net_if_ipv4_addr_add(
			iface, &ipv4, NET_ADDR_AUTOCONF, 0);

		if (!ifaddr_ipv4) {
			LOG_ERR("Failed to register IPv4 wifi address");
		}
#endif /* CONFIG_NET_IPV4 */
#if CONFIG_NET_IPV6
		struct in6_addr ipv6 = IN6ADDR_W91_WIFI_INIT;
		struct net_if_addr *ifaddr_ipv6 = net_if_ipv6_addr_add(
			iface, &ipv6, NET_ADDR_AUTOCONF, 0);

		if (!ifaddr_ipv6) {
			LOG_ERR("Failed to register IPv6 wifi address");
		}
#endif /* CONFIG_NET_IPV6 */
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

#define NET_W91_DEFINE(n)                                   \
                                                            \
	struct wifi_w91_data wifi_data_##n;                     \
                                                            \
	NET_DEVICE_DT_INST_DEFINE(n, wifi_w91_init,             \
		NULL, &wifi_data_##n, NULL,                         \
		CONFIG_TELINK_W91_IPC_DRIVERS_INIT_PRIORITY,        \
		&wifi_w91_driver_api, W91_WIFI_L2,                  \
		NET_L2_GET_CTX_TYPE(W91_WIFI_L2), W91_WIFI_L2_MTU);

DT_INST_FOREACH_STATUS_OKAY(NET_W91_DEFINE)
