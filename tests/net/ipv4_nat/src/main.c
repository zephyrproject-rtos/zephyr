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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_IPV4_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>

#include "ipv4.h"
#include "arp.h"
#include "ipv4_nat_internal.h"
#include "icmpv4.h"
#include "net_private.h"

#include <zephyr/ztest.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_pkt_filter.h>
#include <zephyr/net/ipv4_nat.h>
#include <zephyr/net/net_ip.h>

extern struct net_ip4_table iptables;

uint8_t dummy_mac[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };

static void dummy_iface_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, dummy_mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static struct net_if *sent_iface;
static uint16_t sent_ll_proto_type;
static uint8_t sent_ipv4_ttl;
static struct net_in_addr sent_ipv4_src;
static struct net_in_addr sent_ipv4_dst;

K_SEM_DEFINE(sent_sem, 0, UINT_MAX);

#define WAIT_TIME K_MSEC(250)

static int dummy_iface_send(const struct device *dev, struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	struct net_pkt_cursor backup;
	struct net_ipv4_hdr *hdr;

	sent_iface = net_if_lookup_by_dev(dev);
	sent_ll_proto_type = net_pkt_ll_proto_type(pkt);
	sent_ipv4_ttl = 0U;

	if (sent_ll_proto_type == NET_ETH_PTYPE_IP) {
		net_pkt_cursor_backup(pkt, &backup);
		net_pkt_cursor_init(pkt);

		if (net_pkt_skip(pkt, sizeof(struct net_eth_hdr)) == 0) {
			hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &ipv4_access);
			if (hdr != NULL) {
				sent_ipv4_ttl = hdr->ttl;
				net_ipv4_addr_copy_raw(sent_ipv4_src.s4_addr, hdr->src);
				net_ipv4_addr_copy_raw(sent_ipv4_dst.s4_addr, hdr->dst);
			}
		}

		net_pkt_cursor_restore(pkt, &backup);
	}

	k_sem_give(&sent_sem);

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

static uint16_t ipv4_header_checksum(const struct net_ipv4_hdr *hdr)
{
	const uint8_t *data = (const uint8_t *)hdr;
	uint32_t sum = 0U;

	for (size_t i = 0; i < sizeof(*hdr); i += 2U) {
		sum += ((uint16_t)data[i] << 8) | data[i + 1];
	}

	while ((sum >> 16U) != 0U) {
		sum = (sum & 0xFFFFU) + (sum >> 16U);
	}

	return net_htons((uint16_t)(~sum & 0xFFFFU));
}

/* Inject a UDP frame as if received on iface. */
static void recv_udp_frame(struct net_if *iface,
			   const struct net_in_addr *src, const struct net_in_addr *dst,
			   uint16_t src_port, uint16_t dst_port, uint8_t ttl)
{
	struct net_eth_addr src_lladdr = { { 0x02, 0x00, 0x5e, 0x00, 0x53, 0x99 } };
	struct net_ipv4_hdr *ip_hdr;
	struct net_udp_hdr *udp_hdr;
	struct net_eth_hdr *eth;
	struct net_pkt *pkt;

	while (k_sem_take(&sent_sem, K_NO_WAIT) == 0) {
	}

	pkt = net_pkt_rx_alloc_with_buffer(iface,
					   sizeof(struct net_eth_hdr) +
					   sizeof(struct net_ipv4_hdr) +
					   sizeof(struct net_udp_hdr),
					   NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_not_null(pkt, "Frame alloc failed");

	eth = (struct net_eth_hdr *)net_buf_add(pkt->buffer, sizeof(struct net_eth_hdr));
	memcpy(eth->dst.addr, net_if_get_link_addr(iface)->addr, sizeof(eth->dst.addr));
	memcpy(eth->src.addr, src_lladdr.addr, sizeof(eth->src.addr));
	eth->type = net_htons(NET_ETH_PTYPE_IP);

	ip_hdr = (struct net_ipv4_hdr *)net_buf_add(pkt->buffer, sizeof(struct net_ipv4_hdr));
	memset(ip_hdr, 0, sizeof(*ip_hdr));
	ip_hdr->vhl = 0x45;
	ip_hdr->ttl = ttl;
	ip_hdr->proto = NET_IPPROTO_UDP;
	ip_hdr->len = net_htons(sizeof(struct net_ipv4_hdr) + sizeof(struct net_udp_hdr));
	net_ipv4_addr_copy_raw(ip_hdr->src, src->s4_addr);
	net_ipv4_addr_copy_raw(ip_hdr->dst, dst->s4_addr);
	ip_hdr->chksum = ipv4_header_checksum(ip_hdr);

	udp_hdr = (struct net_udp_hdr *)net_buf_add(pkt->buffer, sizeof(struct net_udp_hdr));
	udp_hdr->src_port = net_htons(src_port);
	udp_hdr->dst_port = net_htons(dst_port);
	udp_hdr->len = net_htons(sizeof(struct net_udp_hdr));
	udp_hdr->chksum = 0U;

	zassert_ok(net_recv_data(iface, pkt), "Frame receive failed");
}

/* Wait for an IPv4 frame to leave, skipping ARP. */
static bool forwarded_frame_sent(void)
{
	for (int i = 0; i < 5; i++) {
		if (k_sem_take(&sent_sem, WAIT_TIME) != 0) {
			return false;
		}

		if (sent_ll_proto_type == NET_ETH_PTYPE_IP) {
			return true;
		}
	}

	return false;
}

/* Client on dummy_iface_a reaches a server on dummy_iface_b through
 * net_ipv4_input(): the request is SNATed and forwarded out b, the
 * reply is DNATed and forwarded out a, each with the TTL decremented
 * once.
 */
ZTEST(net_ipv4_nat_test_suite, test_ipv4_nat_forward_through_input)
{
	struct net_in_addr addr_a = { { { 192, 168, 1, 1 } } };
	struct net_in_addr addr_b = { { { 192, 168, 2, 1 } } };
	struct net_in_addr netmask = { { { 255, 255, 255, 0 } } };
	struct net_in_addr client_addr = { { { 192, 168, 1, 4 } } };
	struct net_in_addr server_addr = { { { 192, 168, 2, 4 } } };
	struct net_eth_addr client_lladdr = { { 0x02, 0x00, 0x5e, 0x00, 0x53, 0x04 } };
	struct net_eth_addr server_lladdr = { { 0x02, 0x00, 0x5e, 0x00, 0x53, 0x05 } };
	struct net_iptable_rule_params rule = {0};
	uint8_t mask[4] = { 255, 255, 255, 0 };
	struct net_if_addr *ifa_a;
	struct net_if_addr *ifa_b;
	int rule_idx;

	npf_remove_ipv4_recv_rule(&npf_default_ok);
	npf_append_ipv4_recv_rule(&npf_default_drop);

	ifa_a = net_if_ipv4_addr_add(dummy_iface_a, &addr_a, NET_ADDR_MANUAL, 0);
	ifa_b = net_if_ipv4_addr_add(dummy_iface_b, &addr_b, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifa_a, "Failed to add IPv4 addr to dummy_iface_a");
	zassert_not_null(ifa_b, "Failed to add IPv4 addr to dummy_iface_b");
	zassert_true(net_if_ipv4_set_netmask_by_addr(dummy_iface_a, &addr_a, &netmask),
		     "Cannot set netmask on dummy_iface_a");
	zassert_true(net_if_ipv4_set_netmask_by_addr(dummy_iface_b, &addr_b, &netmask),
		     "Cannot set netmask on dummy_iface_b");

	net_arp_update(dummy_iface_a, &client_addr, &client_lladdr, true);
	net_arp_update(dummy_iface_b, &server_addr, &server_lladdr, true);

	rule.input_iface_idx = net_if_get_by_iface(dummy_iface_a);
	rule.output_iface_idx = net_if_get_by_iface(dummy_iface_b);
	memcpy(rule.src, client_addr.s4_addr, 4);
	memcpy(rule.src_mask, mask, 4);
	memcpy(rule.dst, server_addr.s4_addr, 4);
	memcpy(rule.dst_mask, mask, 4);
	rule.proto = NET_IPPROTO_UDP;
	rule.priority = 5;

	rule_idx = net_ipv4_table_rule_add(&rule);
	zassert_true(rule_idx >= 0, "Could not add NAT table rule");

	recv_udp_frame(dummy_iface_a, &client_addr, &server_addr, 1234, 4321, 64U);
	zassert_true(forwarded_frame_sent(), "Translated request was not forwarded");
	zassert_equal_ptr(sent_iface, dummy_iface_b,
			  "Request left through the wrong interface");
	zassert_true(net_ipv4_addr_cmp(&sent_ipv4_src, &addr_b),
		     "Request source was not translated");
	zassert_true(net_ipv4_addr_cmp(&sent_ipv4_dst, &server_addr),
		     "Request destination was changed");
	zassert_equal(sent_ipv4_ttl, 63U, "Request TTL should be decremented once");

	recv_udp_frame(dummy_iface_b, &server_addr, &addr_b, 4321, 1234, 64U);
	zassert_true(forwarded_frame_sent(), "Translated reply was not forwarded");
	zassert_equal_ptr(sent_iface, dummy_iface_a,
			  "Reply left through the wrong interface");
	zassert_true(net_ipv4_addr_cmp(&sent_ipv4_src, &server_addr),
		     "Reply source was changed");
	zassert_true(net_ipv4_addr_cmp(&sent_ipv4_dst, &client_addr),
		     "Reply destination was not translated");
	zassert_equal(sent_ipv4_ttl, 63U, "Reply TTL should be decremented once");

	net_ipv4_table_rule_del(rule_idx);
	net_arp_clear_cache(dummy_iface_a);
	net_arp_clear_cache(dummy_iface_b);
	(void)net_if_ipv4_addr_rm(dummy_iface_a, &addr_a);
	(void)net_if_ipv4_addr_rm(dummy_iface_b, &addr_b);
	npf_remove_ipv4_recv_rule(&npf_default_drop);
	npf_append_ipv4_recv_rule(&npf_default_ok);
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

	int rule_idx = net_ipv4_table_rule_add(&rule);

	zassert_true(rule_idx >= 0, "Could not add NAT table rule");

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
	net_ipv4_table_rule_del(rule_idx);
	(void)net_if_ipv4_addr_rm(dummy_iface_a, &addr_a);
	(void)net_if_ipv4_addr_rm(dummy_iface_b, &addr_b);
	npf_remove_ipv4_recv_rule(&npf_default_drop);
	npf_append_ipv4_recv_rule(&npf_default_ok);
}

ZTEST(net_ipv4_nat_test_suite, test_ipv4_nat_rule_index)
{
	struct net_iptable_rule_params rule = {0};
	int idx1, idx2;

	rule.input_iface_idx = net_if_get_by_iface(dummy_iface_a);
	rule.output_iface_idx = net_if_get_by_iface(dummy_iface_b);
	rule.proto = NET_IPPROTO_UDP;

	idx1 = net_ipv4_table_rule_add(&rule);
	zassert_true(idx1 >= 0, "Could not add first NAT table rule");

	idx2 = net_ipv4_table_rule_add(&rule);
	zassert_true(idx2 >= 0, "Could not add second NAT table rule");
	zassert_not_equal(idx1, idx2, "Rules share the same index");

	/* Deleting by the returned index must free that exact slot */
	net_ipv4_table_rule_del(idx1);
	zassert_equal(net_ipv4_table_rule_add(&rule), idx1, "Freed rule index was not reused");

	net_ipv4_table_rule_del(idx1);
	net_ipv4_table_rule_del(idx2);
}

/* A rule matching any destination must not translate traffic to the NAT host itself */
ZTEST(net_ipv4_nat_test_suite, test_ipv4_nat_skip_local_dst)
{
	npf_remove_ipv4_recv_rule(&npf_default_ok);
	npf_append_ipv4_recv_rule(&npf_default_drop);

	struct net_in_addr addr_a = { { { 192, 168, 1, 1 } } };
	struct net_in_addr addr_b = { { { 192, 168, 2, 1 } } };
	struct net_if_addr *ifa_a = net_if_ipv4_addr_add(dummy_iface_a,
		&addr_a, NET_ADDR_MANUAL, 0);
	struct net_if_addr *ifa_b = net_if_ipv4_addr_add(dummy_iface_b,
		&addr_b, NET_ADDR_MANUAL, 0);

	zassert_not_null(ifa_a, "Failed to add IPv4 addr to dummy_iface_a");
	zassert_not_null(ifa_b, "Failed to add IPv4 addr to dummy_iface_b");

	struct net_iptable_rule_params rule = {0};
	struct net_in_addr client_addr = { { { 192, 168, 1, 3 } } };
	struct net_in_addr server_addr = { { { 192, 168, 2, 3 } } };
	uint8_t mask[4] = { 255, 255, 255, 0 };

	rule.input_iface_idx  = net_if_get_by_iface(dummy_iface_a);
	rule.output_iface_idx = net_if_get_by_iface(dummy_iface_b);
	memcpy(rule.src, client_addr.s4_addr, 4);
	memcpy(rule.src_mask, mask, 4);
	rule.proto = NET_IPPROTO_UDP;
	rule.priority = 5;

	int rule_idx = net_ipv4_table_rule_add(&rule);

	zassert_true(rule_idx >= 0, "Could not add NAT table rule");

	/* Local addresses on the input and on the output interface */
	struct net_in_addr *local_dsts[] = { &addr_a, &addr_b };

	ARRAY_FOR_EACH(local_dsts, i) {
		struct net_pkt *pkt = build_test_udp_pkt(&client_addr, local_dsts[i],
			5678, 67, dummy_iface_a);

		zassert_false(net_pkt_filter_ip_recv_ok(pkt),
			      "NAT matched a packet to a local address");
		zassert_true(net_ipv4_addr_cmp_raw(NET_IPV4_HDR(pkt)->src,
						   client_addr.s4_addr),
			     "Source of a packet to a local address was translated");
		net_pkt_unref(pkt);
	}

	/* The same rule still translates forwarded traffic */
	struct net_pkt *pkt = build_test_udp_pkt(&client_addr, &server_addr,
		5678, 8765, dummy_iface_a);

	zassert_true(net_pkt_filter_ip_recv_ok(pkt), "NAT did not match forwarded packet");
	zassert_true(net_ipv4_addr_cmp_raw(NET_IPV4_HDR(pkt)->src, addr_b.s4_addr),
		     "Source of forwarded packet was not translated");
	net_pkt_unref(pkt);

	net_ipv4_table_rule_del(rule_idx);
	(void)net_if_ipv4_addr_rm(dummy_iface_a, &addr_a);
	(void)net_if_ipv4_addr_rm(dummy_iface_b, &addr_b);
	npf_remove_ipv4_recv_rule(&npf_default_drop);
	npf_append_ipv4_recv_rule(&npf_default_ok);
}

/* Run a UDP packet through the NAT filter and return its source and
 * destination after translation, ports in host byte order.
 */
static void nat_udp(struct net_if *iface,
		    struct net_in_addr *src, uint16_t src_port,
		    struct net_in_addr *dst, uint16_t dst_port,
		    struct net_in_addr *out_src, uint16_t *out_src_port,
		    struct net_in_addr *out_dst, uint16_t *out_dst_port)
{
	struct net_pkt *pkt = build_test_udp_pkt(src, dst, src_port, dst_port, iface);
	struct net_ipv4_hdr *ip_hdr;
	struct net_udp_hdr *udp_hdr;

	zassert_true(net_pkt_filter_ip_recv_ok(pkt), "NAT did not match the packet");

	ip_hdr = NET_IPV4_HDR(pkt);
	udp_hdr = (struct net_udp_hdr *)((uint8_t *)ip_hdr + sizeof(struct net_ipv4_hdr));

	net_ipv4_addr_copy_raw(out_src->s4_addr, ip_hdr->src);
	net_ipv4_addr_copy_raw(out_dst->s4_addr, ip_hdr->dst);
	*out_src_port = net_ntohs(udp_hdr->src_port);
	*out_dst_port = net_ntohs(udp_hdr->dst_port);

	net_pkt_unref(pkt);
}

/* Two clients using the same source port towards the same server must get
 * distinct external ports, and replies must reach the right client.
 */
ZTEST(net_ipv4_nat_test_suite, test_ipv4_nat_same_source_port)
{
	struct net_in_addr addr_a = { { { 192, 168, 1, 1 } } };
	struct net_in_addr addr_b = { { { 192, 168, 2, 1 } } };
	struct net_in_addr client1 = { { { 192, 168, 1, 6 } } };
	struct net_in_addr client2 = { { { 192, 168, 1, 7 } } };
	struct net_in_addr server1 = { { { 192, 168, 2, 6 } } };
	struct net_in_addr server2 = { { { 192, 168, 2, 7 } } };
	struct net_iptable_rule_params rule = {0};
	uint8_t mask[4] = { 255, 255, 255, 0 };
	struct net_in_addr src;
	struct net_in_addr dst;
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t ext_port;
	uint16_t ext_port_low;
	int rule_idx;

	npf_remove_ipv4_recv_rule(&npf_default_ok);
	npf_append_ipv4_recv_rule(&npf_default_drop);

	zassert_not_null(net_if_ipv4_addr_add(dummy_iface_a, &addr_a, NET_ADDR_MANUAL, 0),
			 "Failed to add IPv4 addr to dummy_iface_a");
	zassert_not_null(net_if_ipv4_addr_add(dummy_iface_b, &addr_b, NET_ADDR_MANUAL, 0),
			 "Failed to add IPv4 addr to dummy_iface_b");

	rule.input_iface_idx = net_if_get_by_iface(dummy_iface_a);
	rule.output_iface_idx = net_if_get_by_iface(dummy_iface_b);
	memcpy(rule.src, client1.s4_addr, 4);
	memcpy(rule.src_mask, mask, 4);
	rule.proto = NET_IPPROTO_UDP;
	rule.priority = 5;

	rule_idx = net_ipv4_table_rule_add(&rule);
	zassert_true(rule_idx >= 0, "Could not add NAT table rule");

	/* The first mapping keeps the client's port */
	nat_udp(dummy_iface_a, &client1, 2000, &server1, 53, &src, &src_port, &dst, &dst_port);
	zexpect_true(net_ipv4_addr_cmp(&src, &addr_b), "Source was not translated");
	zexpect_equal(src_port, 2000U, "Free source port was not preserved");

	/* The second one must not share it */
	nat_udp(dummy_iface_a, &client2, 2000, &server1, 53, &src, &ext_port, &dst, &dst_port);
	zexpect_true(net_ipv4_addr_cmp(&src, &addr_b), "Source was not translated");
	zexpect_not_equal(ext_port, 2000U, "Two clients share one external port");
	zexpect_true(ext_port >= 1024U, "External port left the 1024-65535 range");
	zexpect_equal(ext_port % 2U, 0U, "External port parity was not preserved");

	nat_udp(dummy_iface_b, &server1, 53, &addr_b, 2000, &src, &src_port, &dst, &dst_port);
	zexpect_true(net_ipv4_addr_cmp(&dst, &client1), "Reply did not reach the first client");
	zexpect_equal(dst_port, 2000U, "Reply port to the first client was wrong");

	nat_udp(dummy_iface_b, &server1, 53, &addr_b, ext_port, &src, &src_port, &dst, &dst_port);
	zexpect_true(net_ipv4_addr_cmp(&dst, &client2), "Reply did not reach the second client");
	zexpect_equal(dst_port, 2000U, "Reply port to the second client was not restored");

	/* The mapping is endpoint independent: a new server sees the same port */
	nat_udp(dummy_iface_a, &client2, 2000, &server2, 53, &src, &src_port, &dst, &dst_port);
	zexpect_equal(src_port, ext_port, "Second server got a different external port");

	/* Well-known source ports stay in the well-known range */
	nat_udp(dummy_iface_a, &client1, 701, &server1, 53, &src, &src_port, &dst, &dst_port);
	zexpect_equal(src_port, 701U, "Free source port was not preserved");
	nat_udp(dummy_iface_a, &client2, 701, &server1, 53, &src, &ext_port_low, &dst, &dst_port);
	zexpect_not_equal(ext_port_low, 701U, "Two clients share one external port");
	zexpect_true(ext_port_low > 0U && ext_port_low < 1024U,
		     "External port left the 1-1023 range");
	zexpect_equal(ext_port_low % 2U, 1U, "External port parity was not preserved");

	net_ipv4_table_rule_del(rule_idx);
	(void)net_if_ipv4_addr_rm(dummy_iface_a, &addr_a);
	(void)net_if_ipv4_addr_rm(dummy_iface_b, &addr_b);
	npf_remove_ipv4_recv_rule(&npf_default_drop);
	npf_append_ipv4_recv_rule(&npf_default_ok);
}

static struct net_pkt *build_test_icmp_pkt(struct net_in_addr *src, struct net_in_addr *dst,
					   uint8_t type, uint16_t id, struct net_if *iface)
{
	struct net_pkt *pkt;

	pkt = net_pkt_rx_alloc_with_buffer(iface, sizeof(struct net_ipv4_hdr) +
					   NET_ICMPV4H_LEN + NET_ICMPV4_UNUSED_LEN,
					   NET_AF_INET, 0, K_NO_WAIT);
	zassert_not_null(pkt, "ICMP packet alloc failed");

	zassert_ok(net_ipv4_create(pkt, src, dst), "Cannot create IPv4 packet");
	zassert_ok(net_icmpv4_create(pkt, type, 0), "Cannot create ICMP header");
	zassert_ok(net_pkt_write_be16(pkt, id), "Cannot write identifier");
	zassert_ok(net_pkt_write_be16(pkt, 1U), "Cannot write sequence");
	net_pkt_cursor_init(pkt);
	zassert_ok(net_ipv4_finalize(pkt, NET_IPPROTO_ICMP), "Cannot finalize ICMP packet");
	net_pkt_cursor_init(pkt);

	return pkt;
}

/* Run an ICMP echo packet through the NAT filter and return its addresses,
 * identifier in host byte order and whether its ICMP checksum is valid.
 */
static bool nat_icmp(struct net_if *iface, struct net_in_addr *src, struct net_in_addr *dst,
		     uint8_t type, uint16_t id, struct net_in_addr *out_src,
		     struct net_in_addr *out_dst, uint16_t *out_id, bool *chksum_ok)
{
	struct net_pkt *pkt = build_test_icmp_pkt(src, dst, type, id, iface);
	struct net_ipv4_hdr *ip_hdr;
	uint16_t chksum;
	bool matched;

	matched = net_pkt_filter_ip_recv_ok(pkt);

	ip_hdr = NET_IPV4_HDR(pkt);
	net_ipv4_addr_copy_raw(out_src->s4_addr, ip_hdr->src);
	net_ipv4_addr_copy_raw(out_dst->s4_addr, ip_hdr->dst);
	*out_id = net_ntohs(UNALIGNED_GET((uint16_t *)((uint8_t *)ip_hdr +
						       sizeof(struct net_ipv4_hdr) +
						       NET_ICMPV4H_LEN)));
	*chksum_ok = net_calc_chksum_icmpv4(pkt, &chksum) == 0 && chksum == 0U;

	net_pkt_unref(pkt);

	return matched;
}

/* Two clients pinging the same server with the same echo identifier must
 * get distinct external identifiers, and replies must reach the right client.
 */
ZTEST(net_ipv4_nat_test_suite, test_ipv4_nat_same_icmp_identifier)
{
	struct net_in_addr addr_a = { { { 192, 168, 1, 1 } } };
	struct net_in_addr addr_b = { { { 192, 168, 2, 1 } } };
	struct net_in_addr client1 = { { { 192, 168, 1, 6 } } };
	struct net_in_addr client2 = { { { 192, 168, 1, 7 } } };
	struct net_in_addr server1 = { { { 192, 168, 2, 6 } } };
	struct net_in_addr server2 = { { { 192, 168, 2, 7 } } };
	struct net_iptable_rule_params rule = {0};
	uint8_t mask[4] = { 255, 255, 255, 0 };
	struct net_in_addr src;
	struct net_in_addr dst;
	uint16_t id;
	uint16_t ext_id;
	bool chksum_ok;
	int rule_idx;

	npf_remove_ipv4_recv_rule(&npf_default_ok);
	npf_append_ipv4_recv_rule(&npf_default_drop);

	zassert_not_null(net_if_ipv4_addr_add(dummy_iface_a, &addr_a, NET_ADDR_MANUAL, 0),
			 "Failed to add IPv4 addr to dummy_iface_a");
	zassert_not_null(net_if_ipv4_addr_add(dummy_iface_b, &addr_b, NET_ADDR_MANUAL, 0),
			 "Failed to add IPv4 addr to dummy_iface_b");

	rule.input_iface_idx = net_if_get_by_iface(dummy_iface_a);
	rule.output_iface_idx = net_if_get_by_iface(dummy_iface_b);
	memcpy(rule.src, client1.s4_addr, 4);
	memcpy(rule.src_mask, mask, 4);
	rule.proto = NET_IPPROTO_ICMP;
	rule.priority = 5;

	rule_idx = net_ipv4_table_rule_add(&rule);
	zassert_true(rule_idx >= 0, "Could not add NAT table rule");

	/* The first mapping keeps the client's identifier */
	zexpect_true(nat_icmp(dummy_iface_a, &client1, &server1, NET_ICMPV4_ECHO_REQUEST,
			      0x1234U, &src, &dst, &id, &chksum_ok),
		     "NAT did not match the first request");
	zexpect_true(net_ipv4_addr_cmp(&src, &addr_b), "Source was not translated");
	zexpect_equal(id, 0x1234U, "Free identifier was not preserved");
	zexpect_true(chksum_ok, "ICMP checksum broken by the translation");

	/* The second one must not share it */
	zexpect_true(nat_icmp(dummy_iface_a, &client2, &server1, NET_ICMPV4_ECHO_REQUEST,
			      0x1234U, &src, &dst, &ext_id, &chksum_ok),
		     "NAT did not match the second request");
	zexpect_true(net_ipv4_addr_cmp(&src, &addr_b), "Source was not translated");
	zexpect_not_equal(ext_id, 0x1234U, "Two clients share one external identifier");
	zexpect_true(chksum_ok, "ICMP checksum not updated for the new identifier");

	zexpect_true(nat_icmp(dummy_iface_b, &server1, &addr_b, NET_ICMPV4_ECHO_REPLY,
			      0x1234U, &src, &dst, &id, &chksum_ok),
		     "NAT did not match the first reply");
	zexpect_true(net_ipv4_addr_cmp(&dst, &client1), "Reply did not reach the first client");
	zexpect_equal(id, 0x1234U, "Reply identifier to the first client was wrong");
	zexpect_true(chksum_ok, "ICMP checksum broken by the translation");

	zexpect_true(nat_icmp(dummy_iface_b, &server1, &addr_b, NET_ICMPV4_ECHO_REPLY,
			      ext_id, &src, &dst, &id, &chksum_ok),
		     "NAT did not match the second reply");
	zexpect_true(net_ipv4_addr_cmp(&dst, &client2), "Reply did not reach the second client");
	zexpect_equal(id, 0x1234U, "Reply identifier to the second client was not restored");
	zexpect_true(chksum_ok, "ICMP checksum not updated for the restored identifier");

	/* The mapping is endpoint independent: a new server sees the same identifier */
	zexpect_true(nat_icmp(dummy_iface_a, &client2, &server2, NET_ICMPV4_ECHO_REQUEST,
			      0x1234U, &src, &dst, &id, &chksum_ok),
		     "NAT did not match the request to the second server");
	zexpect_equal(id, ext_id, "Second server got a different external identifier");

	/* A second identifier from the same client gets its own mapping and
	 * leaves the first one in place.
	 */
	zexpect_true(nat_icmp(dummy_iface_a, &client1, &server1, NET_ICMPV4_ECHO_REQUEST,
			      0x5678U, &src, &dst, &id, &chksum_ok),
		     "NAT did not match the request with a second identifier");
	zexpect_equal(id, 0x5678U, "Free identifier was not preserved");
	zexpect_true(nat_icmp(dummy_iface_b, &server1, &addr_b, NET_ICMPV4_ECHO_REPLY,
			      0x1234U, &src, &dst, &id, &chksum_ok),
		     "Reply to the first identifier no longer matched");
	zexpect_true(net_ipv4_addr_cmp(&dst, &client1), "Reply did not reach the first client");

	net_ipv4_table_rule_del(rule_idx);
	(void)net_if_ipv4_addr_rm(dummy_iface_a, &addr_a);
	(void)net_if_ipv4_addr_rm(dummy_iface_b, &addr_b);
	npf_remove_ipv4_recv_rule(&npf_default_drop);
	npf_append_ipv4_recv_rule(&npf_default_ok);
}

/* Start every test without the mappings of the previous ones */
static void nat_conn_track_clear(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(iptables.table, 0, sizeof(iptables.table));
}

ZTEST_SUITE(net_ipv4_nat_test_suite, NULL, NULL, nat_conn_track_clear, NULL, NULL);
