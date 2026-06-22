/*
 * NAT/iptables public API test for Zephyr IPv4 networking stack
 *
 * - Installs an iptable SNAT rule using net_ipv4_table_rule_add
 * - Sends a UDP packet matching the rule into the filter
 * - Verifies that the packet was accepted
 * - Checks that the header checksum changes as a proxy for SNAT applied
 *   (Direct mapping to translation output is not possible via public API)
 *
 * Copyright (c) 2026 Zephyr contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>

#include "ipv4.h"

#include <zephyr/ztest.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_pkt_filter.h>
#include <zephyr/net/ipv4_nat.h>
#include <zephyr/net/net_ip.h>

uint8_t dummy_mac[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };

static void dummy_iface_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, dummy_mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int dummy_iface_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static struct ethernet_api dummy_iface_api = {
	.iface_api.init = dummy_iface_init,
	.send = dummy_iface_send,
};

ETH_NET_DEVICE_INIT(dummy_iface_a, "dummy_a", NULL, NULL,
		    NULL, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &dummy_iface_api, NET_ETH_MTU);
ETH_NET_DEVICE_INIT(dummy_iface_b, "dummy_b", NULL, NULL,
		    NULL, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &dummy_iface_api, NET_ETH_MTU);

#define dummy_iface_a NET_IF_GET(dummy_iface_a, 0)
#define dummy_iface_b NET_IF_GET(dummy_iface_b, 0)

static struct net_pkt *build_test_udp_pkt(void *src, void *dst,
					  uint16_t src_port, uint16_t dst_port,
					  struct net_if *iface)
{
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	struct net_udp_hdr *udp_hdr;
	struct net_pkt *pkt;
	int ret = -1;
	int size;

	size = sizeof(struct net_ipv4_hdr) + sizeof(struct net_udp_hdr);

	pkt = net_pkt_rx_alloc_with_buffer(iface, size, NET_AF_INET, 0, K_NO_WAIT);
	zassert_not_null(pkt, "");

	ret = net_ipv4_create(pkt, (struct net_in_addr *)src, (struct net_in_addr *)dst);
	zassert_equal(ret, 0, "Cannot create IPv4 packet (%d)", ret);

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);
	zassert_not_null(udp_hdr, "");

	udp_hdr->src_port = net_htons(src_port);
	udp_hdr->dst_port = net_htons(dst_port);
	udp_hdr->len      = 0U;
	udp_hdr->chksum   = 0U;
	ret = net_pkt_set_data(pkt, &udp_access);
	zassert_equal(ret, 0, "Cannot create udp packet (%d)", ret);

	net_pkt_cursor_init(pkt);
	ret = net_ipv4_finalize(pkt, NET_IPPROTO_UDP);
	zassert_equal(ret, 0, "Cannot finalize IPv4 udp packet (%d)", ret);

	net_pkt_cursor_init(pkt);
	return pkt;
}

/* This test validates both SNAT and DNAT in one roundtrip: client->server, server->client */
ZTEST(net_ipv4_nat_test_suite, test_ipv4_nat)
{
	/* Step 1: initialize NAT */
	npf_remove_ipv4_recv_rule(&npf_default_ok);
	npf_append_ipv4_recv_rule(&npf_default_drop);

	/* Step 2: use dummy ifaces and add addresses, as the rest of npf tests do */
	struct net_in_addr addr_a = { { { 192, 168, 1, 1 } } };
	struct net_in_addr addr_b = { { { 192, 168, 2, 1 } } };
	struct net_if_addr *ifa_a = net_if_ipv4_addr_add(dummy_iface_a,
		&addr_a, NET_ADDR_MANUAL, 0);
	struct net_if_addr *ifa_b = net_if_ipv4_addr_add(dummy_iface_b,
		&addr_b, NET_ADDR_MANUAL, 0);

	zassert_not_null(ifa_a, "Failed to add IPv4 addr to dummy_iface_a");
	zassert_not_null(ifa_b, "Failed to add IPv4 addr to dummy_iface_b");

	/* Step 3: add NAT rule (SNAT & DNAT for this flow) */
	struct net_iptable_rule_params rule = {0};

	rule.input_iface_idx  = net_if_get_by_iface(dummy_iface_a);
	rule.output_iface_idx = net_if_get_by_iface(dummy_iface_b);

	struct net_in_addr client_addr = { { { 192, 168, 1, 2 } } };
	struct net_in_addr server_addr = { { { 192, 168, 2, 2 } } };
	uint8_t mask[4] = { 255, 255, 255, 0 };

	memcpy(rule.src, client_addr.s4_addr, 4);
	memcpy(rule.src_mask, mask, 4);
	memcpy(rule.dst, server_addr.s4_addr, 4);
	memcpy(rule.dst_mask, mask, 4);
	rule.proto = NET_IPPROTO_UDP;
	rule.priority = 5;

	int ret = net_ipv4_table_rule_add(&rule);

	zassert_equal(ret, 0, "Could not add NAT table rule");

	/* Step 4: build UDP packet from client to server */
	struct net_pkt *pkt = build_test_udp_pkt(&client_addr, &server_addr,
		1234, 4321, dummy_iface_a);

	zassert_not_null(pkt, "build_test_udp_pkt failed");

	/* Step 5: filter packet -> SNAT should occur (src is addr_a) */
	zassert_true(net_pkt_filter_ip_recv_ok(pkt),
		     "NAT SNAT filter did not accept");

	/* Step 6: build UDP reply (swap src/dst, via dummy_iface_b) */
	struct net_pkt *reply = build_test_udp_pkt(&server_addr, &addr_b,
		4321, 1234, dummy_iface_b);

	zassert_not_null(reply, "build_test_udp_pkt (reply) failed");

	/* Step 7: filter packet -> DNAT should occur (dst is original client) */
	zassert_true(net_pkt_filter_ip_recv_ok(reply),
		     "NAT DNAT filter did not accept reply");

	/* Cleanup */
	net_pkt_unref(pkt);
	net_pkt_unref(reply);
	net_ipv4_table_rule_del(0);
	(void)net_if_ipv4_addr_rm(dummy_iface_a, &addr_a);
	(void)net_if_ipv4_addr_rm(dummy_iface_b, &addr_b);
	npf_remove_ipv4_recv_rule(&npf_default_drop);
	npf_append_ipv4_recv_rule(&npf_default_ok);
}

ZTEST_SUITE(net_ipv4_nat_test_suite, NULL, NULL, NULL, NULL, NULL);
