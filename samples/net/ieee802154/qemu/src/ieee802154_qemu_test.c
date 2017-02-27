/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "Setup"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <errno.h>

#include <net/net_if.h>
#include <net/net_core.h>

#define MCAST_IP6ADDR "ff84::2"

static void setup_device(void)
{
	char hr_addr[NET_IPV6_ADDR_LEN];
	struct net_if *iface;
	struct in6_addr addr;
	struct device *dev;

	dev = device_get_binding(CONFIG_IEEE802154_UPIPE_DRV_NAME);
	if (!dev) {
		NET_INFO("Cannot get UPIPE device\n");
		return;
	}

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		NET_INFO("Cannot get UPIPE network interface\n");
		return;
	}

	if (net_addr_pton(AF_INET6, CONFIG_NET_SAMPLES_MY_IPV6_ADDR, &addr)) {
		NET_ERR("Invalid address: %s", CONFIG_NET_SAMPLES_MY_IPV6_ADDR);
		return;
	}

	net_if_ipv6_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);

	NET_INFO("IPv6 address: %s",
		 net_addr_ntop(AF_INET6, &addr, hr_addr, NET_IPV6_ADDR_LEN));

	if (net_addr_pton(AF_INET6, MCAST_IP6ADDR, &addr)) {
		NET_ERR("Invalid address: %s", MCAST_IP6ADDR);
		return;
	}

	NET_INFO("802.15.4 device up and running\n");
}


void main(void)
{
	setup_device();

	return;
}
