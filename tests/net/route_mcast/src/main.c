/* main.c - Application main entry point */

/*
 * Copyright (c) 2020 Lemonbeat GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_ROUTE_LOG_LEVEL);

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/linker/sections.h>
#include <zephyr/random/random.h>

#include <zephyr/tc_util.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_context.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"
#include "icmpv6.h"
#include "ipv6.h"
#include <zephyr/net/udp.h>
#include "udp_internal.h"
#include "nbr.h"
#include "route.h"

#if defined(CONFIG_NET_ROUTE_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static struct in6_addr iface_1_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x1 } } };

static struct in6_addr iface_2_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				    0, 0, 0, 0, 0x0b, 0x0e, 0x0e, 0x3 } } };

static struct in6_addr iface_3_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				    0, 0, 0, 0, 0x0e, 0x0e, 0x0e, 0x4 } } };

/* Extra address is assigned to ll_addr */
static struct in6_addr ll_addr_1 = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0xf2,
					0xaa, 0x29, 0x02, 0x04 } } };

static struct in6_addr ll_addr_2 = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2,
					   0xaa, 0x29, 0x05, 0x06 } } };

static struct in6_addr ll_addr_3 = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2,
					   0xaa, 0x29, 0x07, 0x08 } } };

static struct in6_addr in6addr_mcast = { { { 0xff, 0x02, 0, 0, 0, 0, 0, 0,
					     0, 0, 0, 0, 0, 0, 0, 0x1 } } };

static struct net_if *iface_1;
static struct net_if *iface_2;
static struct net_if *iface_3;

#define WAIT_TIME K_MSEC(50)

struct net_route_mcast_iface_cfg {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

#define MAX_MCAST_ROUTES CONFIG_NET_MAX_MCAST_ROUTES

static struct net_route_entry_mcast *test_mcast_routes[MAX_MCAST_ROUTES];

static struct in6_addr mcast_prefix_iflocal = { { {
					0xFF, 0x01, 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 0 } } };
static struct in6_addr mcast_prefix_llocal = { { {
					0xFF, 0x02, 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 0 } } };
static struct in6_addr mcast_prefix_admin = { { {
					0xFF, 0x04, 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 0 } } };
static struct in6_addr mcast_prefix_site_local = { { {
					0xFF, 0x05, 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 0 } } };
static struct in6_addr mcast_prefix_orga = { { {
					0xFF, 0x08, 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 0 } } };
static struct in6_addr mcast_prefix_global = { { {
					0xFF, 0x0E, 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 0 } } };
/*
 * full network prefix based address,
 * see RFC-3306 for details
 * FF3F:40:FD01:101:: \128
 * network prefix FD01:101::\64
 */
static struct in6_addr mcast_prefix_nw_based = { { {
		0xFF, 0x3F, 0, 0x40,
		0xFD, 0x01, 0x01, 0x01,
		0, 0, 0, 0,
		0, 0, 0, 0 } } };

static uint8_t forwarding_counter;
static bool iface_1_forwarded;
static bool iface_2_forwarded;
static bool iface_3_forwarded;

struct net_route_mcast_scenario_cfg {
	struct in6_addr src;
	struct in6_addr mcast;
	bool is_active;
};

static struct net_route_mcast_scenario_cfg active_scenario;

int net_route_mcast_dev_init(const struct device *dev)
{
	return 0;
}

static uint8_t *net_route_mcast_get_mac(const struct device *dev)
{
	struct net_route_mcast_iface_cfg *cfg = dev->data;

	if (cfg->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		cfg->mac_addr[0] = 0x00;
		cfg->mac_addr[1] = 0x00;
		cfg->mac_addr[2] = 0x5E;
		cfg->mac_addr[3] = 0x00;
		cfg->mac_addr[4] = 0x53;
		cfg->mac_addr[5] = sys_rand8_get();
	}

	cfg->ll_addr.addr = cfg->mac_addr;
	cfg->ll_addr.len = 6U;

	return cfg->mac_addr;
}

static void net_route_mcast_add_addresses(struct net_if *iface,
		struct in6_addr *ipv6, struct in6_addr *ll_addr)
{
	struct net_if_mcast_addr *maddr;
	struct net_if_addr *ifaddr;

	uint8_t *mac = net_route_mcast_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
				     NET_LINK_ETHERNET);

	ifaddr = net_if_ipv6_addr_add(iface, ipv6, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Cannot add global IPv6 address");

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface, ll_addr, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Cannot add ll IPv6 address");

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	maddr = net_if_ipv6_maddr_add(iface, &in6addr_mcast);
	zassert_not_null(maddr, "Cannot add multicast IPv6 address");
}

static void net_route_mcast_iface_init1(struct net_if *iface)
{
	iface_1 = iface;
	net_route_mcast_add_addresses(iface, &iface_1_addr, &ll_addr_1);
}

static void net_route_mcast_iface_init2(struct net_if *iface)
{
	iface_2 = iface;
	net_route_mcast_add_addresses(iface, &iface_2_addr, &ll_addr_2);
}

static void net_route_mcast_iface_init3(struct net_if *iface)
{
	iface_3 = iface;
	net_route_mcast_add_addresses(iface, &iface_3_addr, &ll_addr_3);
}

static bool check_packet_addresses(struct net_pkt *pkt)
{
	struct net_ipv6_hdr *ipv6_hdr = NET_IPV6_HDR(pkt);

	if ((memcmp(&active_scenario.src,
			&ipv6_hdr->src,
			sizeof(struct in6_addr)) != 0) ||
			(memcmp(&active_scenario.mcast,
				ipv6_hdr->dst,
				sizeof(struct in6_addr)) != 0)) {
		return false;
	}

	return true;
}

static int iface_send(const struct device *dev, struct net_pkt *pkt)
{
	if (!active_scenario.is_active) {
		return 0;
	}
	if (!check_packet_addresses(pkt)) {
		return 0;
	}

	forwarding_counter++;

	if (net_pkt_iface(pkt) == iface_1) {
		iface_1_forwarded = true;
	} else if (net_pkt_iface(pkt) == iface_2) {
		iface_2_forwarded = true;
	} else if (net_pkt_iface(pkt) == iface_3) {
		iface_3_forwarded = true;
	}

	return 0;
}

struct net_route_mcast_iface_cfg net_route_data_if1;
struct net_route_mcast_iface_cfg net_route_data_if2;
struct net_route_mcast_iface_cfg net_route_data_if3;

static struct dummy_api net_route_mcast_if_api_1 = {
	.iface_api.init = net_route_mcast_iface_init1,
	.send = iface_send,
};

static struct dummy_api net_route_mcast_if_api_2 = {
	.iface_api.init = net_route_mcast_iface_init2,
	.send = iface_send,
};

static struct dummy_api net_route_mcast_if_api_3 = {
	.iface_api.init = net_route_mcast_iface_init3,
	.send = iface_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(mcast_iface_1, "mcast_iface_1", iface_1,
			net_route_mcast_dev_init, NULL,
			&net_route_data_if1, NULL,
			CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			&net_route_mcast_if_api_1, _ETH_L2_LAYER,
			_ETH_L2_CTX_TYPE, 127);

NET_DEVICE_INIT_INSTANCE(mcast_iface_2, "mcast_iface_2", iface_2,
			net_route_mcast_dev_init, NULL,
			&net_route_data_if2, NULL,
			CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			&net_route_mcast_if_api_2, _ETH_L2_LAYER,
			_ETH_L2_CTX_TYPE, 127);

NET_DEVICE_INIT_INSTANCE(mcast_iface_3, "mcast_iface_3", iface_3,
			net_route_mcast_dev_init, NULL,
			&net_route_data_if3, NULL,
			CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			&net_route_mcast_if_api_3, _ETH_L2_LAYER,
			_ETH_L2_CTX_TYPE, 127);

static struct net_pkt *setup_ipv6_udp(struct net_if *iface,
				      struct in6_addr *src_addr,
				      struct in6_addr *remote_addr,
				      uint16_t src_port, uint16_t remote_port)
{
	static const char payload[] = "foobar";
	struct net_pkt *pkt;
	int res;

	pkt = net_pkt_alloc_with_buffer(iface, strlen(payload), AF_INET6,
					IPPROTO_UDP, K_FOREVER);
	if (!pkt) {
		return NULL;
	}

	net_pkt_set_ipv6_hop_limit(pkt, 2);

	res = net_ipv6_create(pkt, src_addr, remote_addr);
	zassert_equal(0, res, "ipv6 create failed");

	res = net_udp_create(pkt, htons(src_port), htons(remote_port));
	zassert_equal(0, res, "udp create failed");

	res = net_pkt_write(pkt, (uint8_t *) payload, strlen(payload));
	zassert_equal(0, res, "pkt write failed");

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_UDP);
	net_pkt_cursor_init(pkt);

	return pkt;
}

static void test_route_mcast_init(void)
{
	zassert_not_null(iface_1, "Interface is NULL");
	zassert_not_null(iface_2, "Interface is NULL");
	zassert_not_null(iface_3, "Interface is NULL");

	net_if_flag_set(iface_1, NET_IF_FORWARD_MULTICASTS);
	net_if_flag_set(iface_2, NET_IF_FORWARD_MULTICASTS);
	/* iface_3 should not forward multicasts */
}

static void test_route_mcast_route_add(void)
{
	struct in6_addr nw_prefix_based_all_nodes;
	struct net_route_entry_mcast *entry;

	entry = net_route_mcast_add(iface_1, &mcast_prefix_iflocal, 16);
	zassert_is_null(entry, "add iface local should fail");

	entry = net_route_mcast_add(iface_1, &mcast_prefix_llocal, 16);
	zassert_is_null(entry, "add link local should fail");

	test_mcast_routes[0] = net_route_mcast_add(iface_1,
				    &mcast_prefix_admin, 16);
	zassert_not_null(test_mcast_routes[0], "mcast route add failed");

	test_mcast_routes[1] = net_route_mcast_add(iface_2,
					      &mcast_prefix_site_local, 16);
	zassert_not_null(test_mcast_routes[1], "mcast route add failed");

	test_mcast_routes[2] = net_route_mcast_add(iface_1,
						      &mcast_prefix_orga, 16);
	zassert_not_null(test_mcast_routes[2], "mcast route add failed");

	test_mcast_routes[3] = net_route_mcast_add(iface_2,
						      &mcast_prefix_global, 16);
	zassert_not_null(test_mcast_routes[3], "mcast route add failed");

	/* check if route can be added
	 * if forwarding flag not set on iface
	 */
	test_mcast_routes[4] = net_route_mcast_add(iface_3,
			&mcast_prefix_global, 16);
	zassert_is_null(test_mcast_routes[4], "mcast route add should fail");

	test_mcast_routes[4] = net_route_mcast_add(iface_1,
				&mcast_prefix_nw_based, 96);
	zassert_not_null(test_mcast_routes[4],
			"add for nw prefix based failed");

	memcpy(&nw_prefix_based_all_nodes, &mcast_prefix_nw_based,
			sizeof(struct in6_addr));
	nw_prefix_based_all_nodes.s6_addr[15] = 0x01;

	test_mcast_routes[5] = net_route_mcast_add(iface_2,
				&nw_prefix_based_all_nodes, 128);
	zassert_not_null(test_mcast_routes[5],
			"add for nw prefix based failed");
}

static void mcast_foreach_cb(struct net_route_entry_mcast *entry,
	     void *user_data)
{
	zassert_equal_ptr(user_data, &mcast_prefix_global,
						  "foreach failed, wrong user_data");
}

static void test_route_mcast_foreach(void)
{
	int executed_first = net_route_mcast_foreach(mcast_foreach_cb,
		NULL, &mcast_prefix_global);

	int executed_skip =  net_route_mcast_foreach(mcast_foreach_cb,
			&mcast_prefix_admin, &mcast_prefix_global);

	zassert_true(executed_skip == (executed_first - 1),
			"mcast foreach skip did not skip");
}

static void test_route_mcast_lookup(void)
{
	struct net_route_entry_mcast *route =
			net_route_mcast_lookup(&mcast_prefix_admin);

	zassert_equal_ptr(test_mcast_routes[0], route,
				  "mcast lookup failed");

	route = net_route_mcast_lookup(&mcast_prefix_site_local);

	zassert_equal_ptr(test_mcast_routes[1], route,
					  "mcast lookup failed");

	route = net_route_mcast_lookup(&mcast_prefix_global);

	zassert_equal_ptr(test_mcast_routes[3], route,
						  "mcast lookup failed");
}
static void test_route_mcast_route_del(void)
{
	struct net_route_entry_mcast *route;
	bool success = net_route_mcast_del(test_mcast_routes[0]);

	zassert_true(success, "failed to delete mcast route");

	route = net_route_mcast_lookup(&mcast_prefix_admin);
	zassert_is_null(route, "lookup found deleted route");

	success = net_route_mcast_del(test_mcast_routes[1]);
	zassert_true(success, "failed to delete mcast route");

	route = net_route_mcast_lookup(&mcast_prefix_site_local);
	zassert_is_null(route, "lookup found deleted route");

	success = net_route_mcast_del(test_mcast_routes[2]);
	zassert_true(success, "failed to delete mcast route");

	success = net_route_mcast_del(test_mcast_routes[3]);
	zassert_true(success, "failed to delete mcast route");

	success = net_route_mcast_del(test_mcast_routes[4]);
	zassert_true(success, "failed to delete mcast route");

	success = net_route_mcast_del(test_mcast_routes[5]);
	zassert_true(success, "failed to delete mcast route");
}

static void reset_counters(void)
{
	iface_1_forwarded = false;
	iface_2_forwarded = false;
	iface_3_forwarded = false;
	forwarding_counter = 0;
}

static void test_route_mcast_scenario1(void)
{
	/* scenario 1 site local:
	 * 1.	iface_1 receives site local
	 *		only iface_2 forwards
	 * 2.	iface_3 receives site_local
	 *		only iface_2 forwards
	 */
	reset_counters();
	memcpy(&active_scenario.src, &iface_1_addr, sizeof(struct in6_addr));
	active_scenario.src.s6_addr[15] = 0x02;

	memcpy(&active_scenario.mcast, &mcast_prefix_site_local,
			sizeof(struct in6_addr));
	active_scenario.mcast.s6_addr[15] = 0x01;

	struct net_pkt *pkt1 = setup_ipv6_udp(iface_1, &active_scenario.src,
				&active_scenario.mcast, 20015, 20001);

	active_scenario.is_active = true;
	if (net_recv_data(iface_1, pkt1) < 0) {
		net_pkt_unref(pkt1);
		zassert_true(0, "failed to receive initial packet!");
	}
	k_sleep(WAIT_TIME);
	net_pkt_unref(pkt1);


	zassert_true(iface_2_forwarded, "iface_2 did not forward");
	zassert_false(iface_1_forwarded, "iface_1 forwarded");
	zassert_false(iface_3_forwarded, "iface_3 forwarded");
	zassert_equal(forwarding_counter, 1,
			"unexpected forwarded packet count");

	reset_counters();

	memcpy(&active_scenario.src, &iface_3_addr, sizeof(struct in6_addr));
	active_scenario.src.s6_addr[15] = 0x09;

	struct net_pkt *pkt2 = setup_ipv6_udp(iface_3, &active_scenario.src,
			&active_scenario.mcast, 20015, 20001);
	if (net_recv_data(iface_3, pkt2) < 0) {
		net_pkt_unref(pkt2);
		zassert_true(0, "failed to receive initial packet!");
	}
	k_sleep(WAIT_TIME);
	net_pkt_unref(pkt2);
	active_scenario.is_active = false;
	zassert_true(iface_2_forwarded, "iface_2 did not forward");
	zassert_false(iface_1_forwarded, "iface_1 forwarded");
	zassert_false(iface_3_forwarded, "iface_3 forwarded");
	zassert_equal(forwarding_counter, 1,
			"unexpected forwarded packet count");
	reset_counters();
}

static void test_route_mcast_scenario2(void)
{
	/*
	 *  scenario 2 admin local:
	 *  1.	iface_1 receives
	 *		iface_2 must not forward due to missing routing entry.
	 *		iface_3 must not forward due to missing
	 *			routing entry and missing flag.
	 *		iface_1 must not forward because itself
	 *			received the packet!
	 *
	 *  2.	iface_3 receives
	 *		now iface_1 must forward due to routing entry
	 */
	reset_counters();
	memcpy(&active_scenario.src, &iface_1_addr, sizeof(struct in6_addr));
	active_scenario.src.s6_addr[15] = 0x08;

	memcpy(&active_scenario.mcast, &mcast_prefix_admin,
			sizeof(struct in6_addr));
	active_scenario.mcast.s6_addr[15] = 0x01;

	struct net_pkt *pkt = setup_ipv6_udp(iface_1, &active_scenario.src,
			&active_scenario.mcast, 215, 201);

	active_scenario.is_active = true;
	if (net_recv_data(iface_1, pkt) < 0) {
		net_pkt_unref(pkt);
		zassert_true(0, "failed to receive initial packet!");
	}
	k_sleep(WAIT_TIME);
	net_pkt_unref(pkt);

	zassert_false(iface_1_forwarded, "iface_1 forwarded");
	zassert_false(iface_2_forwarded, "iface_2 forwarded");
	zassert_false(iface_3_forwarded, "iface_3 forwarded");
	zassert_equal(forwarding_counter, 0, "wrong count forwarded packets");

	reset_counters();
	memcpy(&active_scenario.src, &iface_3_addr, sizeof(struct in6_addr));
	active_scenario.src.s6_addr[15] = 0x08;

	struct net_pkt *pkt2 = setup_ipv6_udp(iface_3, &active_scenario.src,
			&active_scenario.mcast, 215, 201);
	if (net_recv_data(iface_3, pkt2) < 0) {
		net_pkt_unref(pkt2);
		zassert_true(0, "failed to receive initial packet!");
	}
	k_sleep(WAIT_TIME);
	active_scenario.is_active = false;
	net_pkt_unref(pkt2);

	zassert_true(iface_1_forwarded, "iface_1 did not forward");
	zassert_false(iface_2_forwarded, "iface_2 forwarded");
	zassert_false(iface_3_forwarded, "iface_3 forwarded");
	zassert_equal(forwarding_counter, 1, "wrong count forwarded packets");
}

static void test_route_mcast_scenario3(void)
{
	/*
	 * scenario 3: network prefix based forwarding
	 * 1.	iface 3 receives nw prefix based all nodes
	 *		iface 1 + 2 forwarding because all nodes group
	 * 2.	iface 3 receives nw prefix based custom group
	 *		only iface 1 forwards
	 *		iface 3 route is set to all nodes
	 * 3.	iface 3 receives all nodes group with different prefix
	 *		no iface forwards
	 */
	reset_counters();
	memcpy(&active_scenario.src, &iface_3_addr, sizeof(struct in6_addr));
	active_scenario.src.s6_addr[15] = 0x08;

	memcpy(&active_scenario.mcast, &mcast_prefix_nw_based,
				sizeof(struct in6_addr));
	active_scenario.mcast.s6_addr[15] = 0x01;

	struct net_pkt *pkt = setup_ipv6_udp(iface_3, &active_scenario.src,
				&active_scenario.mcast, 215, 201);

	active_scenario.is_active = true;
	if (net_recv_data(iface_3, pkt) < 0) {
		net_pkt_unref(pkt);
		zassert_true(0, "failed to receive initial packet!");
	}
	k_sleep(WAIT_TIME);
	net_pkt_unref(pkt);
	active_scenario.is_active = false;

	zassert_true(iface_1_forwarded, "iface_1 did not forward");
	zassert_true(iface_2_forwarded, "iface_2 did not forward");
	zassert_false(iface_3_forwarded, "iface_3 forwarded");
	zassert_equal(forwarding_counter, 2, "wrong count forwarded packets");

	reset_counters();
	/* set to custom group id */
	active_scenario.mcast.s6_addr[15] = 0x0F;
	struct net_pkt *pkt2 = setup_ipv6_udp(iface_3, &active_scenario.src,
					&active_scenario.mcast, 215, 201);

	active_scenario.is_active = true;
	if (net_recv_data(iface_3, pkt2) < 0) {
		net_pkt_unref(pkt2);
		zassert_true(0, "failed to receive initial packet!");
	}
	k_sleep(WAIT_TIME);
	net_pkt_unref(pkt2);
	active_scenario.is_active = false;

	zassert_true(iface_1_forwarded, "iface_1 did not forward");
	zassert_false(iface_2_forwarded, "iface_2 forwarded");
	zassert_false(iface_3_forwarded, "iface_3 forwarded");
	zassert_equal(forwarding_counter, 1, "wrong count forwarded packets");

	reset_counters();

	/* set to all nodes but different prefix */
	active_scenario.mcast.s6_addr[11] = 0x0F;
	active_scenario.mcast.s6_addr[15] = 0x01;
	struct net_pkt *pkt3 = setup_ipv6_udp(iface_3, &active_scenario.src,
			&active_scenario.mcast, 215, 201);

	active_scenario.is_active = true;
	if (net_recv_data(iface_3, pkt3) < 0) {
		net_pkt_unref(pkt3);
		zassert_true(0, "failed to receive initial packet!");
	}
	k_sleep(WAIT_TIME);
	net_pkt_unref(pkt3);
	active_scenario.is_active = false;

	zassert_false(iface_1_forwarded, "iface_1 forwarded");
	zassert_false(iface_2_forwarded, "iface_2 forwarded");
	zassert_false(iface_3_forwarded, "iface_3 forwarded");
	zassert_equal(forwarding_counter, 0, "wrong count forwarded packets");
}

void test_route_mcast_multiple_route_ifaces(void)
{
	/*
	 * Scenario:
	 *    1. Verify that multicast packet sent to site-local scoped address
	 *       to the iface_3 is forwarded only to iface_2 as configured in
	 *       test_route_mcast_route_add() test case.
	 *    2. Verify that interface without NET_IF_FORWARD_MULTICASTS flag
	 *       enabled cannot be added to multicast routing entry.
	 *    3. Add iface_1 to multicast routing entry for site-local scope.
	 *    4. Verify that packet sent to the same scope as before is now
	 *       forwarded also to iface_1.
	 *    5. Remove iface_1 from the multicast routing entry.
	 *    6. Verify that packet sent to the same scope is before is now
	 *       NOT forwarded to iface_1 as it was removed from the list.
	 */
	struct net_route_entry_mcast *route;
	bool res;

	reset_counters();
	memcpy(&active_scenario.src, &iface_3_addr, sizeof(struct in6_addr));
	active_scenario.src.s6_addr[15] = 0x02;

	memcpy(&active_scenario.mcast, &mcast_prefix_site_local, sizeof(struct in6_addr));
	active_scenario.mcast.s6_addr[15] = 0x01;

	struct net_pkt *pkt = setup_ipv6_udp(iface_3, &active_scenario.src, &active_scenario.mcast,
					     20015, 20001);

	active_scenario.is_active = true;
	if (net_recv_data(iface_3, pkt) < 0) {
		net_pkt_unref(pkt);
		zassert_true(0, "failed to receive initial packet!");
	}
	k_sleep(WAIT_TIME);
	net_pkt_unref(pkt);
	active_scenario.is_active = false;

	zassert_true(iface_2_forwarded, "iface_2 did not forward");
	zassert_false(iface_1_forwarded, "iface_1 forwarded");
	zassert_false(iface_3_forwarded, "iface_3 forwarded");
	zassert_equal(forwarding_counter, 1, "unexpected forwarded packet count");

	reset_counters();

	route = net_route_mcast_lookup(&mcast_prefix_site_local);
	zassert_not_null(route, "failed to find the route entry");

	/* Add iface_1 to the entry */
	res = net_route_mcast_iface_add(route, iface_1);
	zassert_true(res, "failed to add iface_1 to the entry");

	struct net_pkt *pkt2 = setup_ipv6_udp(iface_3, &active_scenario.src,
					      &active_scenario.mcast, 215, 201);

	active_scenario.is_active = true;
	if (net_recv_data(iface_3, pkt2) < 0) {
		net_pkt_unref(pkt2);
		zassert_true(0, "failed to receive initial packet!");
	}
	k_sleep(WAIT_TIME);
	net_pkt_unref(pkt2);
	active_scenario.is_active = false;

	zassert_true(iface_2_forwarded, "iface_2 did not forward");
	zassert_true(iface_1_forwarded, "iface_1 did not forward");
	zassert_false(iface_3_forwarded, "iface_3 forwarded");
	zassert_equal(forwarding_counter, 2, "unexpected forwarded packet count");

	reset_counters();

	/* Remove iface_1 from the entry */
	res = net_route_mcast_iface_del(route, iface_1);
	zassert_true(res, "failed to remove iface_1 from the entry");

	struct net_pkt *pkt3 = setup_ipv6_udp(iface_3, &active_scenario.src,
					      &active_scenario.mcast, 215, 201);

	active_scenario.is_active = true;
	if (net_recv_data(iface_3, pkt3) < 0) {
		net_pkt_unref(pkt3);
		zassert_true(0, "failed to receive initial packet!");
	}
	k_sleep(WAIT_TIME);
	net_pkt_unref(pkt3);
	active_scenario.is_active = false;

	zassert_true(iface_2_forwarded, "iface_2 did not forward");
	zassert_false(iface_1_forwarded, "iface_1 forwarded");
	zassert_false(iface_3_forwarded, "iface_3 forwarded");
	zassert_equal(forwarding_counter, 1, "unexpected forwarded packet count");
}

/*test case main entry*/
ZTEST(route_mcast_test_suite, test_route_mcast)
{
	test_route_mcast_init();
	test_route_mcast_route_add();
	test_route_mcast_foreach();
	test_route_mcast_scenario1();
	test_route_mcast_scenario2();
	test_route_mcast_scenario3();
	test_route_mcast_multiple_route_ifaces();
	test_route_mcast_lookup();
	test_route_mcast_route_del();
}
ZTEST_SUITE(route_mcast_test_suite, NULL, NULL, NULL, NULL, NULL);
