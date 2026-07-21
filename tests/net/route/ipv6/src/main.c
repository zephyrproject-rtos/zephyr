/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_IPV6_ROUTE_LOG_LEVEL);

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
#include <zephyr/net_buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_context.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"
#include "icmpv6.h"
#include "ipv6.h"
#include "nbr.h"
#include "route_ipv6.h"

#if defined(CONFIG_NET_IPV6_ROUTE_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static struct net_context *udp_ctx;

/* Interface 1 is the default host and it has my_addr assigned to it */
static struct net_in6_addr my_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct net_in6_addr my_addr_alt = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					      0, 0, 0, 0, 0, 0, 0, 0x2 } } };

/* Interface 2 is the secondary host for peer device with address peer_addr */
static struct net_in6_addr peer_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				    0, 0, 0, 0, 0x0b, 0x0e, 0x0e, 0x3 } } };

/* Alternate next hop address for dest_addr */
static struct net_in6_addr peer_addr_alt = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				    0, 0, 0, 0, 0x0b, 0x0e, 0x0e, 0x4 } } };

/* The dest_addr is only reachable via peer_addr */
static struct net_in6_addr dest_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					 0, 0, 0, 0, 0xd, 0xe, 0x5, 0x7 } } };
static struct net_in6_addr dest_addr_alt = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
						 0, 0, 0, 0, 0xd, 0xe, 0x5, 0x8 } } };
static struct net_in6_addr forward_src_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
						    0, 0, 0, 0, 0xaa, 0xbb, 0, 0x1 } } };
static struct net_in6_addr forward_nexthop = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
						    0, 0, 0, 0, 0x0b, 0x0e, 0x0e, 0x5 } } };
static struct net_in6_addr forward_dest_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
						     0, 0, 0, 0, 0xd, 0xe, 0x5, 0x9 } } };
static struct net_in6_addr onlink_dest_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
						    0, 0, 0, 0, 0, 0, 0, 0x77 } } };

/* Extra address is assigned to ll_addr */
static struct net_in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
				       0x04 } } };

static struct net_in6_addr in6addr_mcast = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					     0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Generic address that we are using to generate some more addresses */
static struct net_in6_addr generic_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					    0, 0, 0, 0, 0xbe, 0xef, 0, 0 } } };

static struct net_if *recipient;
static struct net_if *my_iface;
static struct net_if *peer_iface;

static struct net_route_entry *route_entry;

#define MAX_ROUTES CONFIG_NET_IPV6_MAX_ROUTES
static const int max_routes = MAX_ROUTES;
static struct net_route_entry *test_routes[MAX_ROUTES];
static struct net_in6_addr dest_addresses[MAX_ROUTES];

static bool test_failed;
static bool data_failure;
static bool feed_data; /* feed data back to IP stack */
static bool sent_pkt_seen;
static bool sent_forwarding;
static struct net_if *sent_iface;
static struct net_if *sent_orig_iface;
static uint8_t sent_ipv6_hop_limit;
static const struct net_in6_addr *expected_ipv6_dst;

static int msg_sending;

K_SEM_DEFINE(wait_data, 0, UINT_MAX);

#define WAIT_TIME K_MSEC(250)

struct net_route_test {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_route_dev_init(const struct device *dev)
{
	return 0;
}

static uint8_t *net_route_get_mac(const struct device *dev)
{
	struct net_route_test *route = dev->data;

	if (route->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		route->mac_addr[0] = 0x00;
		route->mac_addr[1] = 0x00;
		route->mac_addr[2] = 0x5E;
		route->mac_addr[3] = 0x00;
		route->mac_addr[4] = 0x53;
		route->mac_addr[5] = sys_rand8_get();
	}

	memcpy(route->ll_addr.addr, route->mac_addr, sizeof(route->mac_addr));
	route->ll_addr.len = sizeof(route->mac_addr);

	return route->mac_addr;
}

static void net_route_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_route_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	bool notify_sender = true;

	if (!pkt->frags) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	/* By default we assume that the test is ok */
	data_failure = false;
	sent_pkt_seen = true;
	sent_iface = net_if_lookup_by_dev(dev);
	sent_orig_iface = net_pkt_orig_iface(pkt);
	sent_forwarding = net_pkt_forwarding(pkt);
	sent_ipv6_hop_limit = 0U;

	if (net_pkt_family(pkt) == NET_AF_INET6 &&
	    net_pkt_get_len(pkt) >= sizeof(struct net_ipv6_hdr)) {
		sent_ipv6_hop_limit = NET_IPV6_HDR(pkt)->hop_limit;

		if (expected_ipv6_dst != NULL &&
		    !net_ipv6_addr_cmp_raw(NET_IPV6_HDR(pkt)->dst,
					   expected_ipv6_dst->s6_addr)) {
			notify_sender = false;
		}
	} else if (expected_ipv6_dst != NULL) {
		notify_sender = false;
	}

	if (feed_data) {
		DBG("Received at iface %p and feeding it into iface %p\n",
		    net_pkt_iface(pkt), recipient);

		net_pkt_ref(pkt);

		if (net_recv_data(recipient, pkt) < 0) {
			TC_ERROR("Data receive failed.");
			net_pkt_unref(pkt);
			test_failed = true;
		}

		goto out;
	}

	DBG("pkt %p to be sent len %lu\n", pkt, net_pkt_get_len(pkt));

	if (data_failure) {
		test_failed = true;
	}

	msg_sending = 0;
out:
	if (notify_sender) {
		k_sem_give(&wait_data);
	}

	return 0;
}

static int tester_send_peer(const struct device *dev, struct net_pkt *pkt)
{
	bool notify_sender = true;

	if (!pkt->frags) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	/* By default we assume that the test is ok */
	data_failure = false;
	sent_pkt_seen = true;
	sent_iface = net_if_lookup_by_dev(dev);
	sent_orig_iface = net_pkt_orig_iface(pkt);
	sent_forwarding = net_pkt_forwarding(pkt);
	sent_ipv6_hop_limit = 0U;

	if (net_pkt_family(pkt) == NET_AF_INET6 &&
	    net_pkt_get_len(pkt) >= sizeof(struct net_ipv6_hdr)) {
		sent_ipv6_hop_limit = NET_IPV6_HDR(pkt)->hop_limit;

		if (expected_ipv6_dst != NULL &&
		    !net_ipv6_addr_cmp_raw(NET_IPV6_HDR(pkt)->dst,
					   expected_ipv6_dst->s6_addr)) {
			notify_sender = false;
		}
	} else if (expected_ipv6_dst != NULL) {
		notify_sender = false;
	}

	if (feed_data) {
		DBG("Received at iface %p and feeding it into iface %p\n",
		    net_pkt_iface(pkt), recipient);

		net_pkt_ref(pkt);
		if (net_recv_data(recipient, pkt) < 0) {
			TC_ERROR("Data receive failed.");
			net_pkt_unref(pkt);
			test_failed = true;
		}

		goto out;
	}

	DBG("pkt %p to be sent len %lu\n", pkt, net_pkt_get_len(pkt));

	if (data_failure) {
		test_failed = true;
	}

	msg_sending = 0;
out:
	if (notify_sender) {
		k_sem_give(&wait_data);
	}

	return 0;
}

static void drain_wait_data(void)
{
	while (k_sem_take(&wait_data, K_NO_WAIT) == 0) {
	}
}

static void reset_send_state(void)
{
	sent_pkt_seen = false;
	sent_forwarding = false;
	sent_iface = NULL;
	sent_orig_iface = NULL;
	sent_ipv6_hop_limit = 0U;
	expected_ipv6_dst = NULL;
}

struct net_route_test net_route_data;
struct net_route_test net_route_data_peer;

static struct dummy_api net_route_if_api = {
	.iface_api.init = net_route_iface_init,
	.send = tester_send,
};

static struct dummy_api net_route_if_api_peer = {
	.iface_api.init = net_route_iface_init,
	.send = tester_send_peer,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(net_route_test, "net_route_test", host,
			 net_route_dev_init, NULL,
			 &net_route_data, NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_route_if_api, _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE, 127);

NET_DEVICE_INIT_INSTANCE(net_route_test_peer, "net_route_test_peer", peer,
			 net_route_dev_init, NULL,
			 &net_route_data_peer, NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_route_if_api_peer, _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE, 127);

static void test_init(void)
{
	struct net_if_mcast_addr *maddr;
	struct net_if_addr *ifaddr;
	int i;

	my_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	peer_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)) + 1;

	DBG("Interfaces: [%d] my %p, [%d] peer %p\n",
	    net_if_get_by_iface(my_iface), my_iface,
	    net_if_get_by_iface(peer_iface), peer_iface);

	zassert_not_null(my_iface,
			 "Interface is NULL");

	zassert_not_null(peer_iface,
			 "Interface is NULL");

	ifaddr = net_if_ipv6_addr_add(my_iface, &my_addr,
				      NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr,
			 "Cannot add IPv6 address");

	/* For testing purposes we need to set the addresses preferred */
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(my_iface, &ll_addr,
				      NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr,
			 "Cannot add IPv6 address");

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(peer_iface, &my_addr_alt,
				      NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr,
			 "Cannot add alternate IPv6 address");

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_ipv6_addr_create(&in6addr_mcast, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);

	maddr = net_if_ipv6_maddr_add(my_iface, &in6addr_mcast);
	zassert_not_null(maddr,
			 "Cannot add multicast IPv6 address");

	/* The peer and dest interfaces are just simulated, they are not
	 * really used so we should not add IP addresses for them.
	 */

	/* Some test addresses are generated */
	for (i = 0; i < max_routes; i++) {
		memcpy(&dest_addresses[i], &generic_addr,
		       sizeof(struct net_in6_addr));

		dest_addresses[i].s6_addr[14] = i + 1;
		dest_addresses[i].s6_addr[15] = sys_rand8_get();
	}
}

static void test_net_ctx_ipv6_create(void)
{
	int ret;

	ret = net_context_get(NET_AF_INET6, NET_SOCK_DGRAM, NET_IPPROTO_UDP,
			      &udp_ctx);
	zassert_equal(ret, 0,
		      "Context create IPv6 UDP test failed");
}

static bool net_test_send_ns(struct net_if *iface,
			     struct net_in6_addr *addr)
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
				   struct net_in6_addr *addr)
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

static void test_populate_ipv6_nbr_cache(void)
{
	struct net_nbr *nbr;

	msg_sending = NET_ICMPV6_NS;
	feed_data = true;
	data_failure = false;

	recipient = my_iface;

	zassert_true(net_test_send_ns(peer_iface, &peer_addr));

	nbr = net_ipv6_nbr_add(net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)),
			       &peer_addr,
			       &net_route_data_peer.ll_addr,
			       false,
			       NET_IPV6_NBR_STATE_REACHABLE);
	zassert_not_null(nbr, "Cannot add peer to neighbor cache");

	zassert_true(net_test_send_ns(peer_iface, &peer_addr_alt));

	nbr = net_ipv6_nbr_add(net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)),
			       &peer_addr_alt,
			       &net_route_data_peer.ll_addr,
			       false,
			       NET_IPV6_NBR_STATE_REACHABLE);
	zassert_not_null(nbr, "Cannot add peer to neighbor cache");

	k_sem_take(&wait_data, WAIT_TIME);

	feed_data = false;

	zassert_false(data_failure, "data failure");

	data_failure = false;

	zassert_true(net_test_nbr_lookup_ok(my_iface, &peer_addr));
}

static void test_route_ipv6_add(void)
{
	route_entry = net_route_ipv6_add(my_iface,
					 &dest_addr, 128,
					 &peer_addr,
					 NET_IPV6_ND_INFINITE_LIFETIME,
					 NET_ROUTE_PREFERENCE_LOW);

	zassert_not_null(route_entry, "Route add failed");
}

static void test_route_ipv6_update(void)
{
	struct net_route_entry *update_entry;

	update_entry = net_route_ipv6_add(my_iface,
					  &dest_addr, 128,
					  &peer_addr,
					  NET_IPV6_ND_INFINITE_LIFETIME,
					  NET_ROUTE_PREFERENCE_LOW);
	zassert_equal_ptr(update_entry, route_entry,
			  "Route add again failed");
}

static void test_route_ipv6_del(void)
{
	int ret;

	ret = net_route_ipv6_del(route_entry);
	if (ret < 0) {
		zassert_true(0, "Route del failed");
	}
}

static void test_route_ipv6_del_again(void)
{
	int ret;

	ret = net_route_ipv6_del(route_entry);
	if (ret >= 0) {
		zassert_true(0, "Route del again failed");
	}
}

static void test_route_ipv6_get_nexthop(void)
{
	struct net_in6_addr *nexthop;

	nexthop = net_route_ipv6_get_nexthop(route_entry);

	zassert_not_null(nexthop, "Route get nexthop failed");

	zassert_true(net_ipv6_addr_cmp(nexthop, &peer_addr),
		     "Route nexthop does not match");
}

static void test_route_ipv6_lookup_ok(void)
{
	struct net_route_entry *entry;

	entry = net_route_ipv6_lookup(my_iface, &dest_addr);
	zassert_not_null(entry,
			 "Route lookup failed");
}

static void test_route_ipv6_lookup_fail(void)
{
	struct net_route_entry *entry;

	entry = net_route_ipv6_lookup(my_iface, &peer_addr);
	zassert_is_null(entry,
			"Route lookup failed for peer address");
}

static void test_route_ipv6_del_nexthop(void)
{
	struct net_in6_addr *nexthop = &peer_addr;
	int ret;

	ret = net_route_ipv6_del_by_nexthop(my_iface, nexthop);
	zassert_false((ret <= 0), "Route del nexthop failed");
}

static void test_route_ipv6_del_nexthop_again(void)
{
	struct net_in6_addr *nexthop = &peer_addr;
	int ret;

	ret = net_route_ipv6_del_by_nexthop(my_iface, nexthop);
	zassert_false((ret >= 0), "Route del again nexthop failed");
}

static void test_route_ipv6_add_many(void)
{
	int i;

	for (i = 0; i < max_routes; i++) {
		DBG("Adding route %d addr %s\n", i + 1,
		    net_sprint_ipv6_addr(&dest_addresses[i]));
		test_routes[i] = net_route_ipv6_add(my_iface,
						    &dest_addresses[i], 128,
						    &peer_addr,
						    NET_IPV6_ND_INFINITE_LIFETIME,
						    NET_ROUTE_PREFERENCE_LOW);
		zassert_not_null(test_routes[i], "Route add failed");
		}
}

static void test_route_ipv6_del_many(void)
{
	int i;

	for (i = 0; i < max_routes; i++) {
		DBG("Deleting route %d addr %s\n", i + 1,
		    net_sprint_ipv6_addr(&dest_addresses[i]));
		zassert_false(net_route_ipv6_del(test_routes[i]),
			      " Route del failed");
	}
}

static void test_route_ipv6_lifetime(void)
{
	route_entry = net_route_ipv6_add(my_iface,
					 &dest_addr, 128,
					 &peer_addr,
					 NET_IPV6_ND_INFINITE_LIFETIME,
					 NET_ROUTE_PREFERENCE_LOW);

	zassert_not_null(route_entry, "Route add failed");

	route_entry = net_route_ipv6_lookup(my_iface, &dest_addr);
	zassert_not_null(route_entry, "Route not found");

	net_route_ipv6_update_lifetime(route_entry, 1);

	k_sleep(K_MSEC(1200));

	route_entry = net_route_ipv6_lookup(my_iface, &dest_addr);
	zassert_is_null(route_entry, "Route did not expire");

}

static void test_route_ipv6_preference(void)
{
	struct net_route_entry *update_entry;

	route_entry = net_route_ipv6_add(my_iface,
					 &dest_addr, 128,
					 &peer_addr,
					 NET_IPV6_ND_INFINITE_LIFETIME,
					 NET_ROUTE_PREFERENCE_LOW);
	zassert_not_null(route_entry, "Route add failed");

	update_entry = net_route_ipv6_add(my_iface,
					  &dest_addr, 128,
					  &peer_addr_alt,
					  NET_IPV6_ND_INFINITE_LIFETIME,
					  NET_ROUTE_PREFERENCE_MEDIUM);
	zassert_equal_ptr(update_entry, route_entry,
			  "Route add again failed");

	update_entry = net_route_ipv6_add(my_iface,
					  &dest_addr, 128,
					  &peer_addr,
					  NET_IPV6_ND_INFINITE_LIFETIME,
					  NET_ROUTE_PREFERENCE_LOW);
	zassert_is_null(update_entry,
			"Low preference route overwritten medium one");

	net_route_ipv6_del(route_entry);
}

static void test_route_ipv6_select_src_iface_uses_explicit_route(void)
{
	const struct net_in6_addr *src_addr;
	struct net_if_router *router;
	struct net_if *iface;
	struct net_route_entry *route;

	router = net_if_ipv6_router_add(my_iface, &peer_addr, true,
					 0U);
	zassert_not_null(router, "Default router add failed");

	route = net_route_ipv6_add(peer_iface, &dest_addr_alt, 128,
				   &peer_addr_alt,
				   NET_IPV6_ND_INFINITE_LIFETIME,
				   NET_ROUTE_PREFERENCE_HIGH);
	zassert_not_null(route, "Selection route add failed");

	src_addr = net_if_ipv6_select_src_addr(NULL, &dest_addr_alt);
	zassert_false(src_addr == net_ipv6_unspecified_address(),
		      "Source address selection failed");
	zassert_true(net_ipv6_addr_cmp(src_addr, &my_addr_alt),
		     "Source address should come from the routed interface");

	iface = net_if_ipv6_select_src_iface(&dest_addr_alt);
	zassert_equal_ptr(iface, peer_iface,
			  "Explicit IPv6 route should select peer interface");

	zassert_ok(net_route_ipv6_del(route), "Selection route del failed");
	zassert_true(net_if_ipv6_router_rm(router), "Default router del failed");
}

static void test_route_ipv6_packet_without_neighbor_ll(void)
{
	struct net_ipv6_hdr *hdr;
	struct net_pkt *pkt;
	int ret;

	sent_pkt_seen = false;

	pkt = net_pkt_alloc_with_buffer(my_iface, sizeof(struct net_ipv6_hdr),
					NET_AF_INET6, NET_IPPROTO_ICMPV6,
					K_NO_WAIT);
	zassert_not_null(pkt, "Packet alloc failed");

	hdr = (struct net_ipv6_hdr *)net_buf_add(pkt->buffer,
						 sizeof(struct net_ipv6_hdr));
	zassert_not_null(hdr, "Cannot reserve IPv6 header");

	hdr->vtc = 0x60;
	net_ipv6_addr_copy_raw(hdr->src, my_addr.s6_addr);
	net_ipv6_addr_copy_raw(hdr->dst, dest_addr_alt.s6_addr);

	net_pkt_set_orig_iface(pkt, my_iface);
	net_pkt_set_iface(pkt, my_iface);

	ret = net_route_ipv6_packet(pkt, &peer_addr_alt);
	zassert_ok(ret, "IPv6 route packet failed without LL neighbor");
	zassert_true(sent_pkt_seen, "Packet was not sent");
}

static void test_route_ipv6_packet_without_iface(void)
{
	struct net_ipv6_hdr *hdr;
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(my_iface, sizeof(struct net_ipv6_hdr),
					NET_AF_INET6, NET_IPPROTO_ICMPV6,
					K_NO_WAIT);
	zassert_not_null(pkt, "Packet alloc failed");

	hdr = (struct net_ipv6_hdr *)net_buf_add(pkt->buffer,
						 sizeof(struct net_ipv6_hdr));
	zassert_not_null(hdr, "Cannot reserve IPv6 header");

	hdr->vtc = 0x60;
	net_ipv6_addr_copy_raw(hdr->src, my_addr.s6_addr);
	net_ipv6_addr_copy_raw(hdr->dst, dest_addr_alt.s6_addr);

	net_pkt_set_iface(pkt, NULL);

	zassert_equal(net_route_ipv6_packet(pkt, &peer_addr_alt), -EINVAL,
		      "IPv6 route packet should reject missing iface");

	net_pkt_unref(pkt);
}

static void test_route_ipv6_forward_hop_limit_expired_is_dropped(void)
{
	struct net_nbr *nbr;
	struct net_pkt *pkt;
	struct net_ipv6_hdr *hdr;
	int ret;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6_FORWARDING);

	nbr = net_ipv6_nbr_add(peer_iface, &forward_nexthop,
			       &net_route_data_peer.ll_addr, false,
			       NET_IPV6_NBR_STATE_REACHABLE);
	zassert_not_null(nbr, "Neighbor add failed");
	reset_send_state();
	drain_wait_data();
	drain_wait_data();

	pkt = net_pkt_alloc_with_buffer(my_iface, sizeof(struct net_ipv6_hdr),
					NET_AF_INET6, NET_IPV6_NEXTHDR_NONE,
					K_NO_WAIT);
	zassert_not_null(pkt, "Packet alloc failed");

	hdr = (struct net_ipv6_hdr *)net_buf_add(pkt->buffer,
						 sizeof(struct net_ipv6_hdr));
	zassert_not_null(hdr, "Cannot reserve IPv6 header");

	memset(hdr, 0, sizeof(*hdr));
	hdr->vtc = 0x60;
	hdr->len = 0U;
	hdr->nexthdr = NET_IPV6_NEXTHDR_NONE;
	hdr->hop_limit = 1U;
	net_ipv6_addr_copy_raw(hdr->src, forward_src_addr.s6_addr);
	net_ipv6_addr_copy_raw(hdr->dst, forward_dest_addr.s6_addr);

	net_pkt_set_orig_iface(pkt, my_iface);
	net_pkt_set_iface(pkt, my_iface);
	net_pkt_set_forwarding(pkt, false);

	ret = net_route_ipv6_packet(pkt, &forward_nexthop);
	zassert_equal(ret, -ETIMEDOUT, "Expected hop-limit expiry");
	zassert_false(sent_pkt_seen,
		      "Hop-limit-expired forwarded IPv6 packet must not be sent");
	zassert_equal(k_sem_take(&wait_data, WAIT_TIME), -EAGAIN,
		      "Hop-limit-expired forwarded IPv6 packet unexpectedly sent");
}

static void test_route_ipv6_forward_packet_between_ifaces(void)
{
	struct net_route_entry *route;
	struct net_nbr *nbr;
	struct net_pkt *pkt;
	struct net_ipv6_hdr *hdr;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6_FORWARDING);

	route = net_route_ipv6_add(peer_iface, &forward_dest_addr, 128,
				   &forward_nexthop,
				   NET_IPV6_ND_INFINITE_LIFETIME,
				   NET_ROUTE_PREFERENCE_HIGH);
	zassert_not_null(route, "Forwarding route add failed");

	nbr = net_ipv6_nbr_add(peer_iface, &forward_nexthop,
			       &net_route_data_peer.ll_addr, false,
			       NET_IPV6_NBR_STATE_REACHABLE);
	zassert_not_null(nbr, "Forwarding nexthop add failed");

	reset_send_state();
	drain_wait_data();
	expected_ipv6_dst = &forward_dest_addr;

	pkt = net_pkt_alloc_with_buffer(my_iface, sizeof(struct net_ipv6_hdr),
					NET_AF_INET6, NET_IPV6_NEXTHDR_NONE,
					K_NO_WAIT);
	zassert_not_null(pkt, "Forwarding packet alloc failed");

	hdr = (struct net_ipv6_hdr *)net_buf_add(pkt->buffer,
						 sizeof(struct net_ipv6_hdr));
	zassert_not_null(hdr, "Cannot reserve IPv6 header");

	memset(hdr, 0, sizeof(*hdr));
	hdr->vtc = 0x60;
	hdr->len = 0U;
	hdr->nexthdr = NET_IPV6_NEXTHDR_NONE;
	hdr->hop_limit = 2U;
	net_ipv6_addr_copy_raw(hdr->src, forward_src_addr.s6_addr);
	net_ipv6_addr_copy_raw(hdr->dst, forward_dest_addr.s6_addr);

	zassert_ok(net_recv_data(my_iface, pkt), "Forwarding receive failed");
	zassert_ok(k_sem_take(&wait_data, WAIT_TIME), "Forwarded packet was not sent");

	zassert_true(sent_pkt_seen, "Forwarded packet not observed");
	zassert_equal_ptr(sent_iface, peer_iface,
			  "Forwarded packet used wrong egress interface");
	zassert_equal(sent_ipv6_hop_limit, 1U,
		      "Forwarded IPv6 packet should decrement hop limit");

	expected_ipv6_dst = NULL;
	zassert_ok(net_route_ipv6_del(route), "Forwarding route del failed");
}

static void test_route_ipv6_forward_onlink_packet_between_ifaces(void)
{
	struct net_nbr *nbr;
	struct net_pkt *pkt;
	struct net_ipv6_hdr *hdr;
	int ret;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6_FORWARDING);

	nbr = net_ipv6_nbr_add(peer_iface, &onlink_dest_addr,
			       &net_route_data_peer.ll_addr, false,
			       NET_IPV6_NBR_STATE_REACHABLE);
	zassert_not_null(nbr, "On-link destination neighbor add failed");

	reset_send_state();
	drain_wait_data();
	expected_ipv6_dst = &onlink_dest_addr;

	pkt = net_pkt_alloc_with_buffer(my_iface, sizeof(struct net_ipv6_hdr),
					NET_AF_INET6, NET_IPV6_NEXTHDR_NONE,
					K_NO_WAIT);
	zassert_not_null(pkt, "On-link forwarding packet alloc failed");

	hdr = (struct net_ipv6_hdr *)net_buf_add(pkt->buffer,
						 sizeof(struct net_ipv6_hdr));
	zassert_not_null(hdr, "Cannot reserve IPv6 header");

	memset(hdr, 0, sizeof(*hdr));
	hdr->vtc = 0x60;
	hdr->len = 0U;
	hdr->nexthdr = NET_IPV6_NEXTHDR_NONE;
	hdr->hop_limit = 2U;
	net_ipv6_addr_copy_raw(hdr->src, forward_src_addr.s6_addr);
	net_ipv6_addr_copy_raw(hdr->dst, onlink_dest_addr.s6_addr);

	net_pkt_set_iface(pkt, my_iface);
	net_pkt_set_family(pkt, NET_AF_INET6);

	ret = net_route_packet_if(pkt, peer_iface);
	zassert_ok(ret, "On-link IPv6 route packet failed");
	zassert_ok(k_sem_take(&wait_data, WAIT_TIME), "On-link forwarded packet was not sent");

	zassert_true(sent_pkt_seen, "On-link forwarded packet not observed");
	zassert_equal_ptr(sent_iface, peer_iface,
			  "On-link forwarded packet used wrong egress interface");
	zassert_equal_ptr(sent_orig_iface, my_iface,
			  "On-link forwarded packet missing ingress interface");
	zassert_equal(sent_ipv6_hop_limit, 1U,
		      "On-link forwarded IPv6 packet should decrement hop limit");
	zassert_true(sent_forwarding, "On-link packet should be marked forwarded");

	expected_ipv6_dst = NULL;
	zassert_ok(net_nbr_unlink(nbr, &net_route_data_peer.ll_addr),
		   "On-link destination neighbor remove failed");
}

static void test_route_ipv6_forward_onlink_hop_limit_expired_is_dropped(void)
{
	struct net_pkt *pkt;
	struct net_ipv6_hdr *hdr;
	int ret;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6_FORWARDING);

	reset_send_state();
	drain_wait_data();

	pkt = net_pkt_alloc_with_buffer(my_iface, sizeof(struct net_ipv6_hdr),
					NET_AF_INET6, NET_IPV6_NEXTHDR_NONE,
					K_NO_WAIT);
	zassert_not_null(pkt, "On-link forwarding packet alloc failed");

	hdr = (struct net_ipv6_hdr *)net_buf_add(pkt->buffer,
						 sizeof(struct net_ipv6_hdr));
	zassert_not_null(hdr, "Cannot reserve IPv6 header");

	memset(hdr, 0, sizeof(*hdr));
	hdr->vtc = 0x60;
	hdr->len = 0U;
	hdr->nexthdr = NET_IPV6_NEXTHDR_NONE;
	hdr->hop_limit = 1U;
	net_ipv6_addr_copy_raw(hdr->src, forward_src_addr.s6_addr);
	net_ipv6_addr_copy_raw(hdr->dst, onlink_dest_addr.s6_addr);

	net_pkt_set_iface(pkt, my_iface);
	net_pkt_set_family(pkt, NET_AF_INET6);

	ret = net_route_packet_if(pkt, peer_iface);
	zassert_equal(ret, -ETIMEDOUT,
		      "Hop-limit-expired on-link packet must not be forwarded");
	zassert_not_equal(k_sem_take(&wait_data, WAIT_TIME), 0,
			  "Hop-limit-expired on-link packet unexpectedly sent");
	zassert_false(sent_pkt_seen,
		      "Hop-limit-expired on-link packet unexpectedly sent");

	net_pkt_unref(pkt);
}

/*test case main entry*/
ZTEST(route_test_suite, test_route)
{
	test_init();
	test_net_ctx_ipv6_create();
	test_populate_ipv6_nbr_cache();
	test_route_ipv6_add();
	test_route_ipv6_update();
	test_route_ipv6_get_nexthop();
	test_route_ipv6_lookup_ok();
	test_route_ipv6_lookup_fail();
	test_route_ipv6_del();
	test_route_ipv6_add();
	test_route_ipv6_del_nexthop();
	test_route_ipv6_del_again();
	test_route_ipv6_del_nexthop_again();
	test_populate_ipv6_nbr_cache();
	test_route_ipv6_add_many();
	test_route_ipv6_del_many();
	test_route_ipv6_lifetime();
	test_route_ipv6_preference();
	test_route_ipv6_select_src_iface_uses_explicit_route();
	test_route_ipv6_packet_without_neighbor_ll();
	test_route_ipv6_packet_without_iface();
	test_route_ipv6_forward_hop_limit_expired_is_dropped();
	test_route_ipv6_forward_packet_between_ifaces();
	test_route_ipv6_forward_onlink_packet_between_ifaces();
	test_route_ipv6_forward_onlink_hop_limit_expired_is_dropped();
}

ZTEST_SUITE(route_test_suite, NULL, NULL, NULL, NULL, NULL);
