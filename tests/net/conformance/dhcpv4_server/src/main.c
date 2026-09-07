/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* System under test for the DHCPv4 server conformance suite.
 *
 * The application starts the server on the default interface and then does
 * nothing: everything the suite looks at is the server answering, so there is
 * no traffic to generate here.
 *
 * The pool is deliberately larger than the default. Each test case in the
 * suite appears as a client of its own so that no test depends on what an
 * earlier one left behind, and every one of them that completes an exchange
 * takes an address that is not given back.
 *
 * The suite that drives it lives in the net-tools repository, under
 * ttcn3/suites/dhcpv4_server.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dhcpv4_server_conformance, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>

/* The lowest address the server hands out. Above the addresses the test
 * network itself uses, so that nothing in the pool is already taken.
 */
#define POOL_BASE	"192.0.2.100"

int main(void)
{
	struct net_if *iface = net_if_get_default();
	struct net_in_addr base;
	int ret;

	if (iface == NULL) {
		LOG_ERR("No network interface");
		return -ENODEV;
	}

	if (net_addr_pton(NET_AF_INET, POOL_BASE, &base) < 0) {
		LOG_ERR("Cannot parse the pool base address");
		return -EINVAL;
	}

	/* The addresses are configured during initialisation, so the interface
	 * is usually up before main() runs. Only wait when it is not: waiting
	 * unconditionally would block until the timeout, because the event has
	 * already been and gone.
	 */
	if (!net_if_is_up(iface)) {
		(void)net_mgmt_event_wait_on_iface(iface, NET_EVENT_IF_UP, NULL,
						   NULL, NULL, K_SECONDS(10));
	}

	ret = net_dhcpv4_server_start(iface, &base);
	if (ret < 0) {
		LOG_ERR("Cannot start the DHCPv4 server (%d)", ret);
		return ret;
	}

	LOG_INF("DHCPv4 server ready");

	k_sleep(K_FOREVER);

	return 0;
}
