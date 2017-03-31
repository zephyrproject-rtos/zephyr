/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "15_4_hw_test"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <errno.h>

#include <net/net_if.h>
#include <net/net_core.h>

#include <ieee802154_settings.h>

static void setup_ipv6(struct net_if *iface)
{
	char hr_addr[NET_IPV6_ADDR_LEN];
	struct in6_addr addr;

	if (net_addr_pton(AF_INET6, CONFIG_NET_APP_MY_IPV6_ADDR, &addr)) {
		NET_ERR("Invalid address: %s", CONFIG_NET_APP_MY_IPV6_ADDR);
		return;
	}

	net_if_ipv6_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);

	NET_INFO("IPv6 address: %s",
		 net_addr_ntop(AF_INET6, &addr, hr_addr, NET_IPV6_ADDR_LEN));
}

void main(void)
{
	struct net_if *iface = net_if_get_default();

	if (ieee802154_sample_setup()) {
		NET_ERR("IEEE 802.15.4 setup failed");
	}

	setup_ipv6(iface);
}
