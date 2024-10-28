/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <zephyr/sys/printk.h>
#include <zephyr/linker/sections.h>

#include <zephyr/ztest.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_l2.h>

static struct in_addr test_addr_ipv4 = { { { 192, 0, 2, 1 } } };
static struct in6_addr test_addr_ipv6 = { { {
	0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1
} } };

/* Dummy socket creator for socket-offloaded ifaces */
int offload_socket(int family, int type, int proto)
{
	return -1;
}

/* Dummy offload API for net-offloaded ifaces */
struct net_offload net_offload_api;

/* Dummy init function for socket-offloaded ifaces */
static void sock_offload_l2_iface_init(struct net_if *iface)
{
	/* This must be called, and the passed-in socket creator cannot be NULL,
	 * or the iface will not be recognized as offloaded
	 */
	net_if_socket_offload_set(iface, offload_socket);
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_flag_set(iface, NET_IF_IPV4);
	net_if_flag_set(iface, NET_IF_IPV6);
}

/* Dummy init function for net-offloaded ifaces */
static void net_offload_l2_iface_init(struct net_if *iface)
{
	/* Reviewers: Is there a better way to do this?
	 * I couldn't find any actual examples in the source
	 */
	iface->if_dev->offload = &net_offload_api;
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_flag_set(iface, NET_IF_IPV4);
	net_if_flag_set(iface, NET_IF_IPV6);
}

/* Tracks the total number of ifaces that are up (theoretically). */
atomic_t up_count = ATOMIC_INIT(0);

/* Tracks the total number of times that the offload_impl_enable callback was called. */
atomic_t call_count = ATOMIC_INIT(0);

/* Expected return value from offload_impl_enable */
atomic_t retval = ATOMIC_INIT(0);

/* Functionality under test */
static int offload_impl_enable(const struct net_if *iface, bool enabled)
{
	atomic_inc(&call_count);
	if (enabled) {
		atomic_inc(&up_count);
	} else {
		atomic_dec(&up_count);
	}
	return atomic_get(&retval);
}

/* Net-dev APIs for L2s with offloaded sockets, with and without .enable */
static struct offloaded_if_api sock_offloaded_impl_api = {
	.iface_api.init = sock_offload_l2_iface_init,
	.enable = offload_impl_enable
};

static struct offloaded_if_api sock_offloaded_no_impl_api = {
	.iface_api.init = sock_offload_l2_iface_init
};

/* Net-dev APIs for L2s that are net-offloaded, with and without .enable */
static struct offloaded_if_api net_offloaded_impl_api = {
	.iface_api.init = net_offload_l2_iface_init,
	.enable = offload_impl_enable
};

static struct offloaded_if_api net_offloaded_no_impl_api = {
	.iface_api.init = net_offload_l2_iface_init
};


/* Socket-offloaded netdevs, with and without .enable */
NET_DEVICE_OFFLOAD_INIT(sock_offload_test_impl, "sock_offload_test_impl",
			NULL, NULL, NULL, NULL,
			CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			&sock_offloaded_impl_api, 0);

NET_DEVICE_OFFLOAD_INIT(sock_offload_test_no_impl, "sock_offload_test_no_impl",
			NULL, NULL, NULL, NULL,
			CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			&sock_offloaded_no_impl_api, 0);

/* Net-offloaded netdevs, with and without .enable */
NET_DEVICE_OFFLOAD_INIT(net_offload_test_impl, "net_offload_test_impl",
			NULL, NULL, NULL, NULL,
			CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			&net_offloaded_impl_api, 0);

NET_DEVICE_OFFLOAD_INIT(net_offload_test_no_impl, "net_offload_test_no_impl",
			NULL, NULL, NULL, NULL,
			CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			&net_offloaded_no_impl_api, 0);

static void net_offloaded_netdev_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Default to successful return value */
	atomic_set(&retval, 0);

	/* Reset all ifaces */
	net_if_down(NET_IF_GET(sock_offload_test_impl, 0));
	net_if_down(NET_IF_GET(sock_offload_test_no_impl, 0));
	net_if_down(NET_IF_GET(net_offload_test_impl, 0));
	net_if_down(NET_IF_GET(net_offload_test_impl, 0));

	/* Reset counters */
	atomic_set(&call_count, 0);
	atomic_set(&up_count, 0);
}

ZTEST(net_offloaded_netdev, test_up_down_sock_off_impl)
{
	struct net_if *test_iface = NET_IF_GET(sock_offload_test_impl, 0);

	/* Verify iface under test is down before test */
	zassert_false(net_if_is_admin_up(test_iface),
			"Iface under test must be admin-down before test");

	/* Bring iface up. */
	(void)net_if_up(test_iface);

	/* Verify that a single iface went up once (according to the enable callback) */
	zassert_equal(atomic_get(&call_count), 1,
			"Bad transition-count, offload_impl_enable not called correctly");
	zassert_equal(atomic_get(&up_count), 1,
			"Bad up-count, offload_impl_enable not called correctly");
	zassert_true(net_if_is_admin_up(test_iface),
			"Iface under test should be up after net_if_up");

	/* Bring iface down */
	(void)net_if_down(test_iface);

	/* Verify that a single iface went down once (according to the enable callback)*/
	zassert_equal(atomic_get(&call_count), 2,
			"Bad transition-count, offload_impl_enable not called correctly");
	zassert_equal(atomic_get(&up_count), 0,
			"Bad up-count, offload_impl_enable not called correctly");
	zassert_false(net_if_is_admin_up(test_iface),
			"Iface under test should be down after net_if_down");
}

ZTEST(net_offloaded_netdev, test_up_down_sock_off_no_impl)
{
	struct net_if *test_iface = NET_IF_GET(sock_offload_test_no_impl, 0);

	/* Verify iface under test is down before test */
	zassert_false(net_if_is_admin_up(test_iface),
			"Iface under test must be admin-down before test");

	/* Bring iface up */
	(void)net_if_up(test_iface);

	/* Verify that the iface went up, but callbacks were not fired*/
	zassert_equal(atomic_get(&call_count), 0,
			"offload_impl_enable was called unexpectedly");
	zassert_equal(atomic_get(&up_count), 0,
			"offload_impl_enable was called unexpectedly");
	zassert_true(net_if_is_admin_up(test_iface),
			"Iface under test should be up after net_if_up");

	/* Bring iface down */
	(void)net_if_down(test_iface);

	/* Verify that the iface went down, but callbacks were not fired*/
	zassert_equal(atomic_get(&call_count), 0,
			"offload_impl_enable was called unexpectedly");
	zassert_equal(atomic_get(&up_count), 0,
			"offload_impl_enable was called unexpectedly");
	zassert_false(net_if_is_admin_up(test_iface),
			"Iface under test should be down after net_if_down");
}

ZTEST(net_offloaded_netdev, test_up_down_net_off_impl)
{
	struct net_if *test_iface = NET_IF_GET(net_offload_test_impl, 0);

	/* Verify iface under test is down before test */
	zassert_false(net_if_is_admin_up(test_iface),
			"Iface under test must be admin-down before test");

	/* Bring iface up. */
	(void)net_if_up(test_iface);

	/* Verify that a single iface went up once (according to the enable callback) */
	zassert_equal(atomic_get(&call_count), 1,
			"Bad transition-count, offload_impl_enable not called correctly");
	zassert_equal(atomic_get(&up_count), 1,
			"Bad up-count, offload_impl_enable not called correctly");
	zassert_true(net_if_is_admin_up(test_iface),
			"Iface under test should be up after net_if_up");

	/* Bring iface down */
	(void)net_if_down(test_iface);

	/* Verify that a single iface went down once (according to the enable callback)*/
	zassert_equal(atomic_get(&call_count), 2,
			"Bad transition-count, offload_impl_enable not called correctly");
	zassert_equal(atomic_get(&up_count), 0,
			"Bad up-count, offload_impl_enable not called correctly");
	zassert_false(net_if_is_admin_up(test_iface),
			"Iface under test should be down after net_if_down");
}

ZTEST(net_offloaded_netdev, test_up_down_net_off_no_impl)
{
	struct net_if *test_iface = NET_IF_GET(net_offload_test_no_impl, 0);

	/* Verify iface under test is down before test */
	zassert_false(net_if_is_admin_up(test_iface),
			"Iface under test must be admin-down before test");

	/* Bring iface up */
	(void)net_if_up(test_iface);

	/* Verify that the iface went up, but callbacks were not fired*/
	zassert_equal(atomic_get(&call_count), 0,
			"offload_impl_enable was called unexpectedly");
	zassert_equal(atomic_get(&up_count), 0,
			"offload_impl_enable was called unexpectedly");
	zassert_true(net_if_is_admin_up(test_iface),
			"Iface under test should be up after net_if_up");

	/* Bring iface down */
	(void)net_if_down(test_iface);

	/* Verify that the iface went down, but callbacks were not fired*/
	zassert_equal(atomic_get(&call_count), 0,
			"offload_impl_enable was called unexpectedly");
	zassert_equal(atomic_get(&up_count), 0,
			"offload_impl_enable was called unexpectedly");
	zassert_false(net_if_is_admin_up(test_iface),
			"Iface under test should be down after net_if_down");
}

ZTEST(net_offloaded_netdev, test_up_down_sock_off_impl_double)
{
	struct net_if *test_iface = NET_IF_GET(sock_offload_test_impl, 0);

	/* Verify iface under test is down before test */
	zassert_false(net_if_is_admin_up(test_iface),
			"Iface under test must be admin-down before test");

	/* Bring iface up twice */
	(void)net_if_up(test_iface);
	(void)net_if_up(test_iface);

	/* Verify that a single iface went up once (according to the enable callback)*/
	zassert_equal(atomic_get(&call_count), 1,
			"Bad transition-count, offload_impl_enable not called correctly");
	zassert_equal(atomic_get(&up_count), 1,
			"Bad up-count, offload_impl_enable not called correctly");
	zassert_true(net_if_is_admin_up(test_iface),
			"Iface under test should be up after net_if_up");

	/* Verify that a single iface went down once (according to the enable callback)*/
	(void)net_if_down(test_iface);
	(void)net_if_down(test_iface);

	/* Verify appropriate calls were made */
	zassert_equal(atomic_get(&call_count), 2,
			"Bad transition-count, offload_impl_enable not called correctly");
	zassert_equal(atomic_get(&up_count), 0,
			"Bad up-count, offload_impl_enable not called correctly");
	zassert_false(net_if_is_admin_up(test_iface),
			"Iface under test should be down after net_if_down");
}

ZTEST(net_offloaded_netdev, test_up_down_sock_off_impl_fail_up)
{
	struct net_if *test_iface = NET_IF_GET(sock_offload_test_impl, 0);

	/* Verify iface under test is down before test */
	zassert_false(net_if_is_admin_up(test_iface),
			"Iface under test must be admin-down before test");

	/* Instruct the enable callback to fail */
	atomic_set(&retval, -E2BIG);

	/* Expect net_if_up to fail accordingly */
	zassert_equal(net_if_up(test_iface), -E2BIG,
		"net_if_up should forward error returned from offload_impl_enabled");

	/* Verify that the iface failed to go up */
	zassert_false(net_if_is_admin_up(test_iface),
			"Iface under test should have failed to go up");
}

ZTEST(net_offloaded_netdev, test_up_down_sock_off_impl_fail_down)
{
	struct net_if *test_iface = NET_IF_GET(sock_offload_test_impl, 0);

	/* Bring iface up before test */
	(void) net_if_up(test_iface);

	/* Instruct the enable callback to fail */
	atomic_set(&retval, -EADDRINUSE);


	/* Expect net_if_down to fail accordingly */
	zassert_equal(net_if_down(test_iface), -EADDRINUSE,
		"net_if_down should forward error returned from offload_impl_enabled");

	/* Verify that the iface failed to go down */
	zassert_true(net_if_is_admin_up(test_iface),
			"Iface under test should have failed to go up");
}

static void test_addr_add_common(struct net_if *test_iface, const char *off_type)
{
	struct net_if *lookup_iface;
	struct net_if_addr *ipv4_addr;
	struct net_if_addr *ipv6_addr;

	/* Bring iface up before test */
	(void)net_if_up(test_iface);

	ipv4_addr = net_if_ipv4_addr_add(test_iface, &test_addr_ipv4,
					 NET_ADDR_MANUAL, 0);
	zassert_not_null(ipv4_addr,
			"Failed to add IPv4 address to a %s offloaded interface",
			off_type);
	ipv6_addr = net_if_ipv6_addr_add(test_iface, &test_addr_ipv6,
					 NET_ADDR_MANUAL, 0);
	zassert_not_null(ipv6_addr,
			 "Failed to add IPv6 address to a socket %s interface",
			 off_type);

	lookup_iface = NULL;
	zassert_equal_ptr(net_if_ipv4_addr_lookup(&test_addr_ipv4, &lookup_iface),
			  ipv4_addr,
			  "Failed to find IPv4 address on a %s offloaded interface");
	zassert_equal_ptr(lookup_iface, test_iface, "Wrong interface");

	lookup_iface = NULL;
	zassert_equal_ptr(net_if_ipv6_addr_lookup(&test_addr_ipv6, &lookup_iface),
			  ipv6_addr,
			  "Failed to find IPv6 address on a %s offloaded interface");
	zassert_equal_ptr(lookup_iface, test_iface, "Wrong interface");

	zassert_true(net_if_ipv4_addr_rm(test_iface, &test_addr_ipv4),
		     "Failed to remove IPv4 address from a %s offloaded interface",
		     off_type);
	zassert_true(net_if_ipv6_addr_rm(test_iface, &test_addr_ipv6),
		     "Failed to remove IPv4 address from a %s offloaded interface",
		     off_type);
}

ZTEST(net_offloaded_netdev, test_addr_add_sock_off_impl)
{
	struct net_if *test_iface = NET_IF_GET(sock_offload_test_impl, 0);

	test_addr_add_common(test_iface, "offloaded");
}

ZTEST(net_offloaded_netdev, test_addr_add_net_off_impl)
{
	struct net_if *test_iface = NET_IF_GET(net_offload_test_impl, 0);

	test_addr_add_common(test_iface, "net");
}

ZTEST_SUITE(net_offloaded_netdev, NULL, NULL, net_offloaded_netdev_before, NULL, NULL);
