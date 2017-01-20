/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sections.h>

#include <tc_util.h>

#include <net/nbuf.h>
#include <net/net_ip.h>
#include <net/net_core.h>
#include <net/ethernet.h>

#include "icmpv6.h"
#include "ipv6.h"

#define NET_LOG_ENABLED 1
#include "net_private.h"

static struct in6_addr my_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct in6_addr peer_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					 0, 0, 0, 0, 0, 0, 0, 0x2 } } };
static struct in6_addr mcast_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* ICMPv6 NS frame (74 bytes) */
static const unsigned char icmpv6_ns_invalid[] = {
/* IPv6 header starts here */
0x60, 0x00, 0x00, 0x00, 0x00, 0x20, 0x3A, 0xFF,
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
/* ICMPv6 NS header starts here */
0x87, 0x00, 0x7B, 0x9C, 0x60, 0x00, 0x00, 0x00,
/* Target Address */
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
/* Source link layer address */
0x01, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0xD8,
/* Target link layer address */
0x02, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0xD7,
/* Source link layer address */
0x01, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0xD6,
/* MTU option */
0x05, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0xD5,
};

/* ICMPv6 NS frame (64 bytes) */
static const unsigned char icmpv6_ns_no_sllao[] = {
/* IPv6 header starts here */
0x60, 0x00, 0x00, 0x00, 0x00, 0x18, 0x3A, 0xFF,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
/* ICMPv6 NS header starts here */
0x87, 0x00, 0x7B, 0x9C, 0x60, 0x00, 0x00, 0x00,
/* Target Address */
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
};

/* */
static const unsigned char icmpv6_ra[] = {
/* IPv6 header starts here */
0x60, 0x00, 0x00, 0x00, 0x00, 0x58, 0x3a, 0xff,
0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x02, 0x60, 0x97, 0xff, 0xfe, 0x07, 0x69, 0xea,
0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
/* ICMPv6 RA header starts here */
0x86, 0x00, 0x46, 0x25, 0x40, 0x00, 0x07, 0x08,
0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01,
/* SLLAO */
0x01, 0x01, 0x00, 0x60, 0x97, 0x07, 0x69, 0xea,
/* MTU */
0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x05, 0xdc,
/* Prefix info*/
0x03, 0x04, 0x40, 0xc0, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
0x3f, 0xfe, 0x05, 0x07, 0x00, 0x00, 0x00, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 6CO */
0x22, 0x03, 0x40, 0x11, 0x00, 0x00, 0x12, 0x34,
0x3f, 0xfe, 0x05, 0x07, 0x00, 0x00, 0x00, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* IPv6 hop-by-hop option in the message */
static const unsigned char ipv6_hbho[] = {
/* IPv6 header starts here (IPv6 addresses are wrong) */
0x60, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x3f, /* `....6.? */
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
/* Hop-by-hop option starts here */
0x11, 0x00,
/* RPL sub-option starts here */
0x63, 0x04, 0x80, 0x1e, 0x01, 0x00, /* ..c..... */
/* UDP header starts here (checksum is "fixed" in this example) */
0xaa, 0xdc, 0xbf, 0xd7, 0x00, 0x2e, 0xa2, 0x55, /* ......M. */
/* User data starts here (38 bytes) */
0x10, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, /* ........ */
0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x02, /* ........ */
0x00, 0x00, 0x03, 0x00, 0x00, 0x02, 0x00, 0x03, /* ........ */
0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xc9, /* ........ */
0x00, 0x00, 0x01, 0x00, 0x00, 0x00,              /* ...... */
};

static bool test_failed;
static struct k_sem wait_data;

#define WAIT_TIME 250
#define WAIT_TIME_LONG MSEC_PER_SEC
#define SENDING 93244
#define MY_PORT 1969
#define PEER_PORT 16233

struct net_test_ipv6 {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_test_dev_init(struct device *dev)
{
	return 0;
}

static uint8_t *net_test_get_mac(struct device *dev)
{
	struct net_test_ipv6 *context = dev->driver_data;

	if (context->mac_addr[0] == 0x00) {
		/* 10-00-00-00-00 to 10-00-00-00-FF Documentation RFC7042 */
		context->mac_addr[0] = 0x10;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x00;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x00;
		context->mac_addr[5] = sys_rand32_get();
	}

	return context->mac_addr;
}

static void net_test_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_test_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr));
}

static struct net_buf *prepare_ra_message(void)
{
	struct net_buf *buf, *frag;
	struct net_if *iface;
	uint16_t reserve;

	buf = net_nbuf_get_reserve_rx(0);

	NET_ASSERT_INFO(buf, "Out of RX buffers");

	iface = net_if_get_default();

	reserve = net_if_get_ll_reserve(iface, NULL);

	frag = net_nbuf_get_reserve_data(reserve);

	net_buf_frag_add(buf, frag);

	net_nbuf_set_ll_reserve(buf, reserve);
	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_family(buf, AF_INET6);
	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));

	net_nbuf_ll_clear(buf);

	memcpy(net_buf_add(frag, sizeof(icmpv6_ra)),
	       icmpv6_ra, sizeof(icmpv6_ra));

	return buf;
}

static int tester_send(struct net_if *iface, struct net_buf *buf)
{
	struct net_icmp_hdr *icmp = NET_ICMP_BUF(buf);

	if (!buf->frags) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	/* Reply with RA messge */
	if (icmp->type == NET_ICMPV6_RS) {
		net_nbuf_unref(buf);
		buf = prepare_ra_message();
	}

	/* Feed this data back to us */
	if (net_recv_data(iface, buf) < 0) {
		TC_ERROR("Data receive failed.");
		goto out;
	}

	return 0;

out:
	net_nbuf_unref(buf);
	test_failed = true;

	return 0;
}

struct net_test_ipv6 net_test_data;

static struct net_if_api net_test_if_api = {
	.init = net_test_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(net_test_ipv6, "net_test_ipv6",
		net_test_dev_init, &net_test_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_test_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE,
		127);

static bool test_init(void)
{
	struct net_if_addr *ifaddr;
	struct net_if_mcast_addr *maddr;
	struct net_if *iface = net_if_get_default();

	if (!iface) {
		TC_ERROR("Interface is NULL\n");
		return false;
	}

	ifaddr = net_if_ipv6_addr_add(iface, &my_addr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		TC_ERROR("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr));
		return false;
	}

	net_ipv6_addr_create(&mcast_addr, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);

	maddr = net_if_ipv6_maddr_add(iface, &mcast_addr);
	if (!maddr) {
		TC_ERROR("Cannot add multicast IPv6 address %s\n",
		       net_sprint_ipv6_addr(&mcast_addr));
		return false;
	}

	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);

	return true;
}

static bool net_test_cmp_prefix(void)
{
	bool st;

	struct in6_addr prefix1 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	struct in6_addr prefix2 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x2 } } };

	st = net_is_ipv6_prefix((uint8_t *)&prefix1, (uint8_t *)&prefix2, 64);
	if (!st) {
		TC_ERROR("Prefix /64  compare failed\n");
		return false;
	}

	st = net_is_ipv6_prefix((uint8_t *)&prefix1, (uint8_t *)&prefix2, 65);
	if (!st) {
		TC_ERROR("Prefix /65 compare failed\n");
		return false;
	}

	/* Set one extra bit in the other prefix for testing /65 */
	prefix1.s6_addr[8] = 0x80;

	st = net_is_ipv6_prefix((uint8_t *)&prefix1, (uint8_t *)&prefix2, 65);
	if (st) {
		TC_ERROR("Prefix /65 compare should have failed\n");
		return false;
	}

	/* Set two bits in prefix2, it is now /66 */
	prefix2.s6_addr[8] = 0xc0;

	st = net_is_ipv6_prefix((uint8_t *)&prefix1, (uint8_t *)&prefix2, 65);
	if (!st) {
		TC_ERROR("Prefix /65 compare failed\n");
		return false;
	}

	/* Set all remaining bits in prefix2, it is now /128 */
	memset(&prefix2.s6_addr[8], 0xff, 8);

	st = net_is_ipv6_prefix((uint8_t *)&prefix1, (uint8_t *)&prefix2, 65);
	if (!st) {
		TC_ERROR("Prefix /65 compare failed\n");
		return false;
	}

	/* Comparing /64 should be still ok */
	st = net_is_ipv6_prefix((uint8_t *)&prefix1, (uint8_t *)&prefix2, 64);
	if (!st) {
		TC_ERROR("Prefix /64 compare failed\n");
		return false;
	}

	/* But comparing /66 should should fail */
	st = net_is_ipv6_prefix((uint8_t *)&prefix1, (uint8_t *)&prefix2, 66);
	if (st) {
		TC_ERROR("Prefix /66 compare should have failed\n");
		return false;
	}

	return true;
}

static bool net_test_send_ns_mcast(void)
{
	int ret;
	struct in6_addr tgt;

	net_ipv6_addr_create_solicited_node(&my_addr, &tgt);

	ret = net_ipv6_send_ns(net_if_get_default(),
			       NULL,
			       &peer_addr,
			       &my_addr,
			       &tgt,
			       false);
	if (ret < 0) {
		TC_ERROR("Cannot send NS (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_test_send_ns(void)
{
	int ret;

	ret = net_ipv6_send_ns(net_if_get_default(),
			       NULL,
			       &peer_addr,
			       &my_addr,
			       &peer_addr,
			       false);
	if (ret < 0) {
		TC_ERROR("Cannot send NS (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_test_nbr_lookup_fail(void)
{
	struct net_nbr *nbr;

	nbr = net_ipv6_nbr_lookup(net_if_get_default(),
				  &peer_addr);
	if (nbr) {
		TC_ERROR("Neighbor %s found in cache\n",
			 net_sprint_ipv6_addr(&peer_addr));
		return false;
	}

	return true;
}

static bool net_test_nbr_lookup_ok(void)
{
	struct net_nbr *nbr;

	nbr = net_ipv6_nbr_lookup(net_if_get_default(),
				  &peer_addr);
	if (!nbr) {
		TC_ERROR("Neighbor %s not found in cache\n",
			 net_sprint_ipv6_addr(&peer_addr));
		return false;
	}

	return true;
}

static bool net_test_send_ns_extra_options(void)
{
	struct net_buf *buf, *frag;
	struct net_if *iface;
	uint16_t reserve;

	buf = net_nbuf_get_reserve_tx(0);

	NET_ASSERT_INFO(buf, "Out of TX buffers");

	iface = net_if_get_default();

	reserve = net_if_get_ll_reserve(iface, NULL);

	frag = net_nbuf_get_reserve_data(reserve);

	net_buf_frag_add(buf, frag);

	net_nbuf_set_ll_reserve(buf, reserve);
	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_family(buf, AF_INET6);
	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));

	net_nbuf_ll_clear(buf);

	memcpy(net_buf_add(frag, sizeof(icmpv6_ns_invalid)),
	       icmpv6_ns_invalid, sizeof(icmpv6_ns_invalid));

	if (net_recv_data(iface, buf) < 0) {
		TC_ERROR("Data receive for invalid NS failed.");
		return false;
	}

	return true;
}

static bool net_test_send_ns_no_options(void)
{
	struct net_buf *buf, *frag;
	struct net_if *iface;
	uint16_t reserve;

	buf = net_nbuf_get_reserve_tx(0);

	NET_ASSERT_INFO(buf, "Out of TX buffers");

	iface = net_if_get_default();

	reserve = net_if_get_ll_reserve(iface, NULL);

	frag = net_nbuf_get_reserve_data(reserve);

	net_buf_frag_add(buf, frag);

	net_nbuf_set_ll_reserve(buf, reserve);
	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_family(buf, AF_INET6);
	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));

	net_nbuf_ll_clear(buf);

	memcpy(net_buf_add(frag, sizeof(icmpv6_ns_no_sllao)),
	       icmpv6_ns_no_sllao, sizeof(icmpv6_ns_no_sllao));

	if (net_recv_data(iface, buf) < 0) {
		TC_ERROR("Data receive for invalid NS failed.");
		return false;
	}

	return true;
}

static bool net_test_prefix_timeout(void)
{
	struct net_if_ipv6_prefix *prefix;
	struct in6_addr addr = { { { 0x20, 1, 0x0d, 0xb8, 42, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0 } } };
	uint32_t lifetime = 1;
	int len = 64;

	prefix = net_if_ipv6_prefix_add(net_if_get_default(),
					&addr, len, lifetime);
	if (!prefix) {
		TC_ERROR("Cannot get prefix\n");
		return false;
	}

	net_if_ipv6_prefix_set_lf(prefix, false);
	net_if_ipv6_prefix_set_timer(prefix, lifetime);

	k_sleep((lifetime * 2) * MSEC_PER_SEC);

	prefix = net_if_ipv6_prefix_lookup(net_if_get_default(),
					   &addr, len);
	if (prefix) {
		TC_ERROR("Prefix %s/%d should have expired",
			 net_sprint_ipv6_addr(&addr), len);
		return false;
	}

	return true;
}

#if 0
/* This test has issues so disabling it temporarily */
static bool net_test_prefix_timeout_overflow(void)
{
	struct net_if_ipv6_prefix *prefix;
	struct in6_addr addr = { { { 0x20, 1, 0x0d, 0xb8, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 1 } } };
	int len = 64, lifetime = 0xfffffffe;

	prefix = net_if_ipv6_prefix_add(net_if_get_default(),
					&addr, len, lifetime);

	net_if_ipv6_prefix_set_lf(prefix, false);
	net_if_ipv6_prefix_set_timer(prefix, lifetime);

	if (!k_sem_take(&wait_data, (lifetime * 3/2) * MSEC_PER_SEC)) {
		TC_ERROR("Prefix %s/%d lock should still be there",
			 net_sprint_ipv6_addr(&addr), len);
		return false;
	}

	if (!net_if_ipv6_prefix_rm(net_if_get_default(), &addr, len)) {
		TC_ERROR("Prefix %s/%d should have been removed",
			 net_sprint_ipv6_addr(&addr), len);
		return false;
	}

	return true;
}
#endif

static bool net_test_ra_message(void)
{
	struct in6_addr addr = { { { 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0x2, 0x60,
				     0x97, 0xff, 0xfe, 0x07, 0x69, 0xea } } };
	struct in6_addr prefix = { { {0x3f, 0xfe, 0x05, 0x07, 0, 0, 0, 1,
				     0, 0, 0, 0, 0, 0, 0, 0 } } };

	if (!net_if_ipv6_prefix_lookup(net_if_get_default(), &prefix, 32)) {
		TC_ERROR("Prefix %s should be here\n",
			 net_sprint_ipv6_addr(&addr));
		return false;
	}

	if (!net_if_ipv6_router_lookup(net_if_get_default(), &addr)) {
		TC_ERROR("Router %s should be here\n",
			 net_sprint_ipv6_addr(&addr));
		return false;
	}

	return true;
}

static bool net_test_hbho_message(void)
{
	struct net_buf *buf, *frag;
	struct net_if *iface;
	uint16_t reserve;

	buf = net_nbuf_get_reserve_tx(0);

	NET_ASSERT_INFO(buf, "Out of TX buffers");

	iface = net_if_get_default();

	reserve = net_if_get_ll_reserve(iface, NULL);

	frag = net_nbuf_get_reserve_data(reserve);

	net_buf_frag_add(buf, frag);

	net_nbuf_set_ll_reserve(buf, reserve);
	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_family(buf, AF_INET6);
	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));

	net_nbuf_ll_clear(buf);

	memcpy(net_buf_add(frag, sizeof(ipv6_hbho)),
	       ipv6_hbho, sizeof(ipv6_hbho));

	if (net_recv_data(iface, buf) < 0) {
		TC_ERROR("Data receive for HBHO failed.");
		return false;
	}

	return true;
}

static bool net_test_change_ll_addr(void)
{
	uint8_t new_mac[] = { 00, 01, 02, 03, 04, 05 };
	struct net_linkaddr_storage *ll;
	struct net_linkaddr *ll_iface;
	struct net_buf *buf, *frag;
	struct in6_addr dst;
	struct net_if *iface;
	struct net_nbr *nbr;
	uint16_t reserve;
	uint32_t flags;
	int ret;

	net_ipv6_addr_create(&dst, 0xff02, 0, 0, 0, 0, 0, 0, 1);

	buf = net_nbuf_get_reserve_tx(0);

	NET_ASSERT_INFO(buf, "Out of TX buffers");

	iface = net_if_get_default();

	reserve = net_if_get_ll_reserve(iface, NULL);

	frag = net_nbuf_get_reserve_data(reserve);

	net_buf_frag_add(buf, frag);

	net_nbuf_set_ll_reserve(buf, reserve);
	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_family(buf, AF_INET6);
	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));

	flags = NET_ICMPV6_NA_FLAG_ROUTER |
		NET_ICMPV6_NA_FLAG_OVERRIDE;

	ret = net_ipv6_send_na(iface, &peer_addr, &dst,
			       &peer_addr, flags);
	if (ret < 0) {
		TC_ERROR("Cannot send NA 1\n");
		return false;
	}

	nbr = net_ipv6_nbr_lookup(iface, &peer_addr);
	ll = net_nbr_get_lladdr(nbr->idx);

	ll_iface = net_if_get_link_addr(iface);

	if (memcmp(ll->addr, ll_iface->addr, ll->len) != 0) {
		TC_ERROR("Wrong link address 1\n");
		return false;
	}

	/* As the net_ipv6_send_na() uses interface link address to
	 * greate tllao, change the interface ll address here.
	 */
	ll_iface->addr = new_mac;

	ret = net_ipv6_send_na(iface, &peer_addr, &dst,
			       &peer_addr, flags);
	if (ret < 0) {
		TC_ERROR("Cannot send NA 2\n");
		return false;
	}

	nbr = net_ipv6_nbr_lookup(iface, &peer_addr);
	ll = net_nbr_get_lladdr(nbr->idx);

	if (memcmp(ll->addr, ll_iface->addr, ll->len) != 0) {
		TC_ERROR("Wrong link address 2\n");
		return false;
	}

	return true;
}

static const struct {
	const char *name;
	bool (*func)(void);
} tests[] = {
	{ "test init", test_init },
	{ "IPv6 compare prefix", net_test_cmp_prefix },
	{ "IPv6 send NS mcast", net_test_send_ns_mcast },
	{ "IPv6 neighbor lookup fail", net_test_nbr_lookup_fail },
	{ "IPv6 send NS", net_test_send_ns },
	{ "IPv6 neighbor lookup ok", net_test_nbr_lookup_ok },
	{ "IPv6 send NS extra options", net_test_send_ns_extra_options },
	{ "IPv6 send NS no options", net_test_send_ns_no_options },
	{ "IPv6 handle RA message", net_test_ra_message },
	{ "IPv6 parse Hop-By-Hop Option", net_test_hbho_message },
	{ "IPv6 change ll address", net_test_change_ll_addr },
	{ "IPv6 prefix timeout", net_test_prefix_timeout },
	/*{ "IPv6 prefix timeout overflow", net_test_prefix_timeout_overflow },*/
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

		k_yield();
	}

	TC_END_REPORT(((pass != ARRAY_SIZE(tests)) ? TC_FAIL : TC_PASS));
}
