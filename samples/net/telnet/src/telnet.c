/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_telnet_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <errno.h>
#include <stdio.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>

#include "net_sample_common.h"

#if defined(CONFIG_NET_IPV6)
#define MCAST_IP6ADDR "ff84::2"

static void setup_ipv6(void)
{
	struct in6_addr addr;
	struct net_if *iface = net_if_get_default();

	if (net_addr_pton(AF_INET6, MCAST_IP6ADDR, &addr)) {
		LOG_ERR("Invalid address: %s", MCAST_IP6ADDR);
		return;
	}

	net_if_ipv6_maddr_add(iface, &addr);
}
#else
#define setup_ipv6(...)
#endif /* CONFIG_NET_IPV6 */

int main(void)
{
	LOG_INF("Starting Telnet sample");

	wait_for_network();
	setup_ipv6();
	return 0;
}
