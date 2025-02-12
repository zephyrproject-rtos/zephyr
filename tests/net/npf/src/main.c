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
"Intel x86, ARC, Nios II, Tensilica Xtensa, and RISC-V, and a large number of\n"
"`supported boards`_.\n";


static struct net_pkt *build_test_pkt(int type, int size, struct net_if *iface)
{
	struct net_pkt *pkt;
	struct net_eth_hdr eth_hdr;
	int ret;

	pkt = net_pkt_rx_alloc_with_buffer(iface, size, AF_UNSPEC, 0, K_NO_WAIT);
	zassert_not_null(pkt, "");

	eth_hdr.src = ETH_SRC_ADDR;
	eth_hdr.dst = ETH_DST_ADDR;
	eth_hdr.type = htons(type);

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
	   sa_family_t family, struct net_if *iface)
{
	struct net_pkt *pkt;
	int ret = -1;
	int size;

	size = (family == AF_INET) ? sizeof(struct net_ipv4_hdr) :
		(family == AF_INET6) ? sizeof(struct net_ipv6_hdr) : 0U;

	pkt = net_pkt_rx_alloc_with_buffer(iface, size, family, 0, K_NO_WAIT);
	zassert_not_null(pkt, "");

	if (family == AF_INET) {
		ret = net_ipv4_create(pkt, (struct in_addr *)src, (struct in_addr *)dst);
	} else if (family == AF_INET6) {
		ret = net_ipv6_create(pkt, (struct in6_addr *)src, (struct in6_addr *)dst);
	}
	zassert_equal(ret, 0, "Cannot create %s packet (%d)",
		(family == AF_INET) ? "IPv4" : "IPv6", ret);

	DBG("pkt %p: iface %p size %d sa_family %d\n", pkt, iface, size, family);
	return pkt;
}

/*
 * Declare some fake interfaces and test their filter conditions.
 */

ETH_NET_DEVICE_INIT(dummy_iface_a, "dummy_a", NULL, NULL,
		    NULL, NULL, CONFIG_ETH_INIT_PRIORITY,
		    NULL, NET_ETH_MTU);
ETH_NET_DEVICE_INIT(dummy_iface_b, "dummy_b", NULL, NULL,
		    NULL, NULL, CONFIG_ETH_INIT_PRIORITY,
		    NULL, NET_ETH_MTU);
#define dummy_iface_a NET_IF_GET_NAME(dummy_iface_a, 0)[0]
#define dummy_iface_b NET_IF_GET_NAME(dummy_iface_b, 0)[0]

static NPF_IFACE_MATCH(match_iface_a, &dummy_iface_a);
static NPF_IFACE_UNMATCH(unmatch_iface_b, &dummy_iface_b);

static NPF_RULE(accept_iface_a, NET_OK, match_iface_a);
static NPF_RULE(accept_all_but_iface_b, NET_OK, unmatch_iface_b);

static void *test_npf_iface(void)
{
	struct net_pkt *pkt_iface_a, *pkt_iface_b;

	pkt_iface_a = build_test_pkt(0, 200, &dummy_iface_a);
	pkt_iface_b = build_test_pkt(0, 200, &dummy_iface_b);

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

static struct in_addr ipv4_address_list[4] = {
	{ { { 192, 168, 1, 1 } } },
	{ { { 192, 0, 2, 1 } } },
	{ { { 172, 16, 0, 1 } } },
	{ { { 10, 49, 0, 252 } } }
};

static struct in6_addr ipv6_address_list[4] = {
	{ { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } },
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } },
	{ { { 0x20, 0x01, 0x0d, 0xb8, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } },
	{ { { 0x20, 0x01, 0x0d, 0xb8, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } },
};

static NPF_IP_SRC_ADDR_ALLOWLIST(allowlist_ipv4_src_addr, (void *)ipv4_address_list,
					 ARRAY_SIZE(ipv4_address_list), AF_INET);
static NPF_IP_SRC_ADDR_BLOCKLIST(blocklist_ipv4_src_addr, (void *)ipv4_address_list,
					 ARRAY_SIZE(ipv4_address_list), AF_INET);

static NPF_RULE(ipv4_allowlist, NET_OK, allowlist_ipv4_src_addr);
static NPF_RULE(ipv4_blocklist, NET_OK, blocklist_ipv4_src_addr);

ZTEST(net_pkt_filter_test_suite, test_npf_ipv4_address_filtering)
{
	struct in_addr dst = { { { 192, 168, 2, 1 } } };
	struct in_addr bad_addr = { { { 192, 168, 2, 3 } } };
	struct net_pkt *pkt_v4 = build_test_ip_pkt(&ipv4_address_list[0], &dst, AF_INET,
						   &dummy_iface_a);
	struct net_pkt *pkt_v6 = build_test_ip_pkt(&ipv6_address_list[0], &ipv6_address_list[1],
						   AF_INET6, &dummy_iface_a);

	/* make sure pkt is initially accepted */
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v4), "");
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v6), "");

	/* validate allowlist */
	npf_insert_ipv4_recv_rule(&ipv4_allowlist);

	for (int it = 0; it < ARRAY_SIZE(ipv4_address_list); it++) {
		memcpy((struct in_addr *)NET_IPV4_HDR(pkt_v4)->src, &ipv4_address_list[it],
		       sizeof(struct in_addr));
		zassert_true(net_pkt_filter_ip_recv_ok(pkt_v4), "");
	}

	/* And one not listed */
	memcpy((struct in_addr *)NET_IPV4_HDR(pkt_v4)->src,
				 &bad_addr, sizeof(struct in_addr));
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
		memcpy((struct in_addr *)NET_IPV4_HDR(pkt_v4)->src, &ipv4_address_list[it],
		       sizeof(struct in_addr));
		zassert_false(net_pkt_filter_ip_recv_ok(pkt_v4), "");
	}

	/* And one not listed */
	memcpy((struct in_addr *)NET_IPV4_HDR(pkt_v4)->src,
				 &bad_addr, sizeof(struct in_addr));
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v4), "");

	zassert_true(npf_remove_all_ipv4_recv_rules(), "");
	net_pkt_unref(pkt_v6);
	net_pkt_unref(pkt_v4);
}

static NPF_IP_SRC_ADDR_ALLOWLIST(allowlist_ipv6_src_addr, (void *)ipv6_address_list,
					 ARRAY_SIZE(ipv6_address_list), AF_INET6);
static NPF_IP_SRC_ADDR_BLOCKLIST(blocklist_ipv6_src_addr, (void *)ipv6_address_list,
					 ARRAY_SIZE(ipv6_address_list), AF_INET6);

static NPF_RULE(ipv6_allowlist, NET_OK, allowlist_ipv6_src_addr);
static NPF_RULE(ipv6_blocklist, NET_OK, blocklist_ipv6_src_addr);

ZTEST(net_pkt_filter_test_suite, test_npf_ipv6_address_filtering)
{
	struct in6_addr dst = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0xf2, 0xaa, 0x29, 0x02, 0x04 } } };
	struct in6_addr bad_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 8, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	struct net_pkt *pkt_v6 = build_test_ip_pkt(&ipv6_address_list[0], &dst, AF_INET6,
						   &dummy_iface_a);
	struct net_pkt *pkt_v4 = build_test_ip_pkt(&ipv4_address_list[0], &ipv4_address_list[1],
						   AF_INET, &dummy_iface_a);

	/* make sure pkt is initially accepted */
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v4), "");
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v6), "");

	/* validate allowlist */
	npf_insert_ipv6_recv_rule(&ipv6_allowlist);

	for (int it = 0; it < ARRAY_SIZE(ipv6_address_list); it++) {
		memcpy((struct in6_addr *)NET_IPV6_HDR(pkt_v6)->src,
				 &ipv6_address_list[it], sizeof(struct in6_addr));
		zassert_true(net_pkt_filter_ip_recv_ok(pkt_v6), "");
	}

	/* And one not listed */
	memcpy((struct in6_addr *)NET_IPV6_HDR(pkt_v6)->src,
				 &bad_addr, sizeof(struct in6_addr));
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
		memcpy((struct in6_addr *)NET_IPV6_HDR(pkt_v6)->src,
				 &ipv6_address_list[it], sizeof(struct in6_addr));
		zassert_false(net_pkt_filter_ip_recv_ok(pkt_v6), "");
	}

	/* And one not listed */
	memcpy((struct in6_addr *)NET_IPV4_HDR(pkt_v6)->src,
				 &bad_addr, sizeof(struct in6_addr));
	zassert_true(net_pkt_filter_ip_recv_ok(pkt_v6), "");

	zassert_true(npf_remove_all_ipv6_recv_rules(), "");
	net_pkt_unref(pkt_v6);
	net_pkt_unref(pkt_v4);
}

ZTEST_SUITE(net_pkt_filter_test_suite, NULL, test_npf_iface, NULL, NULL, NULL);
