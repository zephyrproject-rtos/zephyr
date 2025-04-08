/* main.c - IPv4 route tests */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_IPV4_ROUTE_LOG_LEVEL);

#include <errno.h>

#include <zephyr/ztest.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

#include "route_ipv4.h"
#include "arp.h"

static struct net_in_addr my_addr = { .s4_addr = { 192, 0, 2, 1 } };
static struct net_in_addr my_addr_alt = { .s4_addr = { 192, 0, 3, 1 } };
static struct net_in_addr iface_netmask = { .s4_addr = { 255, 255, 255, 0 } };
static struct net_in_addr peer_addr = { .s4_addr = { 198, 51, 100, 3 } };
static struct net_in_addr peer_addr_alt = { .s4_addr = { 198, 51, 100, 4 } };
static struct net_in_addr dest_addr = { .s4_addr = { 203, 0, 113, 7 } };
static struct net_in_addr dest_addr_iface0 = { .s4_addr = { 203, 0, 113, 10 } };
static struct net_in_addr dest_addr_iface1 = { .s4_addr = { 203, 0, 113, 11 } };
static struct net_in_addr dest_addr_override = { .s4_addr = { 192, 0, 3, 77 } };
static struct net_in_addr gateway_addr = { .s4_addr = { 192, 0, 2, 2 } };
static struct net_in_addr gateway_addr_alt = { .s4_addr = { 192, 0, 3, 2 } };
static struct net_in_addr subnet_addr = { .s4_addr = { 198, 51, 100, 99 } };
static struct net_in_addr subnet_prefix = { .s4_addr = { 198, 51, 100, 0 } };
static struct net_in_addr subnet_dest_addr = { .s4_addr = { 198, 51, 100, 42 } };
static struct net_eth_addr gateway_lladdr = { { 0x02, 0x00, 0x5e, 0x00, 0x53, 0x10 } };
static struct net_eth_addr gateway_lladdr_alt = { { 0x02, 0x00, 0x5e, 0x00, 0x53, 0x11 } };

static struct net_if *my_iface;
static struct net_if *my_iface_alt;
static struct net_route_entry *route_entry;
static struct net_in_addr sent_ll_resolve_addr;
static bool sent_ll_resolve_addr_set;
static bool sent_pkt_seen;
static uint16_t sent_ll_proto_type;
static bool sent_arp_request_seen;
static struct net_if *sent_iface;

#define MAX_ROUTES CONFIG_NET_IPV4_MAX_ROUTES
static struct net_route_entry *test_routes[MAX_ROUTES];
static struct net_in_addr dest_addresses[MAX_ROUTES];

struct net_route_test {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

static int net_route_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint8_t *net_route_get_mac(const struct device *dev)
{
	struct net_route_test *route = dev->data;

	if (route->mac_addr[2] == 0x00U) {
		route->mac_addr[0] = 0x00U;
		route->mac_addr[1] = 0x00U;
		route->mac_addr[2] = 0x5EU;
		route->mac_addr[3] = 0x00U;
		route->mac_addr[4] = 0x53U;
		route->mac_addr[5] = 0x01U;
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

	ethernet_init(iface);
}

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	const struct net_in_addr *resolve_addr;

	sent_pkt_seen = true;
	sent_iface = net_if_lookup_by_dev(dev);
	sent_ll_proto_type = net_pkt_ll_proto_type(pkt);
	sent_arp_request_seen = false;

	resolve_addr = net_pkt_ipv4_ll_resolve_addr(pkt);
	if (resolve_addr != NULL) {
		net_ipaddr_copy(&sent_ll_resolve_addr, resolve_addr);
		sent_ll_resolve_addr_set = true;
	} else {
		sent_ll_resolve_addr_set = false;
	}

	if (net_pkt_ll_proto_type(pkt) == NET_ETH_PTYPE_ARP &&
	    net_pkt_get_len(pkt) >= sizeof(struct net_eth_hdr)) {
		sent_arp_request_seen = true;
	}

	return 0;
}

static struct net_route_test net_route_data = {
	.mac_addr = { 0x00U, 0x00U, 0x5EU, 0x00U, 0x53U, 0x01U },
};
static struct net_route_test net_route_data_alt = {
	.mac_addr = { 0x00U, 0x00U, 0x5EU, 0x00U, 0x53U, 0x02U },
};

static struct ethernet_api net_route_if_api = {
	.iface_api.init = net_route_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER ETHERNET_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(ETHERNET_L2)

NET_DEVICE_INIT_INSTANCE(net_route_test, "net_route_test", host,
			 net_route_dev_init, NULL,
			 &net_route_data, NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_route_if_api, _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE, 127);

NET_DEVICE_INIT_INSTANCE(net_route_test_alt, "net_route_test_alt", host_alt,
			 net_route_dev_init, NULL,
			 &net_route_data_alt, NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_route_if_api, _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE, 127);

static void test_init(void)
{
	struct net_if_addr *ifaddr;

	my_iface = net_if_lookup_by_dev(DEVICE_GET(net_route_test));
	zassert_not_null(my_iface, "Interface is NULL");

	ifaddr = net_if_ipv4_addr_add(my_iface, &my_addr, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Cannot add IPv4 address");
	ifaddr->addr_state = NET_ADDR_PREFERRED;
	zassert_true(net_if_ipv4_set_netmask_by_addr(my_iface, &my_addr, &iface_netmask),
		     "Cannot set IPv4 netmask");

	my_iface_alt = net_if_lookup_by_dev(DEVICE_GET(net_route_test_alt));
	zassert_not_null(my_iface_alt, "Alt interface is NULL");

	ifaddr = net_if_ipv4_addr_add(my_iface_alt, &my_addr_alt, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Cannot add alt IPv4 address");
	ifaddr->addr_state = NET_ADDR_PREFERRED;
	zassert_true(net_if_ipv4_set_netmask_by_addr(my_iface_alt, &my_addr_alt,
						     &iface_netmask),
		     "Cannot set alt IPv4 netmask");

	for (int i = 0; i < MAX_ROUTES; i++) {
		dest_addresses[i].s4_addr[0] = 203U;
		dest_addresses[i].s4_addr[1] = 0U;
		dest_addresses[i].s4_addr[2] = 113U;
		dest_addresses[i].s4_addr[3] = (uint8_t)(i + 1);
	}
}

static void send_routed_ipv4_packet(struct net_in_addr *dst,
				    struct net_in_addr *src,
				    struct net_if *expected_iface,
				    struct net_in_addr *expected_nexthop,
				    struct net_eth_addr *expected_lladdr)
{
	struct net_route_entry *route;
	struct net_in_addr *nexthop;
	struct net_ipv4_hdr *hdr;
	struct net_pkt *pkt;

	zassert_true(net_route_ipv4_get_info(NULL, dst, &route, &nexthop),
		     "Route info lookup failed");
	zassert_not_null(route, "Missing route entry");
	zassert_equal_ptr(route->iface, expected_iface,
			  "Route selected wrong interface");
	zassert_not_null(nexthop, "Missing nexthop");
	zassert_true(net_ipv4_addr_cmp(nexthop, expected_nexthop),
		     "Route selected wrong nexthop");

	sent_pkt_seen = false;
	sent_ll_resolve_addr_set = false;
	sent_ll_proto_type = 0U;
	sent_arp_request_seen = false;
	sent_iface = NULL;

	pkt = net_pkt_alloc_with_buffer(my_iface, sizeof(struct net_ipv4_hdr),
					NET_AF_INET, 0, K_NO_WAIT);
	zassert_not_null(pkt, "Packet alloc failed");

	net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_IP);

	hdr = (struct net_ipv4_hdr *)net_buf_add(pkt->buffer,
						 sizeof(struct net_ipv4_hdr));
	zassert_not_null(hdr, "Cannot reserve IPv4 header");

	hdr->ttl = 2U;
	net_ipv4_addr_copy_raw(hdr->src, src->s4_addr);
	net_ipv4_addr_copy_raw(hdr->dst, dst->s4_addr);

	net_pkt_set_orig_iface(pkt, net_pkt_iface(pkt));
	net_pkt_set_iface(pkt, route->iface);
	(void)net_linkaddr_set(net_pkt_lladdr_src(pkt),
			       net_if_get_link_addr(route->iface)->addr,
			       sizeof(struct net_eth_addr));
	(void)net_linkaddr_set(net_pkt_lladdr_dst(pkt),
			       (const uint8_t *)expected_lladdr,
			       sizeof(struct net_eth_addr));

	zassert_ok(net_send_data(pkt), "Packet send failed");
	zassert_true(sent_pkt_seen, "Packet was not sent");
	zassert_equal_ptr(sent_iface, expected_iface,
			  "Packet was sent via wrong interface");
	zassert_equal(sent_ll_proto_type, NET_ETH_PTYPE_IP,
		      "Expected the routed IPv4 packet to be sent");
	zassert_false(sent_arp_request_seen,
		      "Unexpected ARP request while gateway was cached");
}

static void test_route_ipv4_add(void)
{
	route_entry = net_route_ipv4_add(my_iface, &dest_addr, 32,
					 &peer_addr, NET_ROUTE_INFINITE_LIFETIME,
					 NET_ROUTE_PREFERENCE_LOW);

	zassert_not_null(route_entry, "Route add failed");
}

static void test_route_ipv4_update(void)
{
	struct net_route_entry *update_entry;

	update_entry = net_route_ipv4_add(my_iface, &dest_addr, 32,
					  &peer_addr, NET_ROUTE_INFINITE_LIFETIME,
					  NET_ROUTE_PREFERENCE_LOW);
	zassert_equal_ptr(update_entry, route_entry, "Route update failed");
}

static void test_route_ipv4_del(void)
{
	zassert_ok(net_route_ipv4_del(route_entry), "Route del failed");
}

static void test_route_ipv4_del_again(void)
{
	zassert_true(net_route_ipv4_del(route_entry) < 0, "Route del again failed");
}

static void test_route_ipv4_get_nexthop(void)
{
	struct net_in_addr *nexthop;

	nexthop = net_route_ipv4_get_nexthop(route_entry);
	zassert_not_null(nexthop, "Route get nexthop failed");
	zassert_true(net_ipv4_addr_cmp(nexthop, &peer_addr),
		     "Route nexthop does not match");
}

static void test_route_ipv4_lookup_ok(void)
{
	zassert_not_null(net_route_ipv4_lookup(my_iface, &dest_addr),
			 "Route lookup failed");
}

static void test_route_ipv4_lookup_fail(void)
{
	zassert_is_null(net_route_ipv4_lookup(my_iface, &peer_addr),
			"Route lookup unexpectedly succeeded");
}

static void test_route_ipv4_del_nexthop(void)
{
	int ret = net_route_ipv4_del_by_nexthop(my_iface, &peer_addr);

	zassert_true(ret > 0, "Route del nexthop failed");
}

static void test_route_ipv4_del_nexthop_again(void)
{
	int ret = net_route_ipv4_del_by_nexthop(my_iface, &peer_addr);

	zassert_true(ret < 0, "Route del nexthop again failed");
}

static void test_route_ipv4_add_many(void)
{
	for (int i = 0; i < MAX_ROUTES; i++) {
		test_routes[i] = net_route_ipv4_add(my_iface, &dest_addresses[i], 32,
						    &peer_addr, NET_ROUTE_INFINITE_LIFETIME,
						    NET_ROUTE_PREFERENCE_LOW);
		zassert_not_null(test_routes[i], "Route add failed");
	}
}

static void test_route_ipv4_del_many(void)
{
	for (int i = 0; i < MAX_ROUTES; i++) {
		zassert_ok(net_route_ipv4_del(test_routes[i]), "Route del failed");
	}
}

static void test_route_ipv4_lifetime(void)
{
	route_entry = net_route_ipv4_add(my_iface, &dest_addr, 32,
					 &peer_addr, NET_ROUTE_INFINITE_LIFETIME,
					 NET_ROUTE_PREFERENCE_LOW);
	zassert_not_null(route_entry, "Route add failed");

	net_route_ipv4_update_lifetime(route_entry, 1U);
	k_sleep(K_MSEC(1200));

	zassert_is_null(net_route_ipv4_lookup(my_iface, &dest_addr),
			"Route did not expire");
}

static void test_route_ipv4_preference(void)
{
	struct net_route_entry *update_entry;
	struct net_in_addr *nexthop;

	route_entry = net_route_ipv4_add(my_iface, &dest_addr, 32,
					 &peer_addr, NET_ROUTE_INFINITE_LIFETIME,
					 NET_ROUTE_PREFERENCE_LOW);
	zassert_not_null(route_entry, "Route add failed");

	update_entry = net_route_ipv4_add(my_iface, &dest_addr, 32,
					  &peer_addr_alt, NET_ROUTE_INFINITE_LIFETIME,
					  NET_ROUTE_PREFERENCE_MEDIUM);
	zassert_not_null(update_entry, "Preference update failed");

	nexthop = net_route_ipv4_get_nexthop(update_entry);
	zassert_not_null(nexthop, "Missing nexthop after preference update");
	zassert_true(net_ipv4_addr_cmp(nexthop, &peer_addr_alt),
		     "Higher preference route did not replace nexthop");

	update_entry = net_route_ipv4_add(my_iface, &dest_addr, 32,
					  &peer_addr, NET_ROUTE_INFINITE_LIFETIME,
					  NET_ROUTE_PREFERENCE_LOW);
	zassert_is_null(update_entry, "Low preference route overwrote better route");

	nexthop = net_route_ipv4_get_nexthop(route_entry);
	zassert_not_null(nexthop, "Missing nexthop after low preference add");
	zassert_true(net_ipv4_addr_cmp(nexthop, &peer_addr_alt),
		     "Low preference route changed nexthop");

	zassert_ok(net_route_ipv4_del(route_entry), "Route del failed");
}

static void test_route_ipv4_onlink_subnet(void)
{
	struct net_route_entry *entry;
	struct net_route_entry *update_entry;
	struct net_route_entry *lookup;
	struct net_in_addr *nexthop;
	struct net_route_entry *info_route;
	struct net_in_addr *info_nexthop;

	entry = net_route_ipv4_add(my_iface, &subnet_addr, 24, NULL,
				   NET_ROUTE_INFINITE_LIFETIME,
				   NET_ROUTE_PREFERENCE_MEDIUM);
	zassert_not_null(entry, "On-link route add failed");

	update_entry = net_route_ipv4_add(my_iface, &subnet_addr, 24, NULL,
					  NET_ROUTE_INFINITE_LIFETIME,
					  NET_ROUTE_PREFERENCE_MEDIUM);
	zassert_equal_ptr(update_entry, entry,
			  "On-link route update should reuse the existing route");

	lookup = net_route_ipv4_lookup(my_iface, &subnet_dest_addr);
	zassert_equal_ptr(lookup, entry, "On-link subnet lookup failed");
	zassert_equal(entry->addr.in_addr.s_addr, subnet_prefix.s_addr,
		      "Stored route prefix was not normalized");

	nexthop = net_route_ipv4_get_nexthop(entry);
	zassert_is_null(nexthop, "On-link route should not have a nexthop");

	zassert_true(net_route_ipv4_get_info(my_iface, &subnet_dest_addr,
					     &info_route, &info_nexthop),
		     "On-link route info lookup failed");
	zassert_equal_ptr(info_route, entry, "Route info returned wrong route");
	zassert_not_null(info_nexthop, "On-link route should resolve via destination");
	zassert_true(net_ipv4_addr_cmp(info_nexthop, &subnet_dest_addr),
		     "On-link route should use destination address for ARP");

	zassert_equal(net_route_ipv4_del_by_nexthop(my_iface, &peer_addr), -ENOENT,
		      "Deleting by nexthop should ignore on-link route");
	zassert_ok(net_route_ipv4_del(entry), "On-link route del failed");
}

static void test_route_ipv4_packet_arp_handoff(void)
{
	struct net_ipv4_hdr *hdr;
	struct net_pkt *pkt;

	sent_pkt_seen = false;
	sent_ll_resolve_addr_set = false;
	sent_ll_proto_type = 0U;
	sent_arp_request_seen = false;

	pkt = net_pkt_alloc_with_buffer(my_iface, sizeof(struct net_ipv4_hdr),
					NET_AF_INET, 0, K_NO_WAIT);
	zassert_not_null(pkt, "Packet alloc failed");

	net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_IP);

	hdr = (struct net_ipv4_hdr *)net_buf_add(pkt->buffer,
						 sizeof(struct net_ipv4_hdr));
	zassert_not_null(hdr, "Cannot reserve IPv4 header");

	hdr->ttl = 1U;
	net_ipv4_addr_copy_raw(hdr->src, my_addr.s4_addr);
	net_ipv4_addr_copy_raw(hdr->dst, dest_addr.s4_addr);

	zassert_ok(net_route_ipv4_packet(pkt, &gateway_addr), "Route packet failed");
	zassert_true(sent_pkt_seen, "Packet was not sent");
	zassert_equal(sent_ll_proto_type, NET_ETH_PTYPE_ARP,
		      "Expected an ARP request to be sent");
	zassert_false(sent_ll_resolve_addr_set,
		      "ARP request packet should not carry routed resolve metadata");
	zassert_true(sent_arp_request_seen, "ARP request was not captured");
	zassert_ok(net_arp_clear_pending(my_iface, &gateway_addr),
		   "Original routed packet was not queued pending ARP");
}

static void test_route_ipv4_packet_multi_iface(void)
{
	struct net_route_entry *route_iface0;
	struct net_route_entry *route_iface1;

	route_iface0 = net_route_ipv4_add(my_iface, &dest_addr_iface0, 32,
					  &gateway_addr, NET_ROUTE_INFINITE_LIFETIME,
					  NET_ROUTE_PREFERENCE_MEDIUM);
	zassert_not_null(route_iface0, "Interface 0 route add failed");

	route_iface1 = net_route_ipv4_add(my_iface_alt, &dest_addr_iface1, 32,
					  &gateway_addr_alt, NET_ROUTE_INFINITE_LIFETIME,
					  NET_ROUTE_PREFERENCE_MEDIUM);
	zassert_not_null(route_iface1, "Interface 1 route add failed");

	send_routed_ipv4_packet(&dest_addr_iface0, &my_addr, my_iface, &gateway_addr,
				    &gateway_lladdr);
	send_routed_ipv4_packet(&dest_addr_iface1, &my_addr, my_iface_alt,
				    &gateway_addr_alt, &gateway_lladdr_alt);

	zassert_ok(net_route_ipv4_del(route_iface0), "Interface 0 route del failed");
	zassert_ok(net_route_ipv4_del(route_iface1), "Interface 1 route del failed");
}

static void test_route_ipv4_host_route_overrides_normal_iface(void)
{
	struct net_route_entry *route_override;
	struct net_if *normal_iface = my_iface_alt;

	zassert_true(net_if_ipv4_addr_onlink(&normal_iface, &dest_addr_override),
		     "Override destination should be on-link");
	zassert_equal_ptr(normal_iface, my_iface_alt,
			  "Override destination should normally use alt iface");

	route_override = net_route_ipv4_add(my_iface, &dest_addr_override, 32,
					    &gateway_addr, NET_ROUTE_INFINITE_LIFETIME,
					    NET_ROUTE_PREFERENCE_HIGH);
	zassert_not_null(route_override, "Override route add failed");

	send_routed_ipv4_packet(&dest_addr_override, &my_addr, my_iface,
				    &gateway_addr, &gateway_lladdr);

	zassert_ok(net_route_ipv4_del(route_override), "Override route del failed");
}

ZTEST(route_test_suite, test_route)
{
	test_init();
	test_route_ipv4_add();
	test_route_ipv4_update();
	test_route_ipv4_get_nexthop();
	test_route_ipv4_lookup_ok();
	test_route_ipv4_lookup_fail();
	test_route_ipv4_del();
	test_route_ipv4_add();
	test_route_ipv4_del_nexthop();
	test_route_ipv4_del_again();
	test_route_ipv4_del_nexthop_again();
	test_route_ipv4_add_many();
	test_route_ipv4_del_many();
	test_route_ipv4_lifetime();
	test_route_ipv4_preference();
	test_route_ipv4_onlink_subnet();
	test_route_ipv4_packet_arp_handoff();
	test_route_ipv4_packet_multi_iface();
	test_route_ipv4_host_route_overrides_normal_iface();
}

ZTEST_SUITE(route_test_suite, NULL, NULL, NULL, NULL, NULL);
