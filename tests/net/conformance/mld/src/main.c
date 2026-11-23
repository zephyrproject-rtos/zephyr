/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* System under test for the MLD conformance suite.
 *
 * Everything the suite looks at is what the stack does when a multicast
 * address is listened to or left and when a querier asks, and the stack does
 * all of that on its own. What the application has to provide is the joining
 * and leaving: it joins one address, keeps it for a while, leaves it, waits,
 * and starts again, so that a test that wants to see a join or a leave only
 * has to wait for the next.
 *
 * This one runs on its own interface, with no address on the Linux side of it,
 * so that nothing but the suite answers. See net-tools/zeth-l2.conf.
 *
 * The suite that drives it lives in the net-tools repository, under
 * ttcn3/suites/mld.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mld_conformance, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/mld.h>
#include <zephyr/net/net_if.h>

/* The address has to agree with tsp_group in the suite's configuration, and
 * the times with what the suite's test cases count on. The address is kept
 * long enough for a test to ask about it and be answered, and left long enough
 * for the tester to see the leave before the next join.
 */
#define GROUP		"ff05::db8:1"
#define JOINED_FOR	K_SECONDS(6)
#define LEFT_FOR	K_SECONDS(4)

int main(void)
{
	struct net_if *iface = net_if_get_default();
	struct net_in6_addr group;
	int ret;

	if (iface == NULL) {
		LOG_ERR("No network interface");
		return -ENODEV;
	}

	if (net_addr_pton(NET_AF_INET6, GROUP, &group) < 0) {
		LOG_ERR("Cannot parse the group address");
		return -EINVAL;
	}

	/* net_config has already brought the interface up and set its global
	 * address. MLD messages are sent from the link-local address, and a
	 * report sent before that address has passed Duplicate Address
	 * Detection goes out from the unspecified address, which is not what
	 * the suite is about, so wait for it.
	 */
	while (net_if_ipv6_get_ll(iface, NET_ADDR_PREFERRED) == NULL) {
		k_sleep(K_MSEC(100));
	}

	LOG_INF("MLD listener ready");

	while (true) {
		ret = net_ipv6_mld_join(iface, &group);
		if (ret < 0) {
			LOG_ERR("Cannot join %s (%d)", GROUP, ret);
		}

		k_sleep(JOINED_FOR);

		ret = net_ipv6_mld_leave(iface, &group);
		if (ret < 0) {
			LOG_ERR("Cannot leave %s (%d)", GROUP, ret);
		}

		k_sleep(LEFT_FOR);
	}

	return 0;
}
