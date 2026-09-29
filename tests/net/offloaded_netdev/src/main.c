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
#include <zephyr/net/socket_offload.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/fdtable.h>

static struct net_in_addr test_addr_ipv4 = { { { 192, 0, 2, 1 } } };
static struct net_in6_addr test_addr_ipv6 = { { {
	0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1
} } };

/* Dummy socket creator for socket-offloaded ifaces */
int offload_socket(int family, int type, int proto)
{
	return -1;
}

static net_context_recv_cb_t offload_recv_cb;
static void *offload_recv_user_data;

static int offload_get(net_sa_family_t family, enum net_sock_type type,
		       enum net_ip_protocol ip_proto, struct net_context **context)
{
	ARG_UNUSED(family);
	ARG_UNUSED(type);
	ARG_UNUSED(ip_proto);
	ARG_UNUSED(context);

	return 0;
}

static int offload_bind(struct net_context *context, const struct net_sockaddr *addr,
			net_socklen_t addrlen)
{
	ARG_UNUSED(context);
	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);

	return 0;
}

static int offload_recv(struct net_context *context, net_context_recv_cb_t cb,
			int32_t timeout, void *user_data)
{
	ARG_UNUSED(context);
	ARG_UNUSED(timeout);

	offload_recv_cb = cb;
	offload_recv_user_data = user_data;

	return 0;
}

static int offload_put(struct net_context *context)
{
	ARG_UNUSED(context);

	return 0;
}

/* Dummy offload API for net-offloaded ifaces */
struct net_offload net_offload_api = {
	.get = offload_get,
	.bind = offload_bind,
	.recv = offload_recv,
	.put = offload_put,
};

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
	net_if_offload_set(iface, &net_offload_api);
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
static DEVICE_API(offloaded_if, sock_offloaded_impl_api) = {
	.iface_api.init = sock_offload_l2_iface_init,
	.enable = offload_impl_enable
};

static DEVICE_API(offloaded_if, sock_offloaded_no_impl_api) = {
	.iface_api.init = sock_offload_l2_iface_init
};

/* Net-dev APIs for L2s that are net-offloaded, with and without .enable */
static DEVICE_API(offloaded_if, net_offloaded_impl_api) = {
	.iface_api.init = net_offload_l2_iface_init,
	.enable = offload_impl_enable
};

static DEVICE_API(offloaded_if, net_offloaded_no_impl_api) = {
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
			  "Failed to find IPv4 address on a %s offloaded interface", off_type);
	zassert_equal_ptr(lookup_iface, test_iface, "Wrong interface");

	lookup_iface = NULL;
	zassert_equal_ptr(net_if_ipv6_addr_lookup(&test_addr_ipv6, &lookup_iface),
			  ipv6_addr,
			  "Failed to find IPv6 address on a %s offloaded interface", off_type);
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

#define OFFLOAD_PAYLOAD "abcd"
#define OFFLOAD_PAYLOAD_LEN (sizeof(OFFLOAD_PAYLOAD) - 1U)

static int offload_sock = -1;

static K_THREAD_STACK_DEFINE(deliver_stack, 2048);
static struct k_thread deliver_thread;
static bool deliver_created;

static int offloaded_udp_socket(struct net_if *iface, struct net_context **ctx)
{
	struct net_if *default_iface = net_if_get_default();
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(4242),
	};
	int ret;

	net_if_set_default(iface);
	offload_sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	net_if_set_default(default_iface);
	zassert_true(offload_sock >= 0, "socket() failed (%d)", errno);

	offload_recv_cb = NULL;
	ret = zsock_bind(offload_sock, (struct net_sockaddr *)&addr, sizeof(addr));
	zassert_ok(ret, "bind() failed (%d)", errno);
	zassert_not_null(offload_recv_cb, "offloaded recv() was not called");

	*ctx = zvfs_get_fd_obj(offload_sock, NULL, 0);
	zassert_not_null(*ctx, "no context for socket %d", offload_sock);

	return offload_sock;
}

static struct net_pkt *offloaded_rx_pkt(struct net_if *iface)
{
	struct net_pkt *pkt;

	pkt = net_pkt_rx_alloc_with_buffer(iface, OFFLOAD_PAYLOAD_LEN, NET_AF_INET,
					   NET_IPPROTO_UDP, K_NO_WAIT);
	zassert_not_null(pkt, "cannot allocate a packet");
	zassert_ok(net_pkt_write(pkt, OFFLOAD_PAYLOAD, OFFLOAD_PAYLOAD_LEN),
		   "cannot write the packet");
	net_pkt_cursor_init(pkt);

	return pkt;
}

static void offloaded_close(int sock)
{
	zassert_ok(zsock_close(sock), "close() failed (%d)", errno);
	offload_sock = -1;
}

ZTEST(net_offloaded_netdev, test_recv_net_off_impl)
{
	struct net_if *test_iface = NET_IF_GET(net_offload_test_impl, 0);
	struct net_context *ctx;
	char buf[OFFLOAD_PAYLOAD_LEN];
	int sock;
	int ret;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_NATIVE);

	sock = offloaded_udp_socket(test_iface, &ctx);

	offload_recv_cb(ctx, offloaded_rx_pkt(test_iface), NULL, NULL, 0, offload_recv_user_data);

	ret = zsock_recv(sock, buf, sizeof(buf), ZSOCK_MSG_DONTWAIT);
	zassert_equal(ret, sizeof(buf), "recv() returned %d (%d)", ret, errno);
	zassert_mem_equal(buf, OFFLOAD_PAYLOAD, sizeof(buf), "wrong data");

	offloaded_close(sock);
}

static void deliver(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p3);

	offload_recv_cb(p1, p2, NULL, NULL, 0, offload_recv_user_data);
}

ZTEST(net_offloaded_netdev, test_shutdown_during_recv_net_off_impl)
{
	struct net_if *test_iface = NET_IF_GET(net_offload_test_impl, 0);
	const struct fd_op_vtable *vtable;
	struct net_context *ctx;
	struct k_mem_slab *rx;
	struct k_mutex *lock;
	uint32_t rx_free;
	char state[32];
	char buf[OFFLOAD_PAYLOAD_LEN];
	int sock;
	int ret;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_NATIVE);

	if (arch_num_cpus() > 1) {
		ztest_test_skip();
	}

	sock = offloaded_udp_socket(test_iface, &ctx);
	zassert_not_null(zvfs_get_fd_obj_and_vtable(sock, &vtable, &lock), "lookup failed");

	net_pkt_get_info(&rx, NULL, NULL, NULL);
	rx_free = k_mem_slab_num_free_get(rx);

	/* The driver delivers while shutdown() holds the fd lock */
	zassert_ok(k_mutex_lock(lock, K_FOREVER), "lock failed");
	deliver_created = true;
	k_thread_create(&deliver_thread, deliver_stack, K_THREAD_STACK_SIZEOF(deliver_stack),
			deliver, ctx, offloaded_rx_pkt(test_iface), NULL,
			k_thread_priority_get(k_current_get()) - 1, 0, K_NO_WAIT);
	k_yield();
	zexpect_not_null(strstr(k_thread_state_str(&deliver_thread, state, sizeof(state)),
				"pending"),
			 "delivery is not blocked on the lock: %s", state);
	zexpect_ok(zsock_shutdown(sock, ZSOCK_SHUT_RD), "shutdown() failed (%d)", errno);
	k_mutex_unlock(lock);

	zassert_ok(k_thread_join(&deliver_thread, K_SECONDS(1)), "delivery did not return");
	deliver_created = false;

	ret = zsock_recv(sock, buf, sizeof(buf), ZSOCK_MSG_DONTWAIT);
	zassert_true(ret <= 0, "recv() after shutdown() returned %d", ret);
	zassert_equal(k_mem_slab_num_free_get(rx), rx_free,
		      "RX packet leaked (%u free, expected %u)", k_mem_slab_num_free_get(rx),
		      rx_free);

	offloaded_close(sock);
}

static K_THREAD_STACK_DEFINE(recv_stack, 2048);
static struct k_thread recv_thread;
static bool recv_created;
static int recv_ret;
static int recv_errno;
static bool addr_added;

static void udp_recv(void *p1, void *p2, void *p3)
{
	char buf[OFFLOAD_PAYLOAD_LEN];

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	recv_ret = zsock_recv(POINTER_TO_INT(p1), buf, sizeof(buf), 0);
	recv_errno = errno;
}

static int native_udp_socket(struct net_context **ctx)
{
	struct net_if *default_iface = net_if_get_default();

	net_if_set_default(NET_IF_GET(sock_offload_test_impl, 0));
	offload_sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	net_if_set_default(default_iface);
	zassert_true(offload_sock >= 0, "socket() failed (%d)", errno);

	*ctx = zvfs_get_fd_obj(offload_sock, NULL, 0);
	zassert_not_null(*ctx, "no context for socket %d", offload_sock);
	zassume_false(net_if_is_ip_offloaded(net_context_get_iface(*ctx)),
		      "the socket starts on an IP-offloaded interface");

	return offload_sock;
}

static void native_bind(int sock, struct net_context *ctx)
{
	struct net_if *default_iface = net_if_get_default();
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_addr.s_addr = NET_INADDR_ANY,
	};
	int ret;

	net_if_set_default(NET_IF_GET(sock_offload_test_impl, 0));
	ret = zsock_bind(sock, (struct net_sockaddr *)&addr, sizeof(addr));
	net_if_set_default(default_iface);
	zassert_ok(ret, "bind() failed (%d)", errno);
	zassume_false(net_if_is_ip_offloaded(net_context_get_iface(ctx)),
		      "bind() put the socket on an IP-offloaded interface");
}

static void udp_recv_start(int sock)
{
	int prio = k_thread_priority_get(k_current_get()) + 1;
	char state[32];

	zassume_true(prio <= K_LOWEST_APPLICATION_THREAD_PRIO, "no lower priority for the reader");

	k_thread_create(&recv_thread, recv_stack, K_THREAD_STACK_SIZEOF(recv_stack), udp_recv,
			INT_TO_POINTER(sock), NULL, NULL, prio, 0, K_NO_WAIT);
	recv_created = true;
	k_msleep(10);
	zassert_not_null(strstr(k_thread_state_str(&recv_thread, state, sizeof(state)), "pending"),
			 "recv() is not blocked: %s", state);
}

static void offloaded_bind(int sock)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(4242),
		.sin_addr = test_addr_ipv4,
	};

	offload_recv_cb = NULL;
	zassert_ok(zsock_bind(sock, (struct net_sockaddr *)&addr, sizeof(addr)),
		   "bind() failed (%d)", errno);
	zassert_not_null(offload_recv_cb, "offloaded recv() was not called");
}

static void udp_test_skip(void)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_NATIVE);

	if (arch_num_cpus() > 1) {
		ztest_test_skip();
	}
}

ZTEST(net_offloaded_netdev, test_close_during_recv_native)
{
	struct net_context *ctx;
	int sock;

	udp_test_skip();

	sock = native_udp_socket(&ctx);
	native_bind(sock, ctx);
	udp_recv_start(sock);

	offloaded_close(sock);
	sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(sock >= 0, "socket() failed (%d)", errno);
	offload_sock = sock;
	zexpect_true(zvfs_get_fd_obj(sock, NULL, 0) != ctx,
		     "socket() got the context back while recv() waits on it");

	zassert_ok(k_thread_join(&recv_thread, K_MSEC(100)), "recv() did not return");
	recv_created = false;
	zexpect_equal(recv_ret, -1, "recv() returned %d", recv_ret);
	zexpect_equal(recv_errno, EINTR, "recv() failed with %d", recv_errno);

	offloaded_close(sock);
}

ZTEST(net_offloaded_netdev, test_bind_during_recv_net_off_impl)
{
	struct net_if *test_iface = NET_IF_GET(net_offload_test_impl, 0);
	struct net_context *ctx;
	char buf[OFFLOAD_PAYLOAD_LEN];
	int sock;
	int ret;

	udp_test_skip();

	(void)net_if_up(test_iface);
	zassert_not_null(net_if_ipv4_addr_add(test_iface, &test_addr_ipv4, NET_ADDR_MANUAL, 0),
			 "cannot add an address to the offloaded interface");
	addr_added = true;

	sock = native_udp_socket(&ctx);
	udp_recv_start(sock);

	offloaded_bind(sock);
	offloaded_close(sock);
	sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(sock >= 0, "socket() failed (%d)", errno);
	offload_sock = sock;
	zassume_equal_ptr(zvfs_get_fd_obj(sock, NULL, 0), ctx,
			  "socket() did not reuse the context");
	offloaded_bind(sock);

	zassert_ok(k_thread_join(&recv_thread, K_MSEC(100)), "recv() did not return");
	recv_created = false;
	zexpect_equal(recv_ret, -1, "recv() on the closed socket returned %d", recv_ret);
	zexpect_equal(recv_errno, EINTR, "recv() on the closed socket failed with %d",
		      recv_errno);

	offload_recv_cb(ctx, offloaded_rx_pkt(test_iface), NULL, NULL, 0, offload_recv_user_data);
	ret = zsock_recv(sock, buf, sizeof(buf), ZSOCK_MSG_DONTWAIT);
	zexpect_equal(ret, sizeof(buf), "recv() on the new socket returned %d (%d)", ret, errno);
	zexpect_true(net_context_is_used(ctx), "recv() released the new socket's context");

	offloaded_close(sock);
}

static bool offload_getaddrinfo_called;

static int test_offload_getaddrinfo(const char *node,
				    const char *service,
				    const struct zsock_addrinfo *hints,
				    struct zsock_addrinfo **res)
{
	ARG_UNUSED(node);
	ARG_UNUSED(service);
	ARG_UNUSED(hints);
	ARG_UNUSED(res);

	offload_getaddrinfo_called = true;

	return 0;
}

static void test_offload_freeaddrinfo(struct zsock_addrinfo *res)
{
	ARG_UNUSED(res);
}

static const struct socket_dns_offload test_dns_offload_ops = {
	.getaddrinfo = test_offload_getaddrinfo,
	.freeaddrinfo = test_offload_freeaddrinfo,
};

ZTEST(net_offloaded_netdev, test_dns_offload)
{
	struct zsock_addrinfo *ai;

	/* Register offloaded DNS */
	offload_getaddrinfo_called = false;
	socket_offload_dns_register(&test_dns_offload_ops);
	zassert_true(socket_offload_dns_is_enabled(),
		     "DNS offloading should be enabled");
	zassert_ok(zsock_getaddrinfo("127.0.0.1", NULL, NULL, &ai),
		   "getaddrinfo() failed");
	zassert_true(offload_getaddrinfo_called,
		     "Offloaded implementation should be called");
	zsock_freeaddrinfo(ai);

	/* Disable offloaded DNS */
	offload_getaddrinfo_called = false;
	socket_offload_dns_enable(false);
	zassert_false(socket_offload_dns_is_enabled(),
		     "DNS offloading should be disabled");
	zassert_ok(zsock_getaddrinfo("127.0.0.1", NULL, NULL, &ai),
		   "getaddrinfo() failed");
	zassert_false(offload_getaddrinfo_called,
		     "Offloaded implementation shouldn't be called");
	zsock_freeaddrinfo(ai);

	/* Reenable offloaded DNS */
	offload_getaddrinfo_called = false;
	socket_offload_dns_enable(true);
	zassert_true(socket_offload_dns_is_enabled(),
		     "DNS offloading should be enabled");
	zassert_ok(zsock_getaddrinfo("127.0.0.1", NULL, NULL, &ai),
		   "getaddrinfo() failed");
	zassert_true(offload_getaddrinfo_called,
		     "Offloaded implementation should be called");
	zsock_freeaddrinfo(ai);

	/* Deregister offloaded DNS */
	offload_getaddrinfo_called = false;
	socket_offload_dns_deregister(&test_dns_offload_ops);
	zassert_false(socket_offload_dns_is_enabled(),
		      "DNS offloading should be disabled");
	zassert_ok(zsock_getaddrinfo("127.0.0.1", NULL, NULL, &ai),
		   "getaddrinfo() failed");
	zassert_false(offload_getaddrinfo_called,
		     "Offloaded implementation shouldn't be called");
	zsock_freeaddrinfo(ai);
}

static void net_offloaded_netdev_after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (offload_sock >= 0) {
		(void)zsock_close(offload_sock);
		offload_sock = -1;
	}

	if (deliver_created) {
		if (k_thread_join(&deliver_thread, K_SECONDS(1)) != 0) {
			k_thread_abort(&deliver_thread);
		}
		deliver_created = false;
	}

	if (recv_created) {
		if (k_thread_join(&recv_thread, K_SECONDS(1)) != 0) {
			k_thread_abort(&recv_thread);
		}
		recv_created = false;
	}

	if (addr_added) {
		(void)net_if_ipv4_addr_rm(NET_IF_GET(net_offload_test_impl, 0), &test_addr_ipv4);
		addr_added = false;
	}
}

ZTEST_SUITE(net_offloaded_netdev, NULL, NULL, net_offloaded_netdev_before,
	    net_offloaded_netdev_after, NULL);
