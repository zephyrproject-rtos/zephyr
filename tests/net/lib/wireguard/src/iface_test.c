/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/net_if.h>

ZTEST(wireguard_iface, test_ctrl_iface_link_addr)
{
	int index = net_if_get_by_name("wg_ctrl");
	struct net_if *iface;
	struct net_linkaddr *lladdr;

	zassert_true(index > 0, "No wg_ctrl interface");

	iface = net_if_get_by_index(index);
	zassert_not_null(iface, "No interface %d", index);

	/* Adding the first peer takes the control interface up, and taking an
	 * interface up asserts that it has a link address.
	 */
	lladdr = net_if_get_link_addr(iface);
	zassert_not_null(lladdr, "No link address");
	zassert_true(lladdr->len > 0, "Empty link address");
}

ZTEST_SUITE(wireguard_iface, NULL, NULL, NULL, NULL, NULL);
