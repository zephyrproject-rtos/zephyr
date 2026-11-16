/*
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_PKT_FILTER_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(npf_test, NET_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>

#include "ipv4.h"
#include "ipv6.h"

#include <zephyr/ztest.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_pkt_filter.h>
#include <zephyr/sys/byteorder.h>

#include "net_private.h"

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#define ETH_SRC_ADDR \
	(struct net_eth_addr){ { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 } }
#define ETH_DST_ADDR \
	(struct net_eth_addr){ { 0x00, 0x66, 0x77, 0x88, 0x99, 0xaa } }

static const char dummy_data[] =
"The Zephyr Project is a scalable real-time operating system (RTOS) supporting\n"
"multiple hardware architectures, optimized for resource constrained devices,\n"
"and built with security in mind.\n"
"\n"
"The Zephyr OS is based on a small-footprint kernel designed for use on\n"
"resource-constrained systems: from simple embedded environmental sensors and\n"
"LED wearables to sophisticated smart watches and IoT wireless gateways.\n"
"\n"
"The Zephyr kernel supports multiple architectures, including ARM Cortex-M,\n"
"Intel x86, ARC, Tensilica Xtensa, and RISC-V, and a large number of\n"
"`supported boards`_.\n";


static struct net_pkt *build_test_pkt(int type, int size, struct net_if *iface)
{
	struct net_pkt *pkt;
	struct net_eth_hdr eth_hdr;
	int ret;

	pkt = net_pkt_rx_alloc_with_buffer(iface, size, NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_not_null(pkt, "");

	eth_hdr.src = ETH_SRC_ADDR;
	eth_hdr.dst = ETH_DST_ADDR;
	eth_hdr.type = net_htons(type);

	ret = net_pkt_write(pkt, &eth_hdr, sizeof(eth_hdr));
	zassert_equal(ret, 0, "");

	zassert_true(size >= sizeof(eth_hdr), "");
	zassert_true((size - sizeof(eth_hdr)) <= sizeof(dummy_data), "");
	ret = net_pkt_write(pkt, dummy_data, size - sizeof(eth_hdr));
	zassert_equal(ret, 0, "");

	DBG("pkt %p: iface %p size %d type 0x%04x\n", pkt, iface, size, type);
	return pkt;
}

static struct net_pkt *build_vlan_test_pkt(int type, int size, struct net_if *iface)
{
	struct net_pkt *pkt;
	struct net_eth_vlan_hdr eth_hdr;
	int ret;

	pkt = net_pkt_rx_alloc_with_buffer(iface, size, NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_not_null(pkt, "");

	eth_hdr.src = ETH_SRC_ADDR;
	eth_hdr.dst = ETH_DST_ADDR;
	eth_hdr.type = net_htons(type);
	eth_hdr.vlan.tpid = net_htons(NET_ETH_PTYPE_VLAN);

	ret = net_pkt_write(pkt, &eth_hdr, sizeof(eth_hdr));
	zassert_equal(ret, 0, "");

	zassert_true(size >= sizeof(eth_hdr), "");
	zassert_true((size - sizeof(eth_hdr)) <= sizeof(dummy_data), "");
	ret = net_pkt_write(pkt, dummy_data, size - sizeof(eth_hdr));
	zassert_equal(ret, 0, "");

	DBG("pkt %p: iface %p size %d type 0x%04x\n", pkt, iface, size, type);
	return pkt;
}

static struct net_pkt *build_test_ip_pkt(void *src, void *dst,
					 net_sa_family_t family, struct net_if *iface)
{
	struct net_pkt *pkt;
	int ret = -1;
	int size;

	size = (family == NET_AF_INET) ? sizeof(struct net_ipv4_hdr) :
		(family == NET_AF_INET6) ? sizeof(struct net_ipv6_hdr) : 0U;

	pkt = net_pkt_rx_alloc_with_buffer(iface, size, family, 0, K_NO_WAIT);
	zassert_not_null(pkt, "");

	if (family == NET_AF_INET) {
		ret = net_ipv4_create(pkt, (struct net_in_addr *)src, (struct net_in_addr *)dst);
	} else if (family == NET_AF_INET6) {
		ret = net_ipv6_create(pkt, (struct net_in6_addr *)src, (struct net_in6_addr *)dst);
	}
	zassert_equal(ret, 0, "Cannot create %s packet (%d)",
		(family == NET_AF_INET) ? "IPv4" : "IPv6", ret);

	DBG("pkt %p: iface %p size %d sa_family %d\n", pkt, iface, size, family);
	return pkt;
}

/*
 * Declare some fake interfaces and test their filter conditions.
 */

static void eth_fake_iface_init(struct net_if *iface)
{
	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	uint8_t mac_addr[6] = {0x00, 0x00, 0x5E, 0x00, 0x53, sys_rand8_get()};

	net_if_set_link_addr(iface, mac_addr, sizeof(mac_addr), NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_fake_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static DEVICE_API(ethernet, eth_fake_api_funcs) = {
	.iface_api.init = eth_fake_iface_init,
	.send = eth_fake_send,
};

ETH_NET_DEVICE_INIT(dummy_iface_a, "dummy_a", NULL, NULL,
		    NULL, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &eth_fake_api_funcs, NET_ETH_MTU);
ETH_NET_DEVICE_INIT(dummy_iface_b, "dummy_b", NULL, NULL,
		    NULL, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &eth_fake_api_funcs, NET_ETH_MTU);
#define dummy_iface_a NET_IF_GET(dummy_iface_a, 0)
#define dummy_iface_b NET_IF_GET(dummy_iface_b, 0)

static NPF_IFACE_MATCH(match_iface_a, dummy_iface_a);
static NPF_IFACE_UNMATCH(unmatch_iface_b, dummy_iface_b);

static NPF_RULE(accept_iface_a, NET_OK, match_iface_a);
static NPF_RULE(accept_all_but_iface_b, NET_OK, unmatch_iface_b);

static NPF_ORIG_IFACE_MATCH(match_orig_iface_a, dummy_iface_a);
static NPF_ORIG_IFACE_UNMATCH(unmatch_orig_iface_b, dummy_iface_b);

static NPF_RULE(accept_orig_iface_a, NET_OK, match_orig_iface_a);
static NPF_RULE(accept_all_but_orig_iface_b, NET_OK, unmatch_orig_iface_b);

static void *test_npf_iface(void)
{
	struct net_pkt *pkt_iface_a, *pkt_iface_b;

	pkt_iface_a = build_test_pkt(0, 200, dummy_iface_a);
	pkt_iface_b = build_test_pkt(0, 200, dummy_iface_b);

	/* test with no rules */
	zassert_true(net_pkt_filter_recv_ok(pkt_iface_a), "");
	zassert_true(net_pkt_filter_recv_ok(pkt_iface_b), "");

	/* install rules */
	npf_append_recv_rule(&accept_iface_a);
	npf_append_recv_rule(&npf_default_drop);

	/* test with rules in place */
	zassert_true(net_pkt_filter_recv_ok(pkt_iface_a), "");
	zassert_false(net_pkt_filter_recv_ok(pkt_iface_b), "");

	/* remove first iface rule */
	zassert_true(npf_remove_recv_rule(&accept_iface_a), "");

	/* fails if removed a second time */
	zassert_false(npf_remove_recv_rule(&accept_iface_a), "");

	/* test with only default drop rule in place */
	zassert_false(net_pkt_filter_recv_ok(pkt_iface_a), "");
	zassert_false(net_pkt_filter_recv_ok(pkt_iface_b), "");

	/* insert second iface rule */
	npf_insert_recv_rule(&accept_all_but_iface_b);

	/* test with new rule in place */
	zassert_true(net_pkt_filter_recv_ok(pkt_iface_a), "");
	zassert_false(net_pkt_filter_recv_ok(pkt_iface_b), "");

	/* remove all rules */
	zassert_true(npf_remove_recv_rule(&accept_all_but_iface_b), "");
	zassert_true(npf_remove_recv_rule(&npf_default_drop), "");

	/* should accept any packets again */
	zassert_true(net_pkt_filter_recv_ok(pkt_iface_a), "");
	zassert_true(net_pkt_filter_recv_ok(pkt_iface_b), "");

	net_pkt_unref(pkt_iface_a);
	net_pkt_unref(pkt_iface_b);

	return NULL;
}

ZTEST(net_pkt_filter_test_suite, test_npf_orig_iface)
{
	struct net_pkt *pkt_iface_a, *pkt_iface_b;

	pkt_iface_a = build_test_pkt(0, 200, dummy_iface_a);
	pkt_iface_b = build_test_pkt(0, 200, dummy_iface_b);

	/* Set orig_iface to different values to test orig_iface matching */
	net_pkt_set_orig_iface(pkt_iface_a, dummy_iface_a);
	net_pkt_set_orig_iface(pkt_iface_b, dummy_iface_b);

	/* test with no rules */
	zassert_true(net_pkt_filter_recv_ok(pkt_iface_a), "");
	zassert_true(net_pkt_filter_recv_ok(pkt_iface_b), "");

	/* install rules */
	npf_append_recv_rule(&accept_orig_iface_a);
	npf_append_recv_rule(&npf_default_drop);

	/* test with rules in place */
	zassert_true(net_pkt_filter_recv_ok(pkt_iface_a), "");
	zassert_false(net_pkt_filter_recv_ok(pkt_iface_b), "");

	/* remove first orig_iface rule */
	zassert_true(npf_remove_recv_rule(&accept_orig_iface_a), "");

	/* fails if removed a second time */
	zassert_false(npf_remove_recv_rule(&accept_orig_iface_a), "");

	/* test with only default drop rule in place */
	zassert_false(net_pkt_filter_recv_ok(pkt_iface_a), "");
	zassert_false(net_pkt_filter_recv_ok(pkt_iface_b), "");

	/* insert second orig_iface rule */
	npf_insert_recv_rule(&accept_all_but_orig_iface_b);

	/* test with new rule in place */
	zassert_true(net_pkt_filter_recv_ok(pkt_iface_a), "");
	zassert_false(net_pkt_filter_recv_ok(pkt_iface_b), "");

	/* remove all rules */
	zassert_true(npf_remove_recv_rule(&accept_all_but_orig_iface_b), "");
	zassert_true(npf_remove_recv_rule(&npf_default_drop), "");

	/* should accept any packets again */
	zassert_true(net_pkt_filter_recv_ok(pkt_iface_a), "");
	zassert_true(net_pkt_filter_recv_ok(pkt_iface_b), "");

	net_pkt_unref(pkt_iface_a);
	net_pkt_unref(pkt_iface_b);
}

/*
 * Example 1 in NPF_RULE() documentation.
 */

static NPF_SIZE_MAX(maxsize_200, 200);
static NPF_ETH_TYPE_MATCH(ip_packet, NET_ETH_PTYPE_IP);

static NPF_RULE(small_ip_pkt, NET_OK, ip_packet, maxsize_200);

static void test_npf_example_common(void)
{
	struct net_pkt *pkt;

	/* test small IP packet */
	pkt = build_test_pkt(NET_ETH_PTYPE_IP, 100, NULL);
	zassert_true(net_pkt_filter_recv_ok(pkt), "");
	net_pkt_unref(pkt);

	/* test "big" IP packet */
	pkt = build_test_pkt(NET_ETH_PTYPE_IP, 300, NULL);
	zassert_false(net_pkt_filter_recv_ok(pkt), "");
	net_pkt_unref(pkt);

	/* test "small" non-IP packet */
	pkt = build_test_pkt(NET_ETH_PTYPE_ARP, 100, NULL);
	zassert_false(net_pkt_filter_recv_ok(pkt), "");
	net_pkt_unref(pkt);

	/* test "big" non-IP packet */
	pkt = build_test_pkt(NET_ETH_PTYPE_ARP, 300, NULL);
	zassert_false(net_pkt_filter_recv_ok(pkt), "");
	net_pkt_unref(pkt);
}

ZTEST(net_pkt_filter_test_suite, test_npf_example1)
{
	/* install filter rules */
	npf_insert_recv_rule(&npf_default_drop);
	npf_insert_recv_rule(&small_ip_pkt);

	test_npf_example_common();

	/* remove filter rules */
	zassert_true(npf_remove_recv_rule(&npf_default_drop), "");
	zassert_true(npf_remove_recv_rule(&small_ip_pkt), "");
}

/*
 * Example 2 in NPF_RULE() documentation.
 */

static NPF_SIZE_MIN(minsize_201, 201);
static NPF_ETH_TYPE_UNMATCH(not_ip_packet, NET_ETH_PTYPE_IP);

static NPF_RULE(reject_big_pkts, NET_DROP, minsize_201);
static NPF_RULE(reject_non_ip, NET_DROP, not_ip_packet);

ZTEST(net_pkt_filter_test_suite, test_npf_example2)
{
	/* install filter rules */
	npf_append_recv_rule(&reject_big_pkts);
	npf_append_recv_rule(&reject_non_ip);
	npf_append_recv_rule(&npf_default_ok);

	test_npf_example_common();

	/* remove filter rules */
	zassert_true(npf_remove_all_recv_rules(), "");
	zassert_false(npf_remove_all_recv_rules(), "");
}

/*
 * Ethernet MAC address filtering
 */

static struct net_eth_addr mac_address_list[4] = {
	{ { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 } },
	{ { 0x22, 0x22, 0x22, 0x22, 0x22, 0x22 } },
	{ { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33 } },
	{ { 0x44, 0x44, 0x44, 0x44, 0x44, 0x44 } },
};

static NPF_ETH_SRC_ADDR_MATCH(matched_src_addr, mac_address_list);
static NPF_ETH_DST_ADDR_MATCH(matched_dst_addr, mac_address_list);
static NPF_ETH_SRC_ADDR_UNMATCH(unmatched_src_addr, mac_address_list);
static NPF_ETH_DST_ADDR_UNMATCH(unmatched_dst_addr, mac_address_list);

static NPF_RULE(accept_matched_src_addr, NET_OK, matched_src_addr);
static NPF_RULE(accept_unmatched_src_addr, NET_OK, unmatched_src_addr);
static NPF_RULE(accept_matched_dst_addr, NET_OK, matched_dst_addr);
static NPF_RULE(accept_unmatched_dst_addr, NET_OK, unmatched_dst_addr);

static void test_npf_eth_mac_address(void)
{
	struct net_pkt *pkt = build_test_pkt(NET_ETH_PTYPE_IP, 100, NULL);

	/* make sure pkt is initially accepted */
	zassert_true(net_pkt_filter_recv_ok(pkt), "");

	/* let's test "OK" cases by making "drop" the default */
	npf_append_recv_rule(&npf_default_drop);

	/* validate missing src address */
	npf_insert_recv_rule(&accept_unmatched_src_addr);
	npf_insert_recv_rule(&accept_matched_src_addr);
	zassert_true(net_pkt_filter_recv_ok(pkt), "");
	zassert_true(npf_remove_recv_rule(&accept_unmatched_src_addr), "");
	zassert_false(net_pkt_filter_recv_ok(pkt), "");

	/* insert known src address in the lot */
	mac_address_list[1] = ETH_SRC_ADDR;
	zassert_true(net_pkt_filter_recv_ok(pkt), "");
	npf_insert_recv_rule(&accept_unmatched_src_addr);
	zassert_true(net_pkt_filter_recv_ok(pkt), "");
	zassert_true(npf_remove_recv_rule(&accept_matched_src_addr), "");
	zassert_false(net_pkt_filter_recv_ok(pkt), "");
	zassert_true(npf_remove_recv_rule(&accept_unmatched_src_addr), "");

	/* validate missing dst address */
	npf_insert_recv_rule(&accept_unmatched_dst_addr);
	npf_insert_recv_rule(&accept_matched_dst_addr);
	zassert_true(net_pkt_filter_recv_ok(pkt), "");
	zassert_true(npf_remove_recv_rule(&accept_unmatched_dst_addr), "");
	zassert_false(net_pkt_filter_recv_ok(pkt), "");

	/* insert known dst address in the lot */
	mac_address_list[2] = ETH_DST_ADDR;
	zassert_true(net_pkt_filter_recv_ok(pkt), "");
	npf_insert_recv_rule(&accept_unmatched_dst_addr);
	zassert_true(net_pkt_filter_recv_ok(pkt), "");
	zassert_true(npf_remove_recv_rule(&accept_matched_dst_addr), "");
	zassert_false(net_pkt_filter_recv_ok(pkt), "");
	zassert_true(npf_remove_recv_rule(&accept_unmatched_dst_addr), "");

	net_pkt_unref(pkt);
}

static NPF_ETH_SRC_ADDR_MASK_MATCH(matched_src_addr_mask, mac_address_list,
				   0xff, 0xff, 0xff, 0xff, 0xff, 0x00);
static NPF_RULE(accept_matched_src_addr_mask, NET_OK, matched_src_addr_mask);

static void test_npf_eth_mac_addr_mask(void)
{
	struct net_pkt *pkt = build_test_pkt(NET_ETH_PTYPE_IP, 100, NULL);

	/* test standard match rule from previous test */
	npf_insert_recv_rule(&npf_default_drop);
	npf_insert_recv_rule(&accept_matched_src_addr);
	zassert_true(net_pkt_filter_recv_ok(pkt), "");

	/* clobber one nibble of matching address from previous test */
	mac_address_list[1].addr[5] = 0x00;
	zassert_false(net_pkt_filter_recv_ok(pkt), "");

	/* insert masked address match rule */
	npf_insert_recv_rule(&accept_matched_src_addr_mask);
	zassert_true(net_pkt_filter_recv_ok(pkt), "");

	/* cleanup */
	zassert_true(npf_remove_all_recv_rules(), "");

	net_pkt_unref(pkt);
}

ZTEST(net_pkt_filter_test_suite, test_npf_address_mask)
{
	test_npf_eth_mac_address();
	test_npf_eth_mac_addr_mask();
}

/*
 * IP address filtering
 */

static struct net_in_addr ipv4_address_list[4] = {
	{ { { 192, 168, 1, 1 } } },
	{ { { 192, 0, 2, 1 } } },
	{ { { 172, 16, 0, 1 } } },
	{ { { 10, 49, 0, 252 } } }
};

static struct net_in6_addr ipv6_address_list[4] = {
	{ { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } },
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } },
	{ { { 0x20, 0x01, 0x0d, 0xb8, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } },
	{ { { 0x20, 0x01, 0x0d, 0xb8, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } },
};

static NPF_IP_SRC_ADDR_ALLOWLIST(allowlist_ipv4_src_addr, (void *)ipv4_address_list,
					 ARRAY_SIZE(ipv4_address_list), NET_AF_INET);
static NPF_IP_SRC_ADDR_BLOCKLIST(blocklist_ipv4_src_addr, (void *)ipv4_address_list,
					 ARRAY_SIZE(ipv4_address_list), NET_AF_INET);

static NPF_RULE(ipv4_allowlist, NET_OK, allowlist_ipv4_src_addr);
static NPF_RULE(ipv4_blocklist, NET_OK, blocklist_ipv4_src_addr);

ZTEST(net_pkt_filter_test_suite, test_npf_ipv4_address_filtering)
{
	struct net_in_addr dst = { { { 192, 168, 2, 1 } } };
	struct net_in_addr bad_addr = { { { 192, 168, 2, 3 } } };
	struct net_pkt *pkt_v4 = build_test_ip_pkt(&ipv4_address_list[0], &dst, NET_AF_INET,
						   dummy_iface_a);
	struct net_pkt *pkt_v6 = build_test_ip_pkt(&ipv6_address_list[0], &ipv6_address_list[1],
						   NET_AF_INET6, dummy_iface_a);

	/* make sure pkt is initially accepted */
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v4), "");
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v6), "");

	/* validate allowlist */
	npf_insert_ipv4_recv_rule(&ipv4_allowlist);

	for (int it = 0; it < ARRAY_SIZE(ipv4_address_list); it++) {
		memcpy((struct net_in_addr *)NET_IPV4_HDR(pkt_v4)->src, &ipv4_address_list[it],
		       sizeof(struct net_in_addr));
		zassert_true(net_pkt_filter_ip_recv_ok(pkt_v4), "");
	}

	/* And one not listed */
	memcpy((struct net_in_addr *)NET_IPV4_HDR(pkt_v4)->src,
				 &bad_addr, sizeof(struct net_in_addr));
	zassert_false(net_pkt_filter_ip_recv_ok(pkt_v4), "");
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v6), "");

	/* Prepare new test */
	zassert_true(npf_remove_all_ipv4_recv_rules(), "");

	/* make sure pkt is initially accepted */
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v4), "");
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v6), "");

	/* validate blocklist */
	npf_insert_ipv4_recv_rule(&ipv4_blocklist);

	for (int it = 0; it < ARRAY_SIZE(ipv4_address_list); it++) {
		memcpy((struct net_in_addr *)NET_IPV4_HDR(pkt_v4)->src, &ipv4_address_list[it],
		       sizeof(struct net_in_addr));
		zassert_false(net_pkt_filter_ip_recv_ok(pkt_v4), "");
	}

	/* And one not listed */
	memcpy((struct net_in_addr *)NET_IPV4_HDR(pkt_v4)->src,
				 &bad_addr, sizeof(struct net_in_addr));
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v4), "");

	zassert_true(npf_remove_all_ipv4_recv_rules(), "");
	net_pkt_unref(pkt_v6);
	net_pkt_unref(pkt_v4);
}

static NPF_IP_SRC_ADDR_ALLOWLIST(allowlist_ipv6_src_addr, (void *)ipv6_address_list,
					 ARRAY_SIZE(ipv6_address_list), NET_AF_INET6);
static NPF_IP_SRC_ADDR_BLOCKLIST(blocklist_ipv6_src_addr, (void *)ipv6_address_list,
					 ARRAY_SIZE(ipv6_address_list), NET_AF_INET6);

static NPF_RULE(ipv6_allowlist, NET_OK, allowlist_ipv6_src_addr);
static NPF_RULE(ipv6_blocklist, NET_OK, blocklist_ipv6_src_addr);

ZTEST(net_pkt_filter_test_suite, test_npf_ipv6_address_filtering)
{
	struct net_in6_addr dst = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0xf2, 0xaa, 0x29, 0x02, 0x04 } } };
	struct net_in6_addr bad_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 8, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	struct net_pkt *pkt_v6 = build_test_ip_pkt(&ipv6_address_list[0], &dst, NET_AF_INET6,
						   dummy_iface_a);
	struct net_pkt *pkt_v4 = build_test_ip_pkt(&ipv4_address_list[0], &ipv4_address_list[1],
						   NET_AF_INET, dummy_iface_a);

	/* make sure pkt is initially accepted */
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v4), "");
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v6), "");

	/* validate allowlist */
	npf_insert_ipv6_recv_rule(&ipv6_allowlist);

	for (int it = 0; it < ARRAY_SIZE(ipv6_address_list); it++) {
		memcpy((struct net_in6_addr *)NET_IPV6_HDR(pkt_v6)->src,
				 &ipv6_address_list[it], sizeof(struct net_in6_addr));
		zassert_true(net_pkt_filter_ip_recv_ok(pkt_v6), "");
	}

	/* And one not listed */
	memcpy((struct net_in6_addr *)NET_IPV6_HDR(pkt_v6)->src,
				 &bad_addr, sizeof(struct net_in6_addr));
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v4), "");
	zassert_false(net_pkt_filter_ip_recv_ok(pkt_v6), "");

	/* Prepare new test */
	zassert_true(npf_remove_all_ipv6_recv_rules(), "");

	/* make sure pkt is initially accepted */
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v4), "");
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v6), "");

	/* validate blocklist */
	npf_insert_ipv6_recv_rule(&ipv6_blocklist);

	for (int it = 0; it < ARRAY_SIZE(ipv6_address_list); it++) {
		memcpy((struct net_in6_addr *)NET_IPV6_HDR(pkt_v6)->src,
				 &ipv6_address_list[it], sizeof(struct net_in6_addr));
		zassert_false(net_pkt_filter_ip_recv_ok(pkt_v6), "");
	}

	/* And one not listed */
	memcpy((struct net_in6_addr *)NET_IPV4_HDR(pkt_v6)->src,
				 &bad_addr, sizeof(struct net_in6_addr));
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v6), "");

	zassert_true(npf_remove_all_ipv6_recv_rules(), "");
	net_pkt_unref(pkt_v6);
	net_pkt_unref(pkt_v4);
}

ZTEST(net_pkt_filter_test_suite, test_npf_ip_address_family)
{
	/* Lists of one address, so that reading the IPv4 list as an IPv6
	 * address stays inside ipv4_address_list.
	 */
	static NPF_IP_SRC_ADDR_ALLOWLIST(allow_v4, (void *)ipv4_address_list, 1, NET_AF_INET);
	static NPF_IP_SRC_ADDR_BLOCKLIST(block_v4, (void *)ipv4_address_list, 1, NET_AF_INET);
	static NPF_IP_SRC_ADDR_ALLOWLIST(allow_v6, (void *)ipv6_address_list, 1, NET_AF_INET6);
	static NPF_IP_SRC_ADDR_BLOCKLIST(block_v6, (void *)ipv6_address_list, 1, NET_AF_INET6);
	static NPF_RULE(accept_v4, NET_OK, allow_v4);
	static NPF_RULE(accept_v6, NET_OK, allow_v6);
	static NPF_RULE(drop_others, NET_DROP, block_v4, block_v6);
	struct net_pkt *pkt_v4 = build_test_ip_pkt(&ipv4_address_list[1], &ipv4_address_list[2],
						   NET_AF_INET, dummy_iface_a);
	struct net_pkt *pkt_v6 = build_test_ip_pkt(&ipv6_address_list[1], &ipv6_address_list[2],
						   NET_AF_INET6, dummy_iface_a);

	BUILD_ASSERT(sizeof(ipv4_address_list) == sizeof(struct net_in6_addr));

	/* Give the packets the source address that a condition would find if
	 * it read the list of the other family as addresses of the family of
	 * the packet: the four IPv4 addresses as one IPv6 address, and the
	 * start of the first IPv6 address as an IPv4 address. A packet is in
	 * no list of the other family, so the allowlists are false and the
	 * blocklists are true for these packets.
	 */
	memcpy(NET_IPV6_HDR(pkt_v6)->src, ipv4_address_list, sizeof(struct net_in6_addr));
	memcpy(NET_IPV4_HDR(pkt_v4)->src, ipv6_address_list, sizeof(struct net_in_addr));
	zassert_false(allow_v4.test.fn(&allow_v4.test, pkt_v6));
	zassert_false(allow_v6.test.fn(&allow_v6.test, pkt_v4));
	zassert_true(block_v4.test.fn(&block_v4.test, pkt_v6));
	zassert_true(block_v6.test.fn(&block_v6.test, pkt_v4));

	/* A rule per family accepts the addresses of both lists and no others */
	npf_append_send_rule(&accept_v4);
	npf_append_send_rule(&accept_v6);
	zassert_false(net_pkt_filter_send_ok(pkt_v4));
	zassert_false(net_pkt_filter_send_ok(pkt_v6));

	memcpy(NET_IPV4_HDR(pkt_v4)->src, &ipv4_address_list[0], sizeof(struct net_in_addr));
	memcpy(NET_IPV6_HDR(pkt_v6)->src, &ipv6_address_list[0], sizeof(struct net_in6_addr));
	zassert_true(net_pkt_filter_send_ok(pkt_v4));
	zassert_true(net_pkt_filter_send_ok(pkt_v6));

	zassert_true(npf_remove_send_rule(&accept_v4));
	zassert_true(npf_remove_send_rule(&accept_v6));

	/* A rule with the blocklists of both families drops all other addresses */
	npf_append_send_rule(&drop_others);
	npf_append_send_rule(&npf_default_ok);
	zassert_true(net_pkt_filter_send_ok(pkt_v4));
	zassert_true(net_pkt_filter_send_ok(pkt_v6));

	memcpy(NET_IPV4_HDR(pkt_v4)->src, &ipv4_address_list[1], sizeof(struct net_in_addr));
	memcpy(NET_IPV6_HDR(pkt_v6)->src, &ipv6_address_list[1], sizeof(struct net_in6_addr));
	zassert_false(net_pkt_filter_send_ok(pkt_v4));
	zassert_false(net_pkt_filter_send_ok(pkt_v6));

	zassert_true(npf_remove_send_rule(&drop_others));
	zassert_true(npf_remove_send_rule(&npf_default_ok));
	net_pkt_unref(pkt_v6);
	net_pkt_unref(pkt_v4);
}

static NPF_ETH_VLAN_TYPE_MATCH(vlan_ip_packet, NET_ETH_PTYPE_IP);
static NPF_RULE(vlan_small_ip_pkt, NET_OK, vlan_ip_packet, maxsize_200);

static void test_npf_example_vlan_common(void)
{
	struct net_pkt *pkt;

	/* test small IP packet */
	pkt = build_vlan_test_pkt(NET_ETH_PTYPE_IP, 100, NULL);
	zassert_true(net_pkt_filter_recv_ok(pkt), "");
	net_pkt_unref(pkt);

	/* test "big" IP packet */
	pkt = build_vlan_test_pkt(NET_ETH_PTYPE_IP, 300, NULL);
	zassert_false(net_pkt_filter_recv_ok(pkt), "");
	net_pkt_unref(pkt);

	/* test "small" non-IP packet */
	pkt = build_vlan_test_pkt(NET_ETH_PTYPE_ARP, 100, NULL);
	zassert_false(net_pkt_filter_recv_ok(pkt), "");
	net_pkt_unref(pkt);

	/* test "big" non-IP packet */
	pkt = build_vlan_test_pkt(NET_ETH_PTYPE_ARP, 300, NULL);
	zassert_false(net_pkt_filter_recv_ok(pkt), "");
	net_pkt_unref(pkt);
}

ZTEST(net_pkt_filter_test_suite, test_npf_vlan_filtering)
{
	/* install filter rules */
	npf_insert_recv_rule(&npf_default_drop);
	npf_insert_recv_rule(&vlan_small_ip_pkt);

	test_npf_example_vlan_common();

	/* remove filter rules */
	zassert_true(npf_remove_recv_rule(&npf_default_drop), "");
	zassert_true(npf_remove_recv_rule(&vlan_small_ip_pkt), "");
}

/*
 * Local-in filtering
 *
 * The conditions for the local-in rules use what the IP input stored in the
 * packet, so they are tested with packets that go through the IP input up to
 * the local-in hook.
 */

/* Enough for an IP header, its options or extension headers and a TCP header */
#define TEST_IP_PKT_MAX_LEN 96

/* Length of the IPv4 options and of each IPv6 extension header in the tests */
#define TEST_IP_OPTS_LEN 8

#define TEST_IPV4_ADDR      192, 0, 2, 1
#define TEST_IPV4_PEER_ADDR 192, 0, 2, 2
#define TEST_IPV6_ADDR      0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
#define TEST_IPV6_PEER_ADDR 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2

/* What the local-in rules saw of the packets given to the IP input */
struct local_in_test_result {
	/* Packets for which the condition under test was true */
	uint32_t matched;
	/* Packets that reached the local-in hook and did not match */
	uint32_t unmatched;
};

static struct local_in_test_result local_in_result;

static bool local_in_test_count(struct net_pkt *pkt, void *user_data)
{
	uint32_t *counter = user_data;

	ARG_UNUSED(pkt);

	(*counter)++;

	return true;
}

/* Both rules drop the packet, so that it is not answered */
static NPF_LOCAL_IN_MATCH(local_in_test_count_matched, local_in_test_count,
			  &local_in_result.matched);
static NPF_LOCAL_IN_MATCH(local_in_test_count_unmatched, local_in_test_count,
			  &local_in_result.unmatched);
static NPF_RULE(local_in_test_matched, NET_DROP, local_in_test_count_matched,
		local_in_test_count_matched);
static NPF_RULE(local_in_test_unmatched, NET_DROP, local_in_test_count_unmatched);

/* Install the local-in rules that count the packets for which cond is true */
static void local_in_test_start(struct npf_test *cond)
{
	static const struct net_in_addr ipv4_addr = {{{TEST_IPV4_ADDR}}};
	static const struct net_in6_addr ipv6_addr = {{{TEST_IPV6_ADDR}}};
	struct net_if_addr *ifaddr;

	zassert_not_null(net_if_ipv4_addr_add(dummy_iface_a, (struct net_in_addr *)&ipv4_addr,
					      NET_ADDR_MANUAL, 0));

	ifaddr = net_if_ipv6_addr_add(dummy_iface_a, (struct net_in6_addr *)&ipv6_addr,
				      NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr);
	/* Do not wait for the duplicate address detection */
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	local_in_result = (struct local_in_test_result){0};
	local_in_test_matched.tests[0] = cond;

	npf_append_local_in_recv_rule(&local_in_test_matched);
	npf_append_local_in_recv_rule(&local_in_test_unmatched);
}

static struct local_in_test_result local_in_test_stop(void)
{
	zassert_true(npf_remove_all_local_in_recv_rules());

	return local_in_result;
}

/* Give a packet to the IP input. The data from offset split on is put into a
 * buffer of its own.
 */
static void local_in_test_input(net_sa_family_t family, const uint8_t *data, size_t len,
				size_t split)
{
	const size_t part_len[] = {MIN(split, len), len - MIN(split, len)};
	enum net_verdict verdict;
	struct net_pkt *pkt;

	pkt = net_pkt_rx_alloc_on_iface(dummy_iface_a, K_NO_WAIT);
	zassert_not_null(pkt);

	ARRAY_FOR_EACH(part_len, i) {
		struct net_buf *buf;

		if (part_len[i] > 0U) {
			buf = net_pkt_get_frag(pkt, part_len[i], K_NO_WAIT);
			zassert_not_null(buf);
			zassert_true(net_buf_tailroom(buf) >= part_len[i]);

			net_buf_add_mem(buf, data, part_len[i]);
			net_pkt_frag_add(pkt, buf);
			data += part_len[i];
		}
	}

	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_init(pkt);

	verdict = (family == NET_AF_INET) ? net_ipv4_input(pkt) : net_ipv6_input(pkt);
	if (verdict == NET_DROP) {
		net_pkt_unref(pkt);
	}
}

/* Give one packet to the IP input and tell what the local-in rules saw of it */
static struct local_in_test_result local_in_test_pkt_split(struct npf_test *cond,
							   net_sa_family_t family,
							   const uint8_t *data, size_t len,
							   size_t split)
{
	local_in_test_start(cond);
	local_in_test_input(family, data, len, split);

	return local_in_test_stop();
}

static uint32_t test_chksum_add(uint32_t sum, const uint8_t *data, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		sum += ((i % 2U) == 0U) ? ((uint32_t)data[i] << 8) : data[i];
	}

	return sum;
}

static void test_chksum_put(uint32_t sum, uint8_t *chksum)
{
	while ((sum >> 16) != 0U) {
		sum = (sum & 0xffffU) + (sum >> 16);
	}

	sys_put_be16((uint16_t)~sum, chksum);
}

/* Write an IPv4 packet that holds payload, with opts_len bytes of no-operation
 * IP options.
 */
static size_t put_ipv4_pkt(uint8_t *data, uint8_t proto, size_t opts_len, const uint8_t *payload,
			   size_t payload_len)
{
	size_t hdr_len = sizeof(struct net_ipv4_hdr) + opts_len;
	struct net_ipv4_hdr hdr = {
		.vhl = 0x40 | (hdr_len / 4U),
		.len = net_htons(hdr_len + payload_len),
		.ttl = 64,
		.proto = proto,
		.src = {TEST_IPV4_PEER_ADDR},
		.dst = {TEST_IPV4_ADDR},
	};

	memcpy(data, &hdr, sizeof(hdr));
	memset(&data[sizeof(hdr)], NET_IPV4_OPTS_NOP, opts_len);
	test_chksum_put(test_chksum_add(0, data, hdr_len),
			&data[offsetof(struct net_ipv4_hdr, chksum)]);

	memcpy(&data[hdr_len], payload, payload_len);

	return hdr_len + payload_len;
}

/* Write an IPv6 packet that holds payload. If ext_hdrs is true, it has a
 * hop-by-hop options and a destination options header, which hold one PadN
 * option each.
 */
static size_t put_ipv6_pkt(uint8_t *data, uint8_t proto, bool ext_hdrs, const uint8_t *payload,
			   size_t payload_len)
{
	struct net_ipv6_hdr hdr = {
		.vtc = 0x60,
		.nexthdr = ext_hdrs ? NET_IPV6_NEXTHDR_HBHO : proto,
		.hop_limit = 64,
		.src = {TEST_IPV6_PEER_ADDR},
		.dst = {TEST_IPV6_ADDR},
	};
	size_t len = sizeof(hdr);

	if (ext_hdrs) {
		const uint8_t ext_hdr[TEST_IP_OPTS_LEN] = {
			0, 0, NET_IPV6_EXT_HDR_OPT_PADN, 4, 0, 0, 0, 0,
		};

		memcpy(&data[len], ext_hdr, sizeof(ext_hdr));
		data[len] = NET_IPV6_NEXTHDR_DESTO;
		len += sizeof(ext_hdr);

		memcpy(&data[len], ext_hdr, sizeof(ext_hdr));
		data[len] = proto;
		len += sizeof(ext_hdr);
	}

	memcpy(&data[len], payload, payload_len);
	len += payload_len;

	hdr.len = net_htons(len - sizeof(hdr));
	memcpy(data, &hdr, sizeof(hdr));

	return len;
}

/*
 * UDP and TCP port filtering
 */

#define TEST_SRC_PORT 49152
#define TEST_DST_PORT 4242

/* Write a UDP or TCP header for a packet of the given family, followed by
 * data_len bytes of data. The TCP header and the data hold tcp_flags, so that
 * a UDP datagram can have them where the TCP header has its flags.
 */
static size_t put_port_hdr(uint8_t *data, net_sa_family_t family, uint8_t proto, uint16_t src_port,
			   uint16_t dst_port, uint8_t tcp_flags, size_t data_len)
{
	const uint8_t ipv4_addrs[] = {TEST_IPV4_PEER_ADDR, TEST_IPV4_ADDR};
	const uint8_t ipv6_addrs[] = {TEST_IPV6_PEER_ADDR, TEST_IPV6_ADDR};
	size_t hdr_len = (proto == NET_IPPROTO_UDP) ? sizeof(struct net_udp_hdr)
						    : sizeof(struct net_tcp_hdr);
	size_t chksum_offset = (proto == NET_IPPROTO_UDP) ? offsetof(struct net_udp_hdr, chksum)
							  : offsetof(struct net_tcp_hdr, chksum);
	size_t len = hdr_len + data_len;
	/* The pseudo header adds the protocol, the length and the addresses */
	uint32_t sum = proto + len;

	if (family == NET_AF_INET) {
		sum = test_chksum_add(sum, ipv4_addrs, sizeof(ipv4_addrs));
	} else {
		sum = test_chksum_add(sum, ipv6_addrs, sizeof(ipv6_addrs));
	}

	memset(data, 0, hdr_len);
	memset(&data[hdr_len], tcp_flags, data_len);
	sys_put_be16(src_port, &data[0]);
	sys_put_be16(dst_port, &data[2]);

	if (proto == NET_IPPROTO_UDP) {
		sys_put_be16(len, &data[offsetof(struct net_udp_hdr, len)]);
	} else {
		data[offsetof(struct net_tcp_hdr, offset)] = (hdr_len / 4U) << 4;
		data[offsetof(struct net_tcp_hdr, flags)] = tcp_flags;
	}

	test_chksum_put(test_chksum_add(sum, data, len), &data[chksum_offset]);

	return len;
}

/* Write a UDP or TCP packet from TEST_SRC_PORT to TEST_DST_PORT. With ip_opts,
 * IP options or extension headers precede the UDP or TCP header. The offset
 * of that header is stored in hdr_offset.
 */
static size_t put_port_pkt(uint8_t *data, net_sa_family_t family, uint8_t proto, bool ip_opts,
			   uint8_t tcp_flags, size_t data_len, size_t *hdr_offset)
{
	uint8_t payload[sizeof(struct net_tcp_hdr)];
	size_t payload_len;
	size_t len;

	zassert_true(((proto == NET_IPPROTO_UDP) ? sizeof(struct net_udp_hdr)
						 : sizeof(struct net_tcp_hdr)) +
			     data_len <=
		     sizeof(payload));

	payload_len = put_port_hdr(payload, family, proto, TEST_SRC_PORT, TEST_DST_PORT, tcp_flags,
				   data_len);

	if (family == NET_AF_INET) {
		len = put_ipv4_pkt(data, proto, ip_opts ? TEST_IP_OPTS_LEN : 0, payload,
				   payload_len);
	} else {
		len = put_ipv6_pkt(data, proto, ip_opts, payload, payload_len);
	}

	*hdr_offset = len - payload_len;

	return len;
}

/* Give a UDP or TCP packet from TEST_SRC_PORT to TEST_DST_PORT to the IP input
 * and check which port conditions are true for it. With ip_opts, IP options
 * or extension headers precede the UDP or TCP header. With split_hdr, the
 * header is split between two buffers after its first byte.
 */
static void port_test_pkt(net_sa_family_t family, uint8_t proto, bool ip_opts, bool split_hdr)
{
	static NPF_UDP_SRC_PORT_MATCH(udp_src, TEST_SRC_PORT);
	static NPF_UDP_DST_PORT_MATCH(udp_dst, TEST_DST_PORT);
	static NPF_TCP_SRC_PORT_MATCH(tcp_src, TEST_SRC_PORT);
	static NPF_TCP_DST_PORT_MATCH(tcp_dst, TEST_DST_PORT);
	/* The port of the other direction must not match */
	static NPF_UDP_SRC_PORT_MATCH(udp_src_other, TEST_DST_PORT);
	static NPF_UDP_DST_PORT_MATCH(udp_dst_other, TEST_SRC_PORT);
	static NPF_TCP_SRC_PORT_MATCH(tcp_src_other, TEST_DST_PORT);
	static NPF_TCP_DST_PORT_MATCH(tcp_dst_other, TEST_SRC_PORT);
	const struct {
		struct npf_test *cond;
		uint8_t proto;
		bool port_matches;
	} conds[] = {
		{&udp_src.test, NET_IPPROTO_UDP, true},
		{&udp_dst.test, NET_IPPROTO_UDP, true},
		{&tcp_src.test, NET_IPPROTO_TCP, true},
		{&tcp_dst.test, NET_IPPROTO_TCP, true},
		{&udp_src_other.test, NET_IPPROTO_UDP, false},
		{&udp_dst_other.test, NET_IPPROTO_UDP, false},
		{&tcp_src_other.test, NET_IPPROTO_TCP, false},
		{&tcp_dst_other.test, NET_IPPROTO_TCP, false},
	};
	uint8_t data[TEST_IP_PKT_MAX_LEN];
	size_t hdr_offset;
	size_t split;
	size_t len;

	len = put_port_pkt(data, family, proto, ip_opts, NPF_TCP_FLAG_SYN, 0, &hdr_offset);
	split = split_hdr ? (hdr_offset + 1) : len;

	ARRAY_FOR_EACH(conds, i) {
		struct local_in_test_result result;
		bool match = conds[i].port_matches && conds[i].proto == proto;

		result = local_in_test_pkt_split(conds[i].cond, family, data, len, split);
		zassert_true(result.matched == (match ? 1U : 0U) &&
				     result.unmatched == (match ? 0U : 1U),
			     "condition %zu: matched %u unmatched %u", i, result.matched,
			     result.unmatched);
	}
}

static void port_test_proto(uint8_t proto)
{
	const net_sa_family_t families[] = {NET_AF_INET, NET_AF_INET6};

	ARRAY_FOR_EACH(families, i) {
		port_test_pkt(families[i], proto, false, false);
		port_test_pkt(families[i], proto, true, false);
		port_test_pkt(families[i], proto, false, true);
		port_test_pkt(families[i], proto, true, true);
	}
}

ZTEST(net_pkt_filter_test_suite, test_npf_udp_port)
{
	port_test_proto(NET_IPPROTO_UDP);
}

ZTEST(net_pkt_filter_test_suite, test_npf_tcp_port)
{
	port_test_proto(NET_IPPROTO_TCP);
}

/*
 * TCP flags filtering
 */

/* Data of the UDP datagram, which has to reach as far as the TCP flags */
#define TEST_UDP_DATA_LEN 8

ZTEST(net_pkt_filter_test_suite, test_npf_tcp_flags)
{
	static NPF_TCP_FLAGS_MATCH(syn, NPF_TCP_FLAG_SYN | NPF_TCP_FLAG_ACK, NPF_TCP_FLAG_SYN);
	static NPF_TCP_FLAGS_MATCH(syn_ack, NPF_TCP_FLAG_SYN | NPF_TCP_FLAG_ACK,
				   NPF_TCP_FLAG_SYN | NPF_TCP_FLAG_ACK);
	static NPF_TCP_FLAGS_MATCH(no_syn, NPF_TCP_FLAG_SYN, 0);
	static NPF_TCP_FLAGS_MATCH(any, 0, 0);
	struct npf_test *const conds[] = {&syn.test, &syn_ack.test, &no_syn.test, &any.test};
	const struct {
		uint8_t proto;
		uint8_t flags;
		/* Whether the conditions are true, in the order of conds */
		bool match[ARRAY_SIZE(conds)];
	} pkts[] = {
		{NET_IPPROTO_TCP, NPF_TCP_FLAG_SYN, {true, false, false, true}},
		{NET_IPPROTO_TCP, NPF_TCP_FLAG_SYN | NPF_TCP_FLAG_ACK, {false, true, false, true}},
		{NET_IPPROTO_TCP, NPF_TCP_FLAG_ACK, {false, false, true, true}},
		{NET_IPPROTO_TCP, NPF_TCP_FLAG_RST, {false, false, true, true}},
		/* A flag that is not in the mask does not matter */
		{NET_IPPROTO_TCP, NPF_TCP_FLAG_SYN | NPF_TCP_FLAG_PSH, {true, false, false, true}},
		/* The flags are in the data of the UDP datagram */
		{NET_IPPROTO_UDP, NPF_TCP_FLAG_SYN, {false, false, false, false}},
	};
	const net_sa_family_t families[] = {NET_AF_INET, NET_AF_INET6};
	const bool ip_opts[] = {false, true};
	const bool split_hdr[] = {false, true};

	ARRAY_FOR_EACH(families, f) {
		ARRAY_FOR_EACH(ip_opts, o) {
			ARRAY_FOR_EACH(split_hdr, s) {
				ARRAY_FOR_EACH(pkts, p) {
					uint8_t data[TEST_IP_PKT_MAX_LEN];
					size_t data_len = (pkts[p].proto == NET_IPPROTO_UDP)
								  ? TEST_UDP_DATA_LEN
								  : 0;
					size_t hdr_offset;
					size_t split;
					size_t len;

					len = put_port_pkt(data, families[f], pkts[p].proto,
							   ip_opts[o], pkts[p].flags, data_len,
							   &hdr_offset);
					/* The flags are in the second buffer */
					split = split_hdr[s] ? (hdr_offset + 1) : len;

					ARRAY_FOR_EACH(conds, c) {
						struct local_in_test_result result;
						bool match = pkts[p].match[c];

						result = local_in_test_pkt_split(
							conds[c], families[f], data, len, split);
						zassert_true(
							result.matched == (match ? 1U : 0U) &&
								result.unmatched ==
									(match ? 0U : 1U),
							"family %zu opts %zu split %zu packet %zu "
							"condition %zu: matched %u unmatched %u",
							f, o, s, p, c, result.matched,
							result.unmatched);
					}
				}
			}
		}
	}
}

ZTEST_SUITE(net_pkt_filter_test_suite, NULL, test_npf_iface, NULL, NULL, NULL);
