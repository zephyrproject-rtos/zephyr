/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* System under test for the DHCPv4 client conformance suite.
 *
 * The suite is the server and looks at what the client puts on the wire. No
 * address is configured statically: obtaining one is what the exchange is
 * for.
 *
 * The client is started once and then left alone. Its discovers back off,
 * four seconds and then doubling, which is what paces the suite: each test
 * case picks up the next discover. Restarting the client to hurry that along
 * does not work, because a restart begins with a wait of its own and cuts off
 * whatever exchange was in progress.
 *
 * The suite that drives it lives in the net-tools repository, under
 * ttcn3/suites/dhcpv4.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dhcpv4_conformance, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
/* net_if.h first: dhcpv4.h takes a struct net_if * without declaring it. */
#include <zephyr/net/net_if.h>
#include <zephyr/net/dhcpv4.h>

int main(void)
{
	struct net_if *iface = net_if_get_default();

	if (iface == NULL) {
		LOG_ERR("No network interface");
		return -ENODEV;
	}

	net_dhcpv4_start(iface);

	LOG_INF("DHCPv4 client ready");

	while (true) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
