/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>

#define NO_AUTO_START IS_ENABLED(CONFIG_NET_NATIVE_OFFLOADED_SOCKETS_NO_AUTO_START)

struct nsos_iface_fixture {
	struct net_if *iface;
	/* Interface state captured before any test runs */
	bool boot_no_auto_start;
	bool boot_admin_up;
};

ZTEST_F(nsos_iface, test_boot_state)
{
	zassert_equal(NO_AUTO_START, fixture->boot_no_auto_start);
	zassert_equal(!NO_AUTO_START, fixture->boot_admin_up);
}

ZTEST_F(nsos_iface, test_up_down)
{
	struct net_if *iface = fixture->iface;
	int sock;

	if (net_if_is_admin_up(iface)) {
		zassert_equal(-EALREADY, net_if_up(iface));
		zassert_equal(0, net_if_down(iface));
	}
	zassert_false(net_if_is_admin_up(iface));
	zassert_false(net_if_is_up(iface));

	zassert_equal(0, net_if_up(iface));
	zassert_true(net_if_is_admin_up(iface));
	zassert_true(net_if_is_up(iface));

	/* Offloaded sockets are usable once the interface is up */
	sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(sock >= 0);
	zassert_equal(0, zsock_close(sock));

	/* The flag only affects boot, it is not cleared or set by up/down */
	zassert_equal(NO_AUTO_START, net_if_flag_is_set(iface, NET_IF_NO_AUTO_START));
}

static void *nsos_iface_setup(void)
{
	static struct nsos_iface_fixture fixture;

	fixture.iface = net_if_get_default();
	zassert_not_null(fixture.iface);
	fixture.boot_no_auto_start = net_if_flag_is_set(fixture.iface, NET_IF_NO_AUTO_START);
	fixture.boot_admin_up = net_if_is_admin_up(fixture.iface);

	return &fixture;
}

ZTEST_SUITE(nsos_iface, NULL, nsos_iface_setup, NULL, NULL, NULL);
