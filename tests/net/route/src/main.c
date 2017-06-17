/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <linker/sections.h>

#include <tc_util.h>

#include <net/ethernet.h>
#include <net/buf.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/net_context.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"
#include "icmpv6.h"
#include "ipv6.h"
#include "nbr.h"
#include "route.h"

#if defined(CONFIG_NET_DEBUG_ROUTE)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static struct net_context *udp_ctx;

/* Interface 1 is the default host and it has my_addr assigned to it */
static struct in6_addr my_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Interface 2 is the secondary host for peer device with address peer_addr */
static struct in6_addr peer_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				    0, 0, 0, 0, 0x0b, 0x0e, 0x0e, 0x3 } } };

/* The dest_addr is only reachable via peer_addr */
static struct in6_addr dest_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					 0, 0, 0, 0, 0xd, 0xe, 0x5, 0x7 } } };

/* Extra address is assigned to ll_addr */
static struct in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
				       0x04 } } };

static struct in6_addr in6addr_mcast = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					     0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Generic address that we are using to generate some more addresses */
static struct in6_addr generic_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					    0, 0, 0, 0, 0xbe, 0xef, 0, 0 } } };

static struct net_if *recipient;
static struct net_if *my_iface;
static struct net_if *peer_iface;

static struct net_route_entry *entry;

#define MAX_ROUTES CONFIG_NET_MAX_ROUTES
static const int max_routes = MAX_ROUTES;
static struct net_route_entry *test_routes[MAX_ROUTES];
static struct in6_addr dest_addresses[MAX_ROUTES];

static bool test_failed;
static bool data_failure;
static bool feed_data; /* feed data back to IP stack */

static int msg_sending;

static struct k_sem wait_data;

#define WAIT_TIME 250

struct net_route_test {
	u8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_route_dev_init(struct device *dev)
{
	return 0;
}

static u8_t *net_route_get_mac(struct device *dev)
{
	struct net_route_test *route = dev->driver_data;

	if (route->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		route->mac_addr[0] = 0x00;
		route->mac_addr[1] = 0x00;
		route->mac_addr[2] = 0x5E;
		route->mac_addr[3] = 0x00;
		route->mac_addr[4] = 0x53;
		route->mac_addr[5] = sys_rand32_get();
	}

	route->ll_addr.addr = route->mac_addr;
	route->ll_addr.len = 6;

	return route->mac_addr;
}

static void net_route_iface_init(struct net_if *iface)
{
	u8_t *mac = net_route_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static int tester_send(struct net_if *iface, struct net_pkt *pkt)
{
	if (!pkt->frags) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	/* By default we assume that the test is ok */
	data_failure = false;

	if (feed_data) {
		DBG("Received at iface %p and feeding it into iface %p\n",
		    iface, recipient);

		if (net_recv_data(recipient, pkt) < 0) {
			TC_ERROR("Data receive failed.");
			net_pkt_unref(pkt);
			test_failed = true;
		}

		k_sem_give(&wait_data);

		return 0;
	}

	DBG("pkt %p to be sent len %lu\n", pkt, net_pkt_get_len(pkt));

	net_pkt_unref(pkt);

	if (data_failure) {
		test_failed = true;
	}

	msg_sending = 0;

	k_sem_give(&wait_data);

	return 0;
}

static int tester_send_peer(struct net_if *iface, struct net_pkt *pkt)
{
	if (!pkt->frags) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	/* By default we assume that the test is ok */
	data_failure = false;

	if (feed_data) {
		DBG("Received at iface %p and feeding it into iface %p\n",
		    iface, recipient);

		if (net_recv_data(recipient, pkt) < 0) {
			TC_ERROR("Data receive failed.");
			net_pkt_unref(pkt);
			test_failed = true;
		}

		k_sem_give(&wait_data);

		return 0;
	}

	DBG("pkt %p to be sent len %lu\n", pkt, net_pkt_get_len(pkt));

	net_pkt_unref(pkt);

	if (data_failure) {
		test_failed = true;
	}

	msg_sending = 0;

	k_sem_give(&wait_data);

	return 0;
}

struct net_route_test net_route_data;
struct net_route_test net_route_data_peer;

static struct net_if_api net_route_if_api = {
	.init = net_route_iface_init,
	.send = tester_send,
};

static struct net_if_api net_route_if_api_peer = {
	.init = net_route_iface_init,
	.send = tester_send_peer,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(net_route_test, "net_route_test", host,
		 net_route_dev_init, &net_route_data, NULL,
		 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		 &net_route_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE, 127);

NET_DEVICE_INIT_INSTANCE(net_route_test_peer, "net_route_test_peer", peer,
		 net_route_dev_init, &net_route_data_peer, NULL,
		 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		 &net_route_if_api_peer, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE, 127);

static bool test_init(void)
{
	struct net_if_mcast_addr *maddr;
	struct net_if_addr *ifaddr;
	int i;

	my_iface = net_if_get_default();
	peer_iface = net_if_get_default() + 1;

	DBG("Interfaces: [%d] my %p, [%d] peer %p\n",
	    net_if_get_by_iface(my_iface), my_iface,
	    net_if_get_by_iface(peer_iface), peer_iface);

	if (!my_iface) {
		TC_ERROR("Interface %p is NULL\n", my_iface);
		return false;
	}

	if (!peer_iface) {
		TC_ERROR("Interface %p is NULL\n", peer_iface);
		return false;
	}

	ifaddr = net_if_ipv6_addr_add(my_iface, &my_addr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		TC_ERROR("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr));
		return false;
	}

	/* For testing purposes we need to set the adddresses preferred */
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(my_iface, &ll_addr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		TC_ERROR("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&ll_addr));
		return false;
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_ipv6_addr_create(&in6addr_mcast, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);

	maddr = net_if_ipv6_maddr_add(my_iface, &in6addr_mcast);
	if (!maddr) {
		TC_ERROR("Cannot add multicast IPv6 address %s\n",
		       net_sprint_ipv6_addr(&in6addr_mcast));
		return false;
	}

	/* The peer and dest interfaces are just simulated, they are not
	 * really used so we should not add IP addresses for them.
	 */

	/* Some test addresses are generated */
	for (i = 0; i < max_routes; i++) {
		memcpy(&dest_addresses[i], &generic_addr,
		       sizeof(struct in6_addr));

		dest_addresses[i].s6_addr[14] = i + 1;
		dest_addresses[i].s6_addr[15] = sys_rand32_get();
	}

	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);

	return true;
}

static bool net_ctx_create(void)
{
	int ret;

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_ctx);
	if (ret != 0) {
		TC_ERROR("Context create IPv6 UDP test failed (%d vs %d)\n",
		       ret, 0);
		return false;
	}

	return true;
}

static bool net_test_send_ns(struct net_if *iface,
			     struct in6_addr *addr)
{
	int ret;

	ret = net_ipv6_send_ns(iface,
			       NULL,
			       addr,
			       &my_addr,
			       &my_addr,
			       false);
	if (ret < 0) {
		TC_ERROR("Cannot send NS (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_test_nbr_lookup_ok(struct net_if *iface,
				   struct in6_addr *addr)
{
	struct net_nbr *nbr;

	nbr = net_ipv6_nbr_lookup(iface, addr);
	if (!nbr) {
		TC_ERROR("Neighbor %s not found in cache\n",
			 net_sprint_ipv6_addr(addr));
		return false;
	}

	return true;
}

static bool populate_nbr_cache(void)
{
	struct net_nbr *nbr;

	msg_sending = NET_ICMPV6_NS;
	feed_data = true;
	data_failure = false;

	recipient = my_iface;

	if (!net_test_send_ns(peer_iface, &peer_addr)) {
		return false;
	}

	nbr = net_ipv6_nbr_add(net_if_get_default(),
			       &peer_addr,
			       &net_route_data_peer.ll_addr,
			       false,
			       NET_IPV6_NBR_STATE_REACHABLE);
	if (!nbr) {
		TC_ERROR("Cannot add peer to neighbor cache\n");
		return false;
	}

	k_sem_take(&wait_data, WAIT_TIME);

	feed_data = false;

	if (data_failure) {
		data_failure = false;
		return false;
	}

	data_failure = false;

	if (!net_test_nbr_lookup_ok(my_iface, &peer_addr)) {
		return false;
	}

	return true;
}

static bool route_add(void)
{
	entry = net_route_add(my_iface,
			      &dest_addr, 128,
			      &peer_addr);

	if (!entry) {
		TC_ERROR("Route add failed\n");
		return false;
	}

	return true;
}

static bool route_update(void)
{
	struct net_route_entry *update_entry;

	update_entry = net_route_add(my_iface,
				     &dest_addr, 128,
				     &peer_addr);

	if (update_entry != entry) {
		TC_ERROR("Route add again failed\n");
		return false;
	}

	return true;
}

static bool route_del(void)
{
	int ret;

	ret = net_route_del(entry);
	if (ret < 0) {
		TC_ERROR("Route del failed %d\n", ret);
		return false;
	}

	return true;
}

static bool route_del_again(void)
{
	int ret;

	ret = net_route_del(entry);
	if (ret >= 0) {
		TC_ERROR("Route del again failed %d\n", ret);
		return false;
	}

	return true;
}

static bool route_get_nexthop(void)
{
	struct in6_addr *nexthop;

	nexthop = net_route_get_nexthop(entry);

	if (!nexthop) {
		TC_ERROR("Route get nexthop failed\n");
		return false;
	}

	if (!net_ipv6_addr_cmp(nexthop, &peer_addr)) {
		TC_ERROR("Route nexthop does not match\n");
		return false;
	}

	return true;
}

static bool route_lookup_ok(void)
{
	struct net_route_entry *entry;

	entry = net_route_lookup(my_iface, &dest_addr);
	if (!entry) {
		TC_ERROR("Route lookup failed\n");
		return false;
	}

	return true;
}

static bool route_lookup_fail(void)
{
	struct net_route_entry *entry;

	entry = net_route_lookup(my_iface, &peer_addr);
	if (entry) {
		TC_ERROR("Route lookup failed for peer address\n");
		return false;
	}

	return true;
}

static bool route_del_nexthop(void)
{
	struct in6_addr *nexthop = &peer_addr;
	int ret;

	ret = net_route_del_by_nexthop(my_iface, nexthop);
	if (ret <= 0) {
		TC_ERROR("Route del nexthop failed (%d)\n", ret);
		return false;
	}

	return true;
}

static bool route_del_nexthop_again(void)
{
	struct in6_addr *nexthop = &peer_addr;
	int ret;

	ret = net_route_del_by_nexthop(my_iface, nexthop);
	if (ret >= 0) {
		TC_ERROR("Route del again nexthop failed (%d)\n", ret);
		return false;
	}

	return true;
}

static bool route_add_many(void)
{
	int i;

	for (i = 0; i < max_routes; i++) {
		DBG("Adding route %d addr %s\n", i + 1,
		    net_sprint_ipv6_addr(&dest_addresses[i]));
		test_routes[i] = net_route_add(my_iface,
					  &dest_addresses[i], 128,
					  &peer_addr);
		if (!test_routes[i]) {
			TC_ERROR("[%d] Route add failed\n", i);
			return false;
		}
	}

	return true;
}

static bool route_del_many(void)
{
	int i, ret;

	for (i = 0; i < max_routes; i++) {
		DBG("Deleting route %d addr %s\n", i + 1,
		    net_sprint_ipv6_addr(&dest_addresses[i]));
		ret = net_route_del(test_routes[i]);
		if (ret) {
			TC_ERROR("[%d] Route del failed (%d)\n", i, ret);
			return false;
		}
	}

	return true;
}

static const struct {
	const char *name;
	bool (*func)(void);
} tests[] = {
	{ "test init", test_init },
	{ "test ctx create", net_ctx_create },
	{ "Populate neighbor cache", populate_nbr_cache },
	{ "Add route", route_add },
	{ "Add update", route_update },
	{ "Get nexthop", route_get_nexthop },
	{ "Lookup route ok", route_lookup_ok },
	{ "Lookup route fail", route_lookup_fail },
	{ "Del route", route_del },
	{ "Add route again", route_add },
	{ "Del route by nexthop", route_del_nexthop },
	{ "Del route again", route_del_again },
	{ "Del route by nexthop again", route_del_nexthop_again },
	/* Add the neighbors back, otherwise the rest of the tests will fail */
	{ "Populate neighbor cache again", populate_nbr_cache },
	{ "Add many routes", route_add_many },
	{ "Del many routes", route_del_many },
};

void main(void)
{
	int count, pass;

	for (count = 0, pass = 0; count < ARRAY_SIZE(tests); count++) {
		TC_START(tests[count].name);
		test_failed = false;
		if (!tests[count].func() || test_failed) {
			TC_END(FAIL, "failed\n");
		} else {
			TC_END(PASS, "passed\n");
			pass++;
		}
	}

	TC_END_REPORT(((pass != ARRAY_SIZE(tests)) ? TC_FAIL : TC_PASS));
}
