/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* System under test for the IGMP conformance suite.
 *
 * Everything the suite looks at is what the stack does when a group is joined
 * or left and when a querier asks, and the stack does all of that on its own.
 * What the application has to provide is the joining and leaving: it joins
 * one group, keeps it for a while, leaves it, waits, and starts again, so that
 * a test that wants to see a join or a leave only has to wait for the next.
 *
 * This one runs on its own interface, with no address on the Linux side of it,
 * so that nothing but the suite answers. See net-tools/zeth-l2.conf.
 *
 * The suite that drives it lives in the net-tools repository, under
 * ttcn3/suites/igmp.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(igmp_conformance, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/igmp.h>
#include <zephyr/net/net_if.h>

/* The group has to agree with tsp_group in the suite's configuration, and the
 * times with what the suite's test cases count on. The group is kept long
 * enough for a test to ask about it and be answered, and left long enough for
 * the tester to see the leave before the next join.
 */
#define GROUP		"239.0.2.1"
#define JOINED_FOR	K_SECONDS(6)
#define LEFT_FOR	K_SECONDS(4)

int main(void)
{
	struct net_if *iface = net_if_get_default();
	struct net_in_addr group;
	int ret;

	if (iface == NULL) {
		LOG_ERR("No network interface");
		return -ENODEV;
	}

	if (net_addr_pton(NET_AF_INET, GROUP, &group) < 0) {
		LOG_ERR("Cannot parse the group address");
		return -EINVAL;
	}

	/* net_config has already brought the interface up and set its
	 * address: it waits for both before main() runs.
	 */

	LOG_INF("IGMP host ready");

	while (true) {
		ret = net_ipv4_igmp_join(iface, &group, NULL);
		if (ret < 0) {
			LOG_ERR("Cannot join %s (%d)", GROUP, ret);
		}

		k_sleep(JOINED_FOR);

		ret = net_ipv4_igmp_leave(iface, &group);
		if (ret < 0) {
			LOG_ERR("Cannot leave %s (%d)", GROUP, ret);
		}

		k_sleep(LEFT_FOR);
	}

	return 0;
}
