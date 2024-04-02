/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_IPV6_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/linker/sections.h>

#include <zephyr/ztest.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>

#include <zephyr/random/random.h>

#include "icmpv6.h"
#include "ipv6.h"

#define THREAD_SLEEP 50 /* ms */

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_NET_IPV6_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static struct in6_addr my_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct in6_addr peer_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					 0, 0, 0, 0, 0, 0, 0, 0x2 } } };
static struct in6_addr mcast_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x1 } } };

static struct net_if *net_iface;
static bool is_group_joined;
static bool is_group_left;
static bool is_join_msg_ok;
static bool is_leave_msg_ok;
static bool is_query_received;
static bool is_report_sent;
static bool ignore_already;
K_SEM_DEFINE(wait_data, 0, UINT_MAX);

#define WAIT_TIME 500
#define WAIT_TIME_LONG MSEC_PER_SEC
#define MY_PORT 1969
#define PEER_PORT 13856

struct net_test_mld {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_test_dev_init(const struct device *dev)
{
	return 0;
}

static uint8_t *net_test_get_mac(const struct device *dev)
{
	struct net_test_mld *context = dev->data;

	if (context->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		context->mac_addr[0] = 0x00;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x5E;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x53;
		context->mac_addr[5] = sys_rand32_get();
	}

	return context->mac_addr;
}

static void net_test_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_test_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static struct net_icmp_hdr *get_icmp_hdr(struct net_pkt *pkt)
{
	net_pkt_cursor_init(pkt);

	net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
		     net_pkt_ipv6_ext_len(pkt));

	return (struct net_icmp_hdr *)net_pkt_cursor_get_pos(pkt);
}

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	struct net_icmp_hdr *icmp;

	if (!pkt->buffer) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	icmp = get_icmp_hdr(pkt);

	if (icmp->type == NET_ICMPV6_MLDv2) {
		/* FIXME, add more checks here */

		NET_DBG("Received something....");
		is_join_msg_ok = true;
		is_leave_msg_ok = true;
		is_report_sent = true;

		k_sem_give(&wait_data);
	}

	return 0;
}

struct net_test_mld net_test_data;

static struct dummy_api net_test_if_api = {
	.iface_api.init = net_test_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(net_test_mld, "net_test_mld",
		net_test_dev_init, NULL, &net_test_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_test_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE,
		127);

static void group_joined(struct net_mgmt_event_callback *cb,
			 uint32_t nm_event, struct net_if *iface)
{
	if (nm_event != NET_EVENT_IPV6_MCAST_JOIN) {
		/* Spurious callback. */
		return;
	}

	is_group_joined = true;

	k_sem_give(&wait_data);
}

static void group_left(struct net_mgmt_event_callback *cb,
			 uint32_t nm_event, struct net_if *iface)
{
	if (nm_event != NET_EVENT_IPV6_MCAST_LEAVE) {
		/* Spurious callback. */
		return;
	}

	is_group_left = true;

	k_sem_give(&wait_data);
}

static struct mgmt_events {
	uint32_t event;
	net_mgmt_event_handler_t handler;
	struct net_mgmt_event_callback cb;
} mgmt_events[] = {
	{ .event = NET_EVENT_IPV6_MCAST_JOIN, .handler = group_joined },
	{ .event = NET_EVENT_IPV6_MCAST_LEAVE, .handler = group_left },
	{ 0 }
};

static void setup_mgmt_events(void)
{
	int i;

	for (i = 0; mgmt_events[i].event; i++) {
		net_mgmt_init_event_callback(&mgmt_events[i].cb,
					     mgmt_events[i].handler,
					     mgmt_events[i].event);

		net_mgmt_add_event_callback(&mgmt_events[i].cb);
	}
}

static void *test_mld_setup(void)
{
	struct net_if_addr *ifaddr;

	setup_mgmt_events();

	net_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	zassert_not_null(net_iface, "Interface is NULL");

	ifaddr = net_if_ipv6_addr_add(net_iface, &my_addr,
				      NET_ADDR_MANUAL, 0);

	zassert_not_null(ifaddr, "Cannot add IPv6 address");

	return NULL;
}

static void test_join_group(void)
{
	int ret;

	/* Using adhoc multicast group outside standard range */
	net_ipv6_addr_create(&mcast_addr, 0xff10, 0, 0, 0, 0, 0, 0, 0x0001);

	ret = net_ipv6_mld_join(net_iface, &mcast_addr);

	if (ignore_already) {
		zassert_true(ret == 0 || ret == -EALREADY,
			     "Cannot join IPv6 multicast group");
	} else {
		zassert_equal(ret, 0, "Cannot join IPv6 multicast group");
	}

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);
}

static void test_leave_group(void)
{
	int ret;

	net_ipv6_addr_create(&mcast_addr, 0xff10, 0, 0, 0, 0, 0, 0, 0x0001);

	ret = net_ipv6_mld_leave(net_iface, &mcast_addr);

	zassert_equal(ret, 0, "Cannot leave IPv6 multicast group");

	k_msleep(THREAD_SLEEP);
}

static void test_catch_join_group(void)
{
	is_group_joined = false;

	ignore_already = false;

	test_join_group();

	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting join event");
	}

	if (!is_group_joined) {
		zassert_true(0, "Did not catch join event");
	}

	is_group_joined = false;
}

static void test_catch_leave_group(void)
{
	is_group_joined = false;

	test_leave_group();

	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting leave event");
	}

	if (!is_group_left) {
		zassert_true(0, "Did not catch leave event");
	}

	is_group_left = false;
}

static void test_verify_join_group(void)
{
	is_join_msg_ok = false;

	ignore_already = false;

	test_join_group();

	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting join event");
	}

	if (!is_join_msg_ok) {
		zassert_true(0, "Join msg invalid");
	}

	is_join_msg_ok = false;
}

static void test_verify_leave_group(void)
{
	is_leave_msg_ok = false;

	test_leave_group();

	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting leave event");
	}

	if (!is_leave_msg_ok) {
		zassert_true(0, "Leave msg invalid");
	}

	is_leave_msg_ok = false;
}

static void send_query(struct net_if *iface)
{
	struct net_pkt *pkt;
	struct in6_addr dst;
	int ret;

	/* Sent to all MLDv2-capable routers */
	net_ipv6_addr_create(&dst, 0xff02, 0, 0, 0, 0, 0, 0, 0x0016);

	/* router alert opt + icmpv6 reserved space + mldv2 mcast record */
	pkt = net_pkt_alloc_with_buffer(iface, 144, AF_INET6,
					IPPROTO_ICMPV6, K_FOREVER);
	zassert_not_null(pkt, "Cannot allocate pkt");

	net_pkt_set_ipv6_hop_limit(pkt, 1); /* RFC 3810 ch 7.4 */
	ret = net_ipv6_create(pkt, &peer_addr, &dst);
	zassert_false(ret, "Cannot create ipv6 pkt");

	/* Add hop-by-hop option and router alert option, RFC 3810 ch 5. */
	ret = net_pkt_write_u8(pkt, IPPROTO_ICMPV6);
	zassert_false(ret, "Failed to write");
	ret = net_pkt_write_u8(pkt, 0); /* length (0 means 8 bytes) */
	zassert_false(ret, "Failed to write");

#define ROUTER_ALERT_LEN 8

	/* IPv6 router alert option is described in RFC 2711. */
	ret = net_pkt_write_be16(pkt, 0x0502); /* RFC 2711 ch 2.1 */
	zassert_false(ret, "Failed to write");
	ret = net_pkt_write_be16(pkt, 0); /* pkt contains MLD msg */
	zassert_false(ret, "Failed to write");

	ret = net_pkt_write_u8(pkt, 1); /* padn */
	zassert_false(ret, "Failed to write");
	ret = net_pkt_write_u8(pkt, 0); /* padn len */
	zassert_false(ret, "Failed to write");

	net_pkt_set_ipv6_ext_len(pkt, ROUTER_ALERT_LEN);

	/* ICMPv6 header */
	ret = net_icmpv6_create(pkt, NET_ICMPV6_MLD_QUERY, 0);
	zassert_false(ret, "Cannot create icmpv6 pkt");

	ret = net_pkt_write_be16(pkt, 3); /* maximum response code */
	zassert_false(ret, "Failed to write");
	ret = net_pkt_write_be16(pkt, 0); /* reserved field */
	zassert_false(ret, "Failed to write");

	net_pkt_set_ipv6_next_hdr(pkt, NET_IPV6_NEXTHDR_HBHO);

	ret = net_pkt_write_be16(pkt, 0); /* Resv, S, QRV and QQIC */
	zassert_false(ret, "Failed to write");
	ret = net_pkt_write_be16(pkt, 0); /* number of addresses */
	zassert_false(ret, "Failed to write");

	ret = net_pkt_write(pkt, net_ipv6_unspecified_address(),
			    sizeof(struct in6_addr));
	zassert_false(ret, "Failed to write");

	net_pkt_cursor_init(pkt);
	ret = net_ipv6_finalize(pkt, IPPROTO_ICMPV6);
	zassert_false(ret, "Failed to finalize ipv6 packet");

	net_pkt_cursor_init(pkt);

	ret = net_recv_data(iface, pkt);
	zassert_false(ret, "Failed to receive data");
}

/* interface needs to join the MLDv2-capable routers multicast group before it
 * can receive MLD queries
 */
static void join_mldv2_capable_routers_group(void)
{
	struct net_if *iface;
	int ret;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	net_ipv6_addr_create(&mcast_addr, 0xff02, 0, 0, 0, 0, 0, 0, 0x0016);
	ret = net_ipv6_mld_join(iface, &mcast_addr);

	zassert_true(ret == 0 || ret == -EALREADY,
		     "Cannot join MLDv2-capable routers multicast group");

	k_msleep(THREAD_SLEEP);

}

static void leave_mldv2_capable_routers_group(void)
{
	struct net_if *iface;
	int ret;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	net_ipv6_addr_create(&mcast_addr, 0xff02, 0, 0, 0, 0, 0, 0, 0x0016);
	ret = net_ipv6_mld_leave(iface, &mcast_addr);

	zassert_equal(ret, 0,
		      "Cannot leave MLDv2-capable routers multicast group");

	k_msleep(THREAD_SLEEP);
}

/* We are not really interested to parse the query at this point */
static int handle_mld_query(struct net_icmp_ctx *ctx,
			    struct net_pkt *pkt,
			    struct net_icmp_ip_hdr *hdr,
			    struct net_icmp_hdr *icmp_hdr,
			    void *user_data)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(pkt);
	ARG_UNUSED(hdr);
	ARG_UNUSED(icmp_hdr);
	ARG_UNUSED(user_data);

	is_query_received = true;

	NET_DBG("Handling MLD query");

	return NET_DROP;
}

static void test_catch_query(void)
{
	struct net_icmp_ctx ctx;
	int ret;

	join_mldv2_capable_routers_group();

	is_query_received = false;

	ret = net_icmp_init_ctx(&ctx, NET_ICMPV6_MLD_QUERY,
				0, handle_mld_query);
	zassert_equal(ret, 0, "Cannot register %s handler (%d)",
		      STRINGIFY(NET_ICMPV6_MLD_QUERY), ret);

	send_query(net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)));

	k_msleep(THREAD_SLEEP);

	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting query event");
	}

	if (!is_query_received) {
		zassert_true(0, "Query msg invalid");
	}

	is_query_received = false;

	leave_mldv2_capable_routers_group();

	net_icmp_cleanup_ctx(&ctx);
}

static void test_verify_send_report(void)
{
	join_mldv2_capable_routers_group();

	is_query_received = false;
	is_report_sent = false;

	ignore_already = true;

	k_sem_reset(&wait_data);

	test_join_group();

	k_yield();

	/* Did we send a report? */
	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting for report");
	}

	k_sem_reset(&wait_data);

	is_report_sent = false;
	send_query(net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)));

	k_yield();

	/* Did we send a report? */
	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting for report");
	}

	zassert_true(is_report_sent, "Report not sent");

	leave_mldv2_capable_routers_group();
}

/* This value should be longer that the one in net_if.c when DAD timeouts */
#define DAD_TIMEOUT (MSEC_PER_SEC / 5U)

ZTEST(net_mld_test_suite, test_allnodes)
{
	struct net_if *iface = NULL;
	struct net_if_mcast_addr *ifmaddr;
	struct in6_addr addr;

	net_ipv6_addr_create_ll_allnodes_mcast(&addr);

	/* Let the DAD succeed so that the multicast address will be there */
	k_sleep(K_MSEC(DAD_TIMEOUT));

	ifmaddr = net_if_ipv6_maddr_lookup(&addr, &iface);

	zassert_not_null(ifmaddr, "Interface does not contain "
			"allnodes multicast address");
}

ZTEST(net_mld_test_suite, test_solicit_node)
{
	struct net_if *iface = NULL;
	struct net_if_mcast_addr *ifmaddr;
	struct in6_addr addr;

	net_ipv6_addr_create_solicited_node(&my_addr, &addr);

	ifmaddr = net_if_ipv6_maddr_lookup(&addr, &iface);

	zassert_not_null(ifmaddr, "Interface does not contain "
			"solicit node multicast address");
}

ZTEST(net_mld_test_suite, test_join_leave)
{
	test_join_group();
	test_leave_group();
}

ZTEST(net_mld_test_suite, test_catch_join_leave)
{
	test_catch_join_group();
	test_catch_leave_group();
}

ZTEST(net_mld_test_suite, test_verify_join_leave)
{
	test_verify_join_group();
	test_verify_leave_group();
	test_catch_query();
	test_verify_send_report();
}

ZTEST(net_mld_test_suite, test_no_mld_flag)
{
	int ret;

	is_join_msg_ok = false;
	is_leave_msg_ok = false;

	net_if_flag_set(net_iface, NET_IF_IPV6_NO_MLD);

	/* Using adhoc multicast group outside standard range */
	net_ipv6_addr_create(&mcast_addr, 0xff10, 0, 0, 0, 0, 0, 0, 0x0001);

	ret = net_ipv6_mld_join(net_iface, &mcast_addr);
	zassert_equal(ret, 0, "Cannot add multicast address");

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);

	zassert_false(is_join_msg_ok, "Received join message when not expected");

	ret = net_ipv6_mld_leave(net_iface, &mcast_addr);
	zassert_equal(ret, 0, "Cannot remove multicast address");

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);

	zassert_false(is_leave_msg_ok, "Received leave message when not expected");

	net_if_flag_clear(net_iface, NET_IF_IPV6_NO_MLD);
}

static void socket_group_with_index(const struct in6_addr *local_addr, bool do_join)
{
	struct ipv6_mreq mreq = { 0 };
	int option;
	int ret, fd;

	if (do_join) {
		option = IPV6_ADD_MEMBERSHIP;
	} else {
		option = IPV6_DROP_MEMBERSHIP;
	}

	fd = zsock_socket(AF_INET6, SOCK_DGRAM, 0);
	zassert_true(fd >= 0, "Cannot get socket (%d)", -errno);

	ret = zsock_setsockopt(fd, IPPROTO_IPV6, option,
			       NULL, sizeof(mreq));
	zassert_true(ret == -1 && errno == EINVAL,
		     "Incorrect return value (%d)", -errno);

	ret = zsock_setsockopt(fd, IPPROTO_IPV6, option,
			       (void *)&mreq, 1);
	zassert_true(ret == -1 && errno == EINVAL,
		     "Incorrect return value (%d)", -errno);

	/* First try with empty mreq */
	ret = zsock_setsockopt(fd, IPPROTO_IPV6, option,
			       (void *)&mreq, sizeof(mreq));
	zassert_true(ret == -1 && errno == EINVAL,
		     "Incorrect return value (%d)", -errno);

	mreq.ipv6mr_ifindex = net_if_ipv6_addr_lookup_by_index(local_addr);
	memcpy(&mreq.ipv6mr_multiaddr, &mcast_addr,
	       sizeof(mreq.ipv6mr_multiaddr));

	ret = zsock_setsockopt(fd, IPPROTO_IPV6, option,
			       (void *)&mreq, sizeof(mreq));

	if (do_join) {
		if (ignore_already) {
			zassert_true(ret == 0 || ret == -EALREADY,
				     "Cannot join IPv6 multicast group (%d)",
				     -errno);
		} else {
			zassert_equal(ret, 0,
				      "Cannot join IPv6 multicast group (%d)",
				      -errno);
		}
	} else {
		zassert_equal(ret, 0, "Cannot leave IPv6 multicast group (%d)",
			      -errno);

		if (IS_ENABLED(CONFIG_NET_TC_THREAD_PREEMPTIVE)) {
			/* Let the network stack to proceed */
			k_msleep(THREAD_SLEEP);
		} else {
			k_yield();
		}
	}

	zsock_close(fd);

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);
}

static void socket_join_group_with_index(const struct in6_addr *addr)
{
	socket_group_with_index(addr, true);
}

static void socket_leave_group_with_index(const struct in6_addr *addr)
{
	socket_group_with_index(addr, false);
}

ZTEST_USER(net_mld_test_suite, test_socket_catch_join_with_index)
{
	socket_join_group_with_index(net_ipv6_unspecified_address());
	socket_leave_group_with_index(net_ipv6_unspecified_address());
	socket_join_group_with_index(&my_addr);
	socket_leave_group_with_index(&my_addr);
}

ZTEST_SUITE(net_mld_test_suite, NULL, test_mld_setup, NULL, NULL, NULL);
