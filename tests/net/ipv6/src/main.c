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

#include <tc_util.h>

#include <net/net_pkt.h>
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
0x60, 0x00, 0x00, 0x00, 0x00, 0x40, 0x3a, 0xff,
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
	u8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_test_dev_init(struct device *dev)
{
	return 0;
}

static u8_t *net_test_get_mac(struct device *dev)
{
	struct net_test_ipv6 *context = dev->driver_data;

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

static struct net_pkt *prepare_ra_message(void)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_if *iface;

	iface = net_if_get_default();

	pkt = net_pkt_get_reserve_rx(net_if_get_ll_reserve(iface, NULL),
				      K_FOREVER);

	NET_ASSERT_INFO(pkt, "Out of RX packets");

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));

	net_pkt_ll_clear(pkt);

	memcpy(net_buf_add(frag, sizeof(icmpv6_ra)),
	       icmpv6_ra, sizeof(icmpv6_ra));

	return pkt;
}

#define NET_ICMP_HDR(pkt) ((struct net_icmp_hdr *)net_pkt_icmp_data(pkt))

static int tester_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_icmp_hdr *icmp;

	if (!pkt->frags) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	icmp = NET_ICMP_HDR(pkt);

	/* Reply with RA messge */
	if (icmp->type == NET_ICMPV6_RS) {
		net_pkt_unref(pkt);
		pkt = prepare_ra_message();
	}

	/* Feed this data back to us */
	if (net_recv_data(iface, pkt) < 0) {
		TC_ERROR("Data receive failed.");
		goto out;
	}

	return 0;

out:
	net_pkt_unref(pkt);
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
	struct net_if_addr *ifaddr = NULL, *ifaddr2;
	struct net_if_mcast_addr *maddr;
	struct net_if *iface = net_if_get_default();
	struct net_if *iface2 = NULL;
	int i;

	if (!iface) {
		TC_ERROR("Interface is NULL\n");
		return false;
	}

	/* We cannot use net_if_ipv6_addr_add() to add the address to
	 * network interface in this case as that would trigger DAD which
	 * we are not prepared to handle here. So instead add the address
	 * manually in this special case so that subsequent tests can
	 * pass.
	 */
	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (iface->ipv6.unicast[i].is_used) {
			continue;
		}

		ifaddr = &iface->ipv6.unicast[i];

		ifaddr->is_used = true;
		ifaddr->address.family = AF_INET6;
		ifaddr->addr_type = NET_ADDR_MANUAL;
		ifaddr->addr_state = NET_ADDR_PREFERRED;
		net_ipaddr_copy(&ifaddr->address.in6_addr, &my_addr);
		break;
	}

	ifaddr2 = net_if_ipv6_addr_lookup(&my_addr, &iface2);
	if (ifaddr2 != ifaddr) {
		TC_ERROR("Invalid ifaddr (%p vs %p)\n", ifaddr, ifaddr2);
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

	st = net_is_ipv6_prefix((u8_t *)&prefix1, (u8_t *)&prefix2, 64);
	if (!st) {
		TC_ERROR("Prefix /64  compare failed\n");
		return false;
	}

	st = net_is_ipv6_prefix((u8_t *)&prefix1, (u8_t *)&prefix2, 65);
	if (!st) {
		TC_ERROR("Prefix /65 compare failed\n");
		return false;
	}

	/* Set one extra bit in the other prefix for testing /65 */
	prefix1.s6_addr[8] = 0x80;

	st = net_is_ipv6_prefix((u8_t *)&prefix1, (u8_t *)&prefix2, 65);
	if (st) {
		TC_ERROR("Prefix /65 compare should have failed\n");
		return false;
	}

	/* Set two bits in prefix2, it is now /66 */
	prefix2.s6_addr[8] = 0xc0;

	st = net_is_ipv6_prefix((u8_t *)&prefix1, (u8_t *)&prefix2, 65);
	if (!st) {
		TC_ERROR("Prefix /65 compare failed\n");
		return false;
	}

	/* Set all remaining bits in prefix2, it is now /128 */
	memset(&prefix2.s6_addr[8], 0xff, 8);

	st = net_is_ipv6_prefix((u8_t *)&prefix1, (u8_t *)&prefix2, 65);
	if (!st) {
		TC_ERROR("Prefix /65 compare failed\n");
		return false;
	}

	/* Comparing /64 should be still ok */
	st = net_is_ipv6_prefix((u8_t *)&prefix1, (u8_t *)&prefix2, 64);
	if (!st) {
		TC_ERROR("Prefix /64 compare failed\n");
		return false;
	}

	/* But comparing /66 should should fail */
	st = net_is_ipv6_prefix((u8_t *)&prefix1, (u8_t *)&prefix2, 66);
	if (st) {
		TC_ERROR("Prefix /66 compare should have failed\n");
		return false;
	}

	return true;
}

static bool net_test_add_neighbor(void)
{
	struct net_nbr *nbr;
	struct net_linkaddr_storage llstorage;
	struct net_linkaddr lladdr;

	llstorage.addr[0] = 0x01;
	llstorage.addr[1] = 0x02;
	llstorage.addr[2] = 0x33;
	llstorage.addr[3] = 0x44;
	llstorage.addr[4] = 0x05;
	llstorage.addr[5] = 0x06;

	lladdr.len = 6;
	lladdr.addr = llstorage.addr;
	lladdr.type = NET_LINK_ETHERNET;

	nbr = net_ipv6_nbr_add(net_if_get_default(), &peer_addr, &lladdr,
			       false, NET_IPV6_NBR_STATE_REACHABLE);
	if (!nbr) {
		TC_ERROR("Cannot add peer %s to neighbor cache\n",
			 net_sprint_ipv6_addr(&peer_addr));
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
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_if *iface;

	iface = net_if_get_default();

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, NULL),
				      K_FOREVER);

	NET_ASSERT_INFO(pkt, "Out of TX packets");

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));

	net_pkt_ll_clear(pkt);

	memcpy(net_buf_add(frag, sizeof(icmpv6_ns_invalid)),
	       icmpv6_ns_invalid, sizeof(icmpv6_ns_invalid));

	if (net_recv_data(iface, pkt) < 0) {
		TC_ERROR("Data receive for invalid NS failed.");
		return false;
	}

	return true;
}

static bool net_test_send_ns_no_options(void)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_if *iface;

	iface = net_if_get_default();

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, NULL),
				      K_FOREVER);

	NET_ASSERT_INFO(pkt, "Out of TX packets");

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));

	net_pkt_ll_clear(pkt);

	memcpy(net_buf_add(frag, sizeof(icmpv6_ns_no_sllao)),
	       icmpv6_ns_no_sllao, sizeof(icmpv6_ns_no_sllao));

	if (net_recv_data(iface, pkt) < 0) {
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
	u32_t lifetime = 1;
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
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_if *iface;

	iface = net_if_get_default();

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, NULL),
				      K_FOREVER);

	NET_ASSERT_INFO(pkt, "Out of TX packets");

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));

	net_pkt_ll_clear(pkt);

	memcpy(net_buf_add(frag, sizeof(ipv6_hbho)),
	       ipv6_hbho, sizeof(ipv6_hbho));

	if (net_recv_data(iface, pkt) < 0) {
		TC_ERROR("Data receive for HBHO failed.");
		return false;
	}

	return true;
}

static bool net_test_change_ll_addr(void)
{
	u8_t new_mac[] = { 00, 01, 02, 03, 04, 05 };
	struct net_linkaddr_storage *ll;
	struct net_linkaddr *ll_iface;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct in6_addr dst;
	struct net_if *iface;
	struct net_nbr *nbr;
	u32_t flags;
	int ret;

	net_ipv6_addr_create(&dst, 0xff02, 0, 0, 0, 0, 0, 0, 1);

	iface = net_if_get_default();

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, &dst),
				      K_FOREVER);

	NET_ASSERT_INFO(pkt, "Out of TX packets");

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));

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
	{ "IPv6 neighbor lookup fail", net_test_nbr_lookup_fail },
	{ "IPv6 add neighbor", net_test_add_neighbor },
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
