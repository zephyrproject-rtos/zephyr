/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <linker/sections.h>

#include <ztest.h>

#include <net/net_if.h>
#include <net/net_pkt.h>
#include <net/net_ip.h>
#include <net/net_core.h>
#include <net/ethernet.h>
#include <net/net_mgmt.h>
#include <net/net_event.h>

#include "icmpv6.h"
#include "ipv6.h"

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_NET_IPV6)
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

static struct net_if *iface;
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
	u8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_test_dev_init(struct device *dev)
{
	return 0;
}

static u8_t *net_test_get_mac(struct device *dev)
{
	struct net_test_mld *context = dev->driver_data;

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
	u8_t *mac = net_test_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

#define NET_ICMP_HDR(pkt) ((struct net_icmp_hdr *)net_pkt_icmp_data(pkt))

static int tester_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_icmp_hdr *icmp = NET_ICMP_HDR(pkt);

	if (!pkt->frags) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	if (icmp->type == NET_ICMPV6_MLDv2) {
		/* FIXME, add more checks here */

		NET_DBG("Received something....");
		is_join_msg_ok = true;
		is_leave_msg_ok = true;
		is_report_sent = true;

		k_sem_give(&wait_data);
	}

	net_pkt_unref(pkt);

	return 0;
}

struct net_test_mld net_test_data;

static struct net_if_api net_test_if_api = {
	.init = net_test_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(net_test_mld, "net_test_mld",
		net_test_dev_init, &net_test_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_test_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE,
		127);

static void group_joined(struct net_mgmt_event_callback *cb,
			 u32_t nm_event, struct net_if *iface)
{
	if (nm_event != NET_EVENT_IPV6_MCAST_JOIN) {
		/* Spurious callback. */
		return;
	}

	is_group_joined = true;

	k_sem_give(&wait_data);
}

static void group_left(struct net_mgmt_event_callback *cb,
			 u32_t nm_event, struct net_if *iface)
{
	if (nm_event != NET_EVENT_IPV6_MCAST_LEAVE) {
		/* Spurious callback. */
		return;
	}

	is_group_left = true;

	k_sem_give(&wait_data);
}

static struct mgmt_events {
	u32_t event;
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

static void mld_setup(void)
{
	struct net_if_addr *ifaddr;

	setup_mgmt_events();

	iface = net_if_get_default();

	zassert_not_null(iface, "Interface is NULL");

	ifaddr = net_if_ipv6_addr_add(iface, &my_addr,
				      NET_ADDR_MANUAL, 0);

	zassert_not_null(ifaddr, "Cannot add IPv6 address");
}

static void join_group(void)
{
	int ret;

	/* Using adhoc multicast group outside standard range */
	net_ipv6_addr_create(&mcast_addr, 0xff10, 0, 0, 0, 0, 0, 0, 0x0001);

	ret = net_ipv6_mld_join(iface, &mcast_addr);

	if (ignore_already) {
		zassert_true(ret == 0 || ret == -EALREADY,
			     "Cannot join IPv6 multicast group");
	} else {
		zassert_equal(ret, 0, "Cannot join IPv6 multicast group");
	}

	k_yield();
}

static void leave_group(void)
{
	int ret;

	net_ipv6_addr_create(&mcast_addr, 0xff10, 0, 0, 0, 0, 0, 0, 0x0001);

	ret = net_ipv6_mld_leave(iface, &mcast_addr);

	zassert_equal(ret, 0, "Cannot leave IPv6 multicast group");

	k_yield();
}

static void catch_join_group(void)
{
	is_group_joined = false;

	ignore_already = false;

	join_group();

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		zassert_true(0, "Timeout while waiting join event");
	}

	if (!is_group_joined) {
		zassert_true(0, "Did not catch join event");
	}

	is_group_joined = false;
}

static void catch_leave_group(void)
{
	is_group_joined = false;

	leave_group();

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		zassert_true(0, "Timeout while waiting leave event");
	}

	if (!is_group_left) {
		zassert_true(0, "Did not catch leave event");
	}

	is_group_left = false;
}

static void verify_join_group(void)
{
	is_join_msg_ok = false;

	ignore_already = false;

	join_group();

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		zassert_true(0, "Timeout while waiting join event");
	}

	if (!is_join_msg_ok) {
		zassert_true(0, "Join msg invalid");
	}

	is_join_msg_ok = false;
}

static void verify_leave_group(void)
{
	is_leave_msg_ok = false;

	leave_group();

	if (k_sem_take(&wait_data, WAIT_TIME)) {
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
	u16_t pos;

	/* Sent to all MLDv2-capable routers */
	net_ipv6_addr_create(&dst, 0xff02, 0, 0, 0, 0, 0, 0, 0x0016);

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, &dst),
				     K_FOREVER);

	pkt = net_ipv6_create(pkt,
			      &peer_addr,
			      &dst,
			      iface,
			      NET_IPV6_NEXTHDR_HBHO);

	NET_IPV6_HDR(pkt)->hop_limit = 1; /* RFC 3810 ch 7.4 */

	/* Add hop-by-hop option and router alert option, RFC 3810 ch 5. */
	net_pkt_append_u8(pkt, IPPROTO_ICMPV6);
	net_pkt_append_u8(pkt, 0); /* length (0 means 8 bytes) */

#define ROUTER_ALERT_LEN 8

	/* IPv6 router alert option is described in RFC 2711. */
	net_pkt_append_be16(pkt, 0x0502); /* RFC 2711 ch 2.1 */
	net_pkt_append_be16(pkt, 0); /* pkt contains MLD msg */

	net_pkt_append_u8(pkt, 1); /* padn */
	net_pkt_append_u8(pkt, 0); /* padn len */

	/* ICMPv6 header */
	net_pkt_append_u8(pkt, NET_ICMPV6_MLD_QUERY); /* type */
	net_pkt_append_u8(pkt, 0); /* code */
	net_pkt_append_be16(pkt, 0); /* chksum */

	net_pkt_append_be16(pkt, 3); /* maximum response code */
	net_pkt_append_be16(pkt, 0); /* reserved field */

	net_pkt_append_all(pkt, sizeof(struct in6_addr),
		       (const u8_t *)net_ipv6_unspecified_address(),
		       K_FOREVER); /* multicast address */

	net_pkt_append_be16(pkt, 0); /* Resv, S, QRV and QQIC */
	net_pkt_append_be16(pkt, 0); /* number of addresses */

	net_ipv6_finalize(pkt, NET_IPV6_NEXTHDR_HBHO);

	net_pkt_set_iface(pkt, iface);

	net_pkt_write_be16(pkt, pkt->frags,
			   NET_IPV6H_LEN + ROUTER_ALERT_LEN + 2,
			   &pos, ntohs(~net_calc_chksum_icmpv6(pkt)));

	net_recv_data(iface, pkt);
}

/* We are not really interested to parse the query at this point */
static enum net_verdict handle_mld_query(struct net_pkt *pkt)
{
	is_query_received = true;

	return NET_DROP;
}

static struct net_icmpv6_handler mld_query_input_handler = {
	.type = NET_ICMPV6_MLD_QUERY,
	.code = 0,
	.handler = handle_mld_query,
};

static void catch_query(void)
{
	is_query_received = false;

	net_icmpv6_register_handler(&mld_query_input_handler);

	send_query(net_if_get_default());

	k_yield();

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		zassert_true(0, "Timeout while waiting query event");
	}

	if (!is_query_received) {
		zassert_true(0, "Query msg invalid");
	}

	is_query_received = false;
}

static void verify_send_report(void)
{
	/* We need to remove our temporary handler so that the
	 * stack handler is called instead.
	 */
	net_icmpv6_unregister_handler(&mld_query_input_handler);

	is_query_received = false;
	is_report_sent = false;

	ignore_already = true;

	join_group();

	send_query(net_if_get_default());

	k_yield();

	/* Did we send a report? */
	if (k_sem_take(&wait_data, WAIT_TIME)) {
		zassert_true(0, "Timeout while waiting report");
	}

	if (!is_report_sent) {
		zassert_true(0, "Report not sent");
	}
}

/* This value should be longer that the one in net_if.c when DAD timeouts */
#define DAD_TIMEOUT (MSEC_PER_SEC / 5)

static void test_allnodes(void)
{
	struct net_if *iface = NULL;
	struct net_if_mcast_addr *ifmaddr;
	struct in6_addr addr;

	net_ipv6_addr_create_ll_allnodes_mcast(&addr);

	/* Let the DAD succeed so that the multicast address will be there */
	k_sleep(DAD_TIMEOUT);

	ifmaddr = net_if_ipv6_maddr_lookup(&addr, &iface);

	zassert_not_null(ifmaddr, "Interface does not contain "
			"allnodes multicast address");
}

static void test_solicit_node(void)
{
	struct net_if *iface = NULL;
	struct net_if_mcast_addr *ifmaddr;
	struct in6_addr addr;

	net_ipv6_addr_create_solicited_node(&my_addr, &addr);

	ifmaddr = net_if_ipv6_maddr_lookup(&addr, &iface);

	zassert_not_null(ifmaddr, "Interface does not contain "
			"solicit node multicast address");
}

void test_main(void)
{
	ztest_test_suite(net_mld_test,
			 ztest_unit_test(mld_setup),
			 ztest_unit_test(join_group),
			 ztest_unit_test(leave_group),
			 ztest_unit_test(catch_join_group),
			 ztest_unit_test(catch_leave_group),
			 ztest_unit_test(verify_join_group),
			 ztest_unit_test(verify_leave_group),
			 ztest_unit_test(catch_query),
			 ztest_unit_test(verify_send_report),
			 ztest_unit_test(test_allnodes),
			 ztest_unit_test(test_solicit_node)
			 );

	ztest_run_test_suite(net_mld_test);
}
