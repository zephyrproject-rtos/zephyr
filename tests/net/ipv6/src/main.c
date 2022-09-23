/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_IPV6_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/linker/sections.h>
#include <zephyr/random/rand32.h>

#include <zephyr/ztest.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/udp.h>

#include "icmpv6.h"
#include "ipv6.h"
#include "route.h"

#include "udp_internal.h"

#define NET_LOG_ENABLED 1
#include "net_private.h"

#define TEST_NET_IF net_if_lookup_by_dev(DEVICE_GET(eth_ipv6_net))

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
	0x86, 0x00, 0x05, 0xd7, 0x40, 0x00, 0x07, 0x08,
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
/* Route info */
	0x18, 0x03, 0x30, 0x08, 0xff, 0xff, 0xff, 0xff,
	0x20, 0x01, 0x0d, 0xb0, 0x0f, 0xff, 0x00, 0x00,
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
	0x63, 0x04, 0x80, 0x1e, 0x01, 0x00,             /* ..c..... */
/* UDP header starts here (checksum is "fixed" in this example) */
	0xaa, 0xdc, 0xbf, 0xd7, 0x00, 0x2e, 0xa2, 0x55, /* ......M. */
/* User data starts here (38 bytes) */
	0x10, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, /* ........ */
	0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x02, /* ........ */
	0x00, 0x00, 0x03, 0x00, 0x00, 0x02, 0x00, 0x03, /* ........ */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xc9, /* ........ */
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00,             /* ...... */
};

static bool expecting_ra;
static uint32_t dad_time[3];
static bool test_failed;
static struct k_sem wait_data;
static bool recv_cb_called;
struct net_if_addr *ifaddr_record;

#define WAIT_TIME 250
#define WAIT_TIME_LONG MSEC_PER_SEC
#define SENDING 93244
#define MY_PORT 1969
#define PEER_PORT 16233

struct net_test_ipv6 {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_test_dev_init(const struct device *dev)
{
	return 0;
}

static uint8_t *net_test_get_mac(const struct device *dev)
{
	struct net_test_ipv6 *context = dev->data;

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
	uint8_t *mac = net_test_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

/**
 * @brief IPv6 handle RA message
 */
static void prepare_ra_message(struct net_pkt *pkt)
{
	struct net_eth_hdr hdr;

	net_buf_unref(pkt->buffer);
	pkt->buffer = NULL;

	net_pkt_alloc_buffer(pkt, sizeof(struct net_eth_hdr) +
			     sizeof(icmpv6_ra), AF_UNSPEC, K_NO_WAIT);
	net_pkt_cursor_init(pkt);

	hdr.type = htons(NET_ETH_PTYPE_IPV6);
	memset(&hdr.src, 0, sizeof(struct net_eth_addr));
	memcpy(&hdr.dst, net_pkt_iface(pkt)->if_dev->link_addr.addr,
	       sizeof(struct net_eth_addr));

	net_pkt_set_overwrite(pkt, false);

	net_pkt_write(pkt, &hdr, sizeof(struct net_eth_hdr));
	net_pkt_write(pkt, icmpv6_ra, sizeof(icmpv6_ra));

	net_pkt_cursor_init(pkt);
}

static struct net_icmp_hdr *get_icmp_hdr(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access, struct net_icmp_hdr);
	/* First frag is the ll header */
	struct net_buf *bak = pkt->buffer;
	struct net_pkt_cursor backup;
	struct net_icmp_hdr *hdr;

	pkt->buffer = bak->frags;

	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
			 net_pkt_ipv6_ext_len(pkt))) {
		hdr = NULL;
		goto out;
	}

	hdr = (struct net_icmp_hdr *)net_pkt_get_data(pkt, &icmp_access);

out:
	pkt->buffer = bak;

	net_pkt_cursor_restore(pkt, &backup);

	return hdr;
}


static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	struct net_icmp_hdr *icmp;

	if (!pkt->buffer) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	icmp = get_icmp_hdr(pkt);

	/* Reply with RA message */
	if (icmp->type == NET_ICMPV6_RS) {
		if (expecting_ra) {
			prepare_ra_message(pkt);
		} else {
			goto out;
		}
	}

	if (icmp->type == NET_ICMPV6_NS) {
		if (dad_time[0] == 0U) {
			dad_time[0] = k_uptime_get_32();
		} else if (dad_time[1] == 0U) {
			dad_time[1] = k_uptime_get_32();
		} else if (dad_time[2] == 0U) {
			dad_time[2] = k_uptime_get_32();
		}

		goto out;
	}

	/* Feed this data back to us */
	if (net_recv_data(net_pkt_iface(pkt),
			  net_pkt_clone(pkt, K_NO_WAIT)) < 0) {
		TC_ERROR("Data receive failed.");
		goto out;
	}

	return 0;

out:
	test_failed = true;

	return 0;
}

/* Ethernet interface (interface under test) */
struct net_test_ipv6 net_test_data;

static const struct ethernet_api net_test_if_api = {
	.iface_api.init = net_test_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER ETHERNET_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(ETHERNET_L2)

NET_DEVICE_INIT(eth_ipv6_net, "eth_ipv6_net",
		net_test_dev_init, NULL, &net_test_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_test_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE,
		NET_ETH_MTU);

/* dummy interface for multi-interface tests */
static int dummy_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

struct net_test_ipv6 net_dummy_data;

static const struct dummy_api net_dummy_if_api = {
		.iface_api.init = net_test_iface_init,
		.send = dummy_send,
};

#define _DUMMY_L2_LAYER DUMMY_L2
#define _DUMMY_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(eth_ipv6_net_dummy, "eth_ipv6_net_dummy",
		net_test_dev_init, NULL, &net_dummy_data,
		NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_dummy_if_api, _DUMMY_L2_LAYER, _DUMMY_L2_CTX_TYPE,
		127);

/**
 * @brief IPv6 add neighbor
 */
static void add_neighbor(void)
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

	lladdr.len = 6U;
	lladdr.addr = llstorage.addr;
	lladdr.type = NET_LINK_ETHERNET;

	nbr = net_ipv6_nbr_add(TEST_NET_IF, &peer_addr, &lladdr,
			       false, NET_IPV6_NBR_STATE_REACHABLE);
	zassert_not_null(nbr, "Cannot add peer %s to neighbor cache\n",
			 net_sprint_ipv6_addr(&peer_addr));
}

static void rm_neighbor(void)
{
	struct net_linkaddr_storage llstorage;
	struct net_linkaddr lladdr;

	llstorage.addr[0] = 0x01;
	llstorage.addr[1] = 0x02;
	llstorage.addr[2] = 0x33;
	llstorage.addr[3] = 0x44;
	llstorage.addr[4] = 0x05;
	llstorage.addr[5] = 0x06;

	lladdr.len = 6U;
	lladdr.addr = llstorage.addr;
	lladdr.type = NET_LINK_ETHERNET;

	net_ipv6_nbr_rm(TEST_NET_IF, &peer_addr);
}

/**
 * @brief IPv6 add more than max neighbors
 */
static void add_max_neighbors(void)
{
	struct in6_addr dst_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					 0, 0, 0, 0, 0, 0, 0, 0x3 } } };
	struct net_nbr *nbr;
	struct net_linkaddr_storage llstorage;
	struct net_linkaddr lladdr;
	uint8_t i;

	llstorage.addr[0] = 0x01;
	llstorage.addr[1] = 0x02;
	llstorage.addr[2] = 0x33;
	llstorage.addr[3] = 0x44;
	llstorage.addr[4] = 0x05;
	llstorage.addr[5] = 0x07;

	lladdr.len = 6U;
	lladdr.addr = llstorage.addr;
	lladdr.type = NET_LINK_ETHERNET;

	for (i = 0U; i < CONFIG_NET_IPV6_MAX_NEIGHBORS + 1; i++) {
		llstorage.addr[5] += i;
		dst_addr.s6_addr[15] += i;
		nbr = net_ipv6_nbr_add(TEST_NET_IF, &dst_addr,
				       &lladdr, false,
				       NET_IPV6_NBR_STATE_STALE);
		zassert_not_null(nbr, "Cannot add peer %s to neighbor cache\n",
				 net_sprint_ipv6_addr(&dst_addr));
	}
}

static void rm_max_neighbors(void)
{
	struct in6_addr dst_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					 0, 0, 0, 0, 0, 0, 0, 0x3 } } };
	struct net_linkaddr_storage llstorage;
	struct net_linkaddr lladdr;
	uint8_t i;

	llstorage.addr[0] = 0x01;
	llstorage.addr[1] = 0x02;
	llstorage.addr[2] = 0x33;
	llstorage.addr[3] = 0x44;
	llstorage.addr[4] = 0x05;
	llstorage.addr[5] = 0x07;

	lladdr.len = 6U;
	lladdr.addr = llstorage.addr;
	lladdr.type = NET_LINK_ETHERNET;

	for (i = 0U; i < CONFIG_NET_IPV6_MAX_NEIGHBORS + 1; i++) {
		llstorage.addr[5] += i;
		dst_addr.s6_addr[15] += i;
		net_ipv6_nbr_rm(TEST_NET_IF, &dst_addr);
	}
}

/**
 * @brief IPv6 neighbor lookup fail
 */
static void nbr_lookup_fail(void)
{
	struct net_nbr *nbr;

	nbr = net_ipv6_nbr_lookup(TEST_NET_IF, &peer_addr);
	zassert_is_null(nbr, "Neighbor %s found in cache\n",
			net_sprint_ipv6_addr(&peer_addr));

}

/**
 * @brief IPv6 neighbor lookup ok
 */
static void nbr_lookup_ok(void)
{
	struct net_nbr *nbr;

	nbr = net_ipv6_nbr_lookup(TEST_NET_IF, &peer_addr);
	zassert_not_null(nbr, "Neighbor %s not found in cache\n",
			 net_sprint_ipv6_addr(&peer_addr));
}

/**
 * @brief IPv6 setup
 */
static void *ipv6_setup(void)
{
	struct net_if_addr *ifaddr = NULL, *ifaddr2;
	struct net_if_mcast_addr *maddr;
	struct net_if *iface = TEST_NET_IF;
	struct net_if *iface2 = NULL;
	struct net_if_ipv6 *ipv6;
	int i;

	zassert_not_null(iface, "Interface is NULL");

	/* We cannot use net_if_ipv6_addr_add() to add the address to
	 * network interface in this case as that would trigger DAD which
	 * we are not prepared to handle here. So instead add the address
	 * manually in this special case so that subsequent tests can
	 * pass.
	 */
	zassert_false(net_if_config_ipv6_get(iface, &ipv6) < 0,
			"IPv6 config is not valid");

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (iface->config.ip.ipv6->unicast[i].is_used) {
			continue;
		}

		ifaddr = &iface->config.ip.ipv6->unicast[i];

		ifaddr->is_used = true;
		ifaddr->address.family = AF_INET6;
		ifaddr->addr_type = NET_ADDR_MANUAL;
		ifaddr->addr_state = NET_ADDR_PREFERRED;
		ifaddr_record = ifaddr;
		net_ipaddr_copy(&ifaddr->address.in6_addr, &my_addr);
		break;
	}

	ifaddr2 = net_if_ipv6_addr_lookup(&my_addr, &iface2);
	zassert_true(ifaddr2 == ifaddr, "Invalid ifaddr (%p vs %p)\n", ifaddr, ifaddr2);

	net_ipv6_addr_create(&mcast_addr, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);

	maddr = net_if_ipv6_maddr_add(iface, &mcast_addr);
	zassert_not_null(maddr, "Cannot add multicast IPv6 address %s\n",
			 net_sprint_ipv6_addr(&mcast_addr));

	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);

	nbr_lookup_fail();
	add_neighbor();
	add_max_neighbors();
	nbr_lookup_ok();
	k_sleep(K_MSEC(50));

	return NULL;
}


static void ipv6_teardown(void *dummy)
{
	ARG_UNUSED(dummy);
	struct net_if *iface = TEST_NET_IF;

	rm_max_neighbors();
	rm_neighbor();

	net_ipv6_addr_create(&mcast_addr, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);
	net_if_ipv6_maddr_rm(iface, &mcast_addr);
	ifaddr_record->is_used = false;
}

/**
 * @brief IPv6 compare prefix
 *
 */
ZTEST(net_ipv6, test_cmp_prefix)
{
	bool st;

	struct in6_addr prefix1 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	struct in6_addr prefix2 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x2 } } };

	st = net_ipv6_is_prefix((uint8_t *)&prefix1, (uint8_t *)&prefix2, 64);
	zassert_true(st, "Prefix /64  compare failed");

	st = net_ipv6_is_prefix((uint8_t *)&prefix1, (uint8_t *)&prefix2, 65);
	zassert_true(st, "Prefix /65 compare failed");

	/* Set one extra bit in the other prefix for testing /65 */
	prefix1.s6_addr[8] = 0x80;

	st = net_ipv6_is_prefix((uint8_t *)&prefix1, (uint8_t *)&prefix2, 65);
	zassert_false(st, "Prefix /65 compare should have failed");

	/* Set two bits in prefix2, it is now /66 */
	prefix2.s6_addr[8] = 0xc0;

	st = net_ipv6_is_prefix((uint8_t *)&prefix1, (uint8_t *)&prefix2, 65);
	zassert_true(st, "Prefix /65 compare failed");

	/* Set all remaining bits in prefix2, it is now /128 */
	(void)memset(&prefix2.s6_addr[8], 0xff, 8);

	st = net_ipv6_is_prefix((uint8_t *)&prefix1, (uint8_t *)&prefix2, 65);
	zassert_true(st, "Prefix /65 compare failed");

	/* Comparing /64 should be still ok */
	st = net_ipv6_is_prefix((uint8_t *)&prefix1, (uint8_t *)&prefix2, 64);
	zassert_true(st, "Prefix /64 compare failed");

	/* But comparing /66 should should fail */
	st = net_ipv6_is_prefix((uint8_t *)&prefix1, (uint8_t *)&prefix2, 66);
	zassert_false(st, "Prefix /66 compare should have failed");

}

/**
 * @brief IPv6 send NS extra options
 */
ZTEST(net_ipv6, test_send_ns_extra_options)
{
	struct net_pkt *pkt;
	struct net_if *iface;

	iface = TEST_NET_IF;

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(icmpv6_ns_invalid),
					AF_UNSPEC, 0, K_FOREVER);

	NET_ASSERT(pkt, "Out of TX packets");

	net_pkt_write(pkt, icmpv6_ns_invalid, sizeof(icmpv6_ns_invalid));
	net_pkt_lladdr_clear(pkt);

	zassert_false((net_recv_data(iface, pkt) < 0),
		      "Data receive for invalid NS failed.");

}

/**
 * @brief IPv6 send NS no option
 */
ZTEST(net_ipv6, test_send_ns_no_options)
{
	struct net_pkt *pkt;
	struct net_if *iface;

	iface = TEST_NET_IF;

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(icmpv6_ns_no_sllao),
					AF_UNSPEC, 0, K_FOREVER);

	NET_ASSERT(pkt, "Out of TX packets");

	net_pkt_write(pkt, icmpv6_ns_no_sllao, sizeof(icmpv6_ns_no_sllao));
	net_pkt_lladdr_clear(pkt);

	zassert_false((net_recv_data(iface, pkt) < 0),
		      "Data receive for invalid NS failed.");
}

/**
 * @brief IPv6 prefix timeout
 */
ZTEST(net_ipv6, test_prefix_timeout)
{
	struct net_if_ipv6_prefix *prefix;
	struct in6_addr addr = { { { 0x20, 1, 0x0d, 0xb8, 42, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0 } } };
	uint32_t lifetime = 1U;
	int len = 64;

	prefix = net_if_ipv6_prefix_add(TEST_NET_IF, &addr, len, lifetime);
	zassert_not_null(prefix, "Cannot get prefix");

	net_if_ipv6_prefix_set_lf(prefix, false);
	net_if_ipv6_prefix_set_timer(prefix, lifetime);

	k_sleep(K_SECONDS(lifetime * 2U));

	prefix = net_if_ipv6_prefix_lookup(TEST_NET_IF, &addr, len);
	zassert_is_null(prefix, "Prefix %s/%d should have expired",
			net_sprint_ipv6_addr(&addr), len);
}

ZTEST(net_ipv6, test_prefix_timeout_long)
{
	struct net_if_ipv6_prefix *ifprefix;
	struct in6_addr prefix = { { { 0x20, 1, 0x0d, 0xb8, 43, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0 } } };
	uint32_t lifetime = 0xfffffffe;
	int len = 64;
	uint64_t remaining;
	int ret;

	ifprefix = net_if_ipv6_prefix_add(TEST_NET_IF, &prefix, len, lifetime);

	net_if_ipv6_prefix_set_lf(ifprefix, false);
	net_if_ipv6_prefix_set_timer(ifprefix, lifetime);

	zassert_equal(ifprefix->lifetime.wrap_counter, 1999,
		      "Wrap counter wrong (%d)",
		      ifprefix->lifetime.wrap_counter);
	remaining = MSEC_PER_SEC * (uint64_t)lifetime -
		NET_TIMEOUT_MAX_VALUE * (uint64_t)ifprefix->lifetime.wrap_counter;

	zassert_equal(remaining, ifprefix->lifetime.timer_timeout,
		     "Remaining time wrong (%llu vs %d)", remaining,
		      ifprefix->lifetime.timer_timeout);

	ret = net_if_ipv6_prefix_rm(TEST_NET_IF, &prefix, len);
	zassert_equal(ret, true, "Prefix %s/%d should have been removed",
		      net_sprint_ipv6_addr(&prefix), len);
}

static void rs_message(void)
{
	struct net_if *iface;
	int ret;

	iface = TEST_NET_IF;

	expecting_ra = true;

	ret = net_ipv6_send_rs(iface);

	zassert_equal(ret, 0, "RS sending failed (%d)", ret);

	k_yield();
}

static void ra_message(void)
{
	struct in6_addr addr = { { { 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0x2, 0x60,
				     0x97, 0xff, 0xfe, 0x07, 0x69, 0xea } } };
	struct in6_addr prefix = { { { 0x3f, 0xfe, 0x05, 0x07, 0, 0, 0, 1,
				       0, 0, 0, 0, 0, 0, 0, 0 } } };
	struct in6_addr route_prefix = { { { 0x20, 0x01, 0x0d, 0xb0, 0x0f, 0xff } } };
	struct net_route_entry *route;

	/* We received RA message earlier, make sure that the information
	 * in that message is placed to proper prefix and lookup info.
	 */

	expecting_ra = false;

	zassert_false(!net_if_ipv6_prefix_lookup(TEST_NET_IF, &prefix, 32),
		      "Prefix %s should be here\n",
		      net_sprint_ipv6_addr(&prefix));

	zassert_false(!net_if_ipv6_router_lookup(TEST_NET_IF, &addr),
		      "Router %s should be here\n",
		      net_sprint_ipv6_addr(&addr));

	/* Check if route was added correctly. */
	route = net_route_lookup(TEST_NET_IF, &route_prefix);
	zassert_not_null(route, "Route not found");
	zassert_equal(route->prefix_len, 48, "Wrong prefix length set");
	zassert_mem_equal(&route->addr, &route_prefix, sizeof(route_prefix),
			  "Wrong prefix set");
	zassert_true(route->is_infinite, "Wrong lifetime set");
	zassert_equal(route->preference, NET_ROUTE_PREFERENCE_HIGH,
		      "Wrong preference set");
}

ZTEST(net_ipv6, test_rs_ra_message)
{
	rs_message();
	ra_message();
}

/**
 * @brief IPv6 parse Hop-By-Hop Option
 */
ZTEST(net_ipv6, test_hbho_message)
{
	struct net_pkt *pkt;
	struct net_if *iface;

	iface = TEST_NET_IF;

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(ipv6_hbho),
					AF_UNSPEC, 0, K_FOREVER);

	NET_ASSERT(pkt, "Out of TX packets");

	net_pkt_write(pkt, ipv6_hbho, sizeof(ipv6_hbho));
	net_pkt_lladdr_clear(pkt);

	zassert_false(net_recv_data(iface, pkt) < 0,
		      "Data receive for HBHO failed.");
}

/* IPv6 hop-by-hop option in the message HBHO (72 Bytes) */
static const unsigned char ipv6_hbho_1[] = {
/* IPv6 header starts here */
0x60, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x40,
0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x08, 0xc0, 0xde, 0xff, 0xfe, 0x9b, 0xb4, 0x47,
0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
/* Hop-by-hop option starts here */
0x11, 0x08,
/* Padding */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* UDP header starts here (8 bytes) */
0x4e, 0x20, 0x10, 0x92, 0x00, 0x30, 0xa1, 0xc5,
/* User data starts here (40 bytes) */
0x30, 0x26, 0x02, 0x01, 0x00, 0x04, 0x06, 0x70,
0x75, 0x62, 0x6c, 0x69, 0x63, 0xa0, 0x19, 0x02,
0x01, 0x00, 0x02, 0x01, 0x00, 0x02, 0x01, 0x00,
0x30, 0x0e, 0x30, 0x0c, 0x06, 0x08, 0x2b, 0x06,
0x01, 0x02, 0x01, 0x01, 0x05, 0x00, 0x05, 0x00 };

/**
 * @brief IPv6 parse Hop-By-Hop Option
 */
ZTEST(net_ipv6, test_hbho_message_1)
{
	struct net_pkt *pkt;
	struct net_if *iface;

	iface = TEST_NET_IF;

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(ipv6_hbho_1),
					AF_UNSPEC, 0, K_FOREVER);

	NET_ASSERT(pkt, "Out of TX packets");

	net_pkt_write(pkt, ipv6_hbho_1, sizeof(ipv6_hbho_1));

	net_pkt_lladdr_clear(pkt);

	zassert_false(net_recv_data(iface, pkt) < 0,
		      "Data receive for HBHO failed.");

	/* Verify IPv6 Ext hdr length */
	zassert_false(net_pkt_ipv6_ext_len(pkt) == 72U,
		      "IPv6 mismatch ext hdr length");
}

/* IPv6 hop-by-hop option in the message HBHO (104 Bytes) */
static const unsigned char ipv6_hbho_2[] = {
/* IPv6 header starts here */
0x60, 0x00, 0x00, 0x00, 0x00, 0x98, 0x00, 0x40,
0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x08, 0xc0, 0xde, 0xff, 0xfe, 0x9b, 0xb4, 0x47,
0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
/* Hop-by-hop option starts here */
0x11, 0x0c,
/* padding */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x01, 0x04, 0x00, 0x00, 0x00, 0x00,
/* udp header starts here (8 bytes) */
0x4e, 0x20, 0x10, 0x92, 0x00, 0x30, 0xa1, 0xc5,
/* User data starts here (40 bytes) */
0x30, 0x26, 0x02, 0x01, 0x00, 0x04, 0x06, 0x70,
0x75, 0x62, 0x6c, 0x69, 0x63, 0xa0, 0x19, 0x02,
0x01, 0x00, 0x02, 0x01, 0x00, 0x02, 0x01, 0x00,
0x30, 0x0e, 0x30, 0x0c, 0x06, 0x08, 0x2b, 0x06,
0x01, 0x02, 0x01, 0x01, 0x05, 0x00, 0x05, 0x00 };

/**
 * @brief IPv6 parse Hop-By-Hop Option
 */
ZTEST(net_ipv6, test_hbho_message_2)
{
	struct net_pkt *pkt;
	struct net_if *iface;

	iface = TEST_NET_IF;

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(ipv6_hbho_2),
					AF_UNSPEC, 0, K_FOREVER);

	NET_ASSERT(pkt, "Out of TX packets");


	net_pkt_write(pkt, ipv6_hbho_2, sizeof(ipv6_hbho_2));
	net_pkt_lladdr_clear(pkt);

	zassert_false(net_recv_data(iface, pkt) < 0,
		      "Data receive for HBHO failed.");

	/* Verify IPv6 Ext hdr length */
	zassert_false(net_pkt_ipv6_ext_len(pkt) == 104U,
		      "IPv6 mismatch ext hdr length");
}

/* IPv6 hop-by-hop option in the message HBHO (920 bytes) */
static const unsigned char ipv6_hbho_3[] = {
/* IPv6 header starts here */
0x60, 0x00, 0x00, 0x00, 0x03, 0xc8, 0x00, 0x40,
0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x08, 0xc0, 0xde, 0xff, 0xfe, 0x9b, 0xb4, 0x47,
0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
/* Hop-by-hop option starts here */
0x11, 0x72,
/* padding */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x01, 0x04, 0x00, 0x00, 0x00, 0x00,
/* udp header starts here (8 bytes) */
0x4e, 0x20, 0x10, 0x92, 0x00, 0x30, 0xa1, 0xc5,
/* User data starts here (40 bytes) */
0x30, 0x26, 0x02, 0x01, 0x00, 0x04, 0x06, 0x70,
0x75, 0x62, 0x6c, 0x69, 0x63, 0xa0, 0x19, 0x02,
0x01, 0x00, 0x02, 0x01, 0x00, 0x02, 0x01, 0x00,
0x30, 0x0e, 0x30, 0x0c, 0x06, 0x08, 0x2b, 0x06,
0x01, 0x02, 0x01, 0x01, 0x05, 0x00, 0x05, 0x00
};

/**
 * @brief IPv6 parse Hop-By-Hop Option
 */
ZTEST(net_ipv6, test_hbho_message_3)
{
	struct net_pkt *pkt;
	struct net_if *iface;

	iface = TEST_NET_IF;

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(ipv6_hbho_3),
					AF_UNSPEC, 0, K_FOREVER);

	NET_ASSERT(pkt, "Out of TX packets");

	net_pkt_write(pkt, ipv6_hbho_3, sizeof(ipv6_hbho_3));
	net_pkt_lladdr_clear(pkt);

	zassert_false(net_recv_data(iface, pkt) < 0,
		      "Data receive for HBHO failed.");

	/* Verify IPv6 Ext hdr length */
	zassert_false(net_pkt_ipv6_ext_len(pkt) == 920U,
		      "IPv6 mismatch ext hdr length");
}

#define FIFTY_DAYS (60 * 60 * 24 * 50)

/* Implemented in subsys/net/ip/net_if.c */
extern void net_address_lifetime_timeout(void);

ZTEST(net_ipv6, test_address_lifetime)
{
	struct in6_addr addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0x20, 0x1 } } };
	struct net_if *iface = TEST_NET_IF;
	uint32_t vlifetime = 0xffff;
	uint64_t timeout = (uint64_t)vlifetime * MSEC_PER_SEC;
	struct net_if_addr *ifaddr;
	uint64_t remaining;
	bool ret;

	ifaddr = net_if_ipv6_addr_add(iface, &addr, NET_ADDR_AUTOCONF,
				      vlifetime);
	zassert_not_null(ifaddr, "Address with lifetime cannot be added");

	/* Make sure DAD gets some time to run */
	k_sleep(K_MSEC(200));

	/* Then check that the timeout values in net_if_addr are set correctly.
	 * Start first with smaller timeout values.
	 */
	zassert_equal(ifaddr->lifetime.timer_timeout, timeout,
		      "Timer timeout set wrong (%d vs %llu)",
		      ifaddr->lifetime.timer_timeout, timeout);
	zassert_equal(ifaddr->lifetime.wrap_counter, 0,
		      "Wrap counter wrong (%d)", ifaddr->lifetime.wrap_counter);

	/* Then update the lifetime and check that timeout values are correct
	 */
	vlifetime = FIFTY_DAYS;
	net_if_ipv6_addr_update_lifetime(ifaddr, vlifetime);

	zassert_equal(ifaddr->lifetime.wrap_counter, 2,
		      "Wrap counter wrong (%d)", ifaddr->lifetime.wrap_counter);
	remaining = MSEC_PER_SEC * (uint64_t)vlifetime -
		NET_TIMEOUT_MAX_VALUE * (uint64_t)ifaddr->lifetime.wrap_counter;

	zassert_equal(remaining, ifaddr->lifetime.timer_timeout,
		     "Remaining time wrong (%llu vs %d)", remaining,
		      ifaddr->lifetime.timer_timeout);

	/* The address should not expire */
	net_address_lifetime_timeout();

	zassert_equal(ifaddr->lifetime.wrap_counter, 2,
		      "Wrap counter wrong (%d)", ifaddr->lifetime.wrap_counter);

	ifaddr->lifetime.timer_timeout = 10;
	ifaddr->lifetime.timer_start = k_uptime_get_32() - 10;
	ifaddr->lifetime.wrap_counter = 0;

	net_address_lifetime_timeout();

	/* The address should be expired now */
	zassert_equal(ifaddr->lifetime.timer_timeout, 0,
		      "Timer timeout set wrong (%llu vs %llu)",
		      ifaddr->lifetime.timer_timeout, 0);
	zassert_equal(ifaddr->lifetime.wrap_counter, 0,
		      "Wrap counter wrong (%d)", ifaddr->lifetime.wrap_counter);

	ret = net_if_ipv6_addr_rm(iface, &addr);
	zassert_true(ret, "Address with lifetime cannot be removed");
}

/**
 * @brief IPv6 change ll address
 */
ZTEST(net_ipv6, test_change_ll_addr)
{
	static uint8_t new_mac[] = { 00, 01, 02, 03, 04, 05 };
	struct net_linkaddr_storage *ll;
	struct net_linkaddr *ll_iface;
	struct in6_addr dst;
	struct net_if *iface;
	struct net_nbr *nbr;
	uint32_t flags;
	int ret;

	net_ipv6_addr_create(&dst, 0xff02, 0, 0, 0, 0, 0, 0, 1);

	iface = TEST_NET_IF;

	flags = NET_ICMPV6_NA_FLAG_ROUTER |
		NET_ICMPV6_NA_FLAG_OVERRIDE;

	ret = net_ipv6_send_na(iface, &peer_addr, &dst,
			       &peer_addr, flags);
	zassert_false(ret < 0, "Cannot send NA 1");

	nbr = net_ipv6_nbr_lookup(iface, &peer_addr);
	zassert_not_null(nbr, "Neighbor %s not found in cache\n",
			 net_sprint_ipv6_addr(&peer_addr));
	ll = net_nbr_get_lladdr(nbr->idx);

	ll_iface = net_if_get_link_addr(iface);

	zassert_true(memcmp(ll->addr, ll_iface->addr, ll->len) != 0,
		     "Wrong link address 1");

	/* As the net_ipv6_send_na() uses interface link address to
	 * greate tllao, change the interface ll address here.
	 */
	ll_iface->addr = new_mac;

	ret = net_ipv6_send_na(iface, &peer_addr, &dst,
			       &peer_addr, flags);
	zassert_false(ret < 0, "Cannot send NA 2");

	nbr = net_ipv6_nbr_lookup(iface, &peer_addr);
	zassert_not_null(nbr, "Neighbor %s not found in cache\n",
			 net_sprint_ipv6_addr(&peer_addr));
	ll = net_nbr_get_lladdr(nbr->idx);

	zassert_true(memcmp(ll->addr, ll_iface->addr, ll->len) != 0,
		     "Wrong link address 2");

	ll_iface->addr[0] = 0x00;
	ll_iface->addr[1] = 0x00;
	ll_iface->addr[2] = 0x5E;
	ll_iface->addr[3] = 0x00;
	ll_iface->addr[4] = 0x53;
	ll_iface->addr[5] = sys_rand32_get();
}

ZTEST(net_ipv6, test_dad_timeout)
{
#if defined(CONFIG_NET_IPV6_DAD)
	struct in6_addr addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				      0, 0, 0, 0, 0, 0, 0x99, 0x1 } } };
	struct in6_addr addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				      0, 0, 0, 0, 0, 0, 0x99, 0x2 } } };
	struct in6_addr addr3 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				      0, 0, 0, 0, 0, 0, 0x99, 0x3 } } };
	struct net_if *iface = TEST_NET_IF;

	struct net_if_addr *ifaddr;

	dad_time[0] = dad_time[1] = dad_time[2] = 0U;

	ifaddr = net_if_ipv6_addr_add(iface, &addr1, NET_ADDR_AUTOCONF, 0xffff);
	zassert_not_null(ifaddr, "Address 1 cannot be added");

	k_sleep(K_MSEC(10));

	ifaddr = net_if_ipv6_addr_add(iface, &addr2, NET_ADDR_AUTOCONF, 0xffff);
	zassert_not_null(ifaddr, "Address 2 cannot be added");

	k_sleep(K_MSEC(10));

	ifaddr = net_if_ipv6_addr_add(iface, &addr3, NET_ADDR_AUTOCONF, 0xffff);
	zassert_not_null(ifaddr, "Address 3 cannot be added");

	k_sleep(K_MSEC(200));

	/* Check we have received three DAD queries */
	zassert_true((dad_time[0] != 0U) && (dad_time[1] != 0U) &&
			(dad_time[2] != 0U), "Did not get DAD reply");

	net_if_ipv6_addr_rm(iface, &addr1);
	net_if_ipv6_addr_rm(iface, &addr2);
	net_if_ipv6_addr_rm(iface, &addr3);
#endif
}

#define NET_UDP_HDR(pkt)  ((struct net_udp_hdr *)(net_udp_get_hdr(pkt, NULL)))

static struct net_pkt *setup_ipv6_udp(struct net_if *iface,
				      struct in6_addr *local_addr,
				      struct in6_addr *remote_addr,
				      uint16_t local_port,
				      uint16_t remote_port)
{
	static const char payload[] = "foobar";
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(iface, strlen(payload),
					AF_INET6, IPPROTO_UDP, K_FOREVER);
	if (!pkt) {
		return NULL;
	}

	if (net_ipv6_create(pkt, local_addr, remote_addr)) {
		printk("Cannot create IPv6  pkt %p", pkt);
		zassert_true(0, "exiting");
	}

	if (net_udp_create(pkt, htons(local_port), htons(remote_port))) {
		printk("Cannot create IPv6  pkt %p", pkt);
		zassert_true(0, "exiting");
	}

	if (net_pkt_write(pkt, (uint8_t *)payload, strlen(payload))) {
		printk("Cannot write IPv6 ext header pkt %p", pkt);
		zassert_true(0, "exiting");
	}

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_UDP);
	net_pkt_cursor_init(pkt);

	return pkt;
}

static enum net_verdict recv_msg(struct in6_addr *src, struct in6_addr *dst)
{
	struct net_pkt *pkt;
	struct net_if *iface;

	iface = TEST_NET_IF;

	pkt = setup_ipv6_udp(iface, src, dst, 4242, 4321);

	/* We by-pass the normal packet receiving flow in this case in order
	 * to simplify the testing.
	 */
	return net_ipv6_input(pkt, false);
}

static int send_msg(struct in6_addr *src, struct in6_addr *dst)
{
	struct net_pkt *pkt;
	struct net_if *iface;

	iface = TEST_NET_IF;

	pkt = setup_ipv6_udp(iface, src, dst, 4242, 4321);

	return net_send_data(pkt);
}

ZTEST(net_ipv6, test_src_localaddr_recv)
{
	struct in6_addr localaddr = { { { 0, 0, 0, 0, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	struct in6_addr addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	enum net_verdict verdict;

	verdict = recv_msg(&localaddr, &addr);
	zassert_equal(verdict, NET_DROP,
		      "Local address packet was not dropped");
}

ZTEST(net_ipv6, test_dst_localaddr_recv)
{
	struct in6_addr localaddr = { { { 0, 0, 0, 0, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	struct in6_addr addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	enum net_verdict verdict;

	verdict = recv_msg(&addr, &localaddr);
	zassert_equal(verdict, NET_DROP,
		      "Local address packet was not dropped");
}

ZTEST(net_ipv6, test_dst_iface_scope_mcast_recv)
{
	struct in6_addr mcast_iface = { { { 0xff, 0x01, 0, 0, 0, 0, 0, 0,
					    0, 0, 0, 0, 0, 0, 0, 0 } } };
	struct in6_addr addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	enum net_verdict verdict;

	verdict = recv_msg(&addr, &mcast_iface);
	zassert_equal(verdict, NET_DROP,
		      "Interface scope multicast packet was not dropped");
}

ZTEST(net_ipv6, test_dst_zero_scope_mcast_recv)
{
	struct in6_addr mcast_zero = { { { 0xff, 0x00, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0 } } };
	struct in6_addr addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	enum net_verdict verdict;

	verdict = recv_msg(&addr, &mcast_zero);
	zassert_equal(verdict, NET_DROP,
		      "Zero scope multicast packet was not dropped");
}

ZTEST(net_ipv6, test_dst_site_scope_mcast_recv_drop)
{
	struct in6_addr mcast_site = { { { 0xff, 0x05, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0 } } };
	struct in6_addr addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	enum net_verdict verdict;

	verdict = recv_msg(&addr, &mcast_site);
	zassert_equal(verdict, NET_DROP,
		      "Site scope multicast packet was not dropped");
}

static void net_ctx_create(struct net_context **ctx)
{
	int ret;

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, ctx);
	zassert_equal(ret, 0,
		      "Context create IPv6 UDP test failed");
}

static void net_ctx_bind_mcast(struct net_context *ctx, struct in6_addr *maddr)
{
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(4321),
		.sin6_addr = { { { 0 } } },
	};
	int ret;

	net_ipaddr_copy(&addr.sin6_addr, maddr);

	ret = net_context_bind(ctx, (struct sockaddr *)&addr,
			       sizeof(struct sockaddr_in6));
	zassert_equal(ret, 0, "Context bind test failed (%d)", ret);
}

static void net_ctx_listen(struct net_context *ctx)
{
	zassert_true(net_context_listen(ctx, 0),
		     "Context listen IPv6 UDP test failed");
}

static void recv_cb(struct net_context *context,
		    struct net_pkt *pkt,
		    union net_ip_header *ip_hdr,
		    union net_proto_header *proto_hdr,
		    int status,
		    void *user_data)
{
	ARG_UNUSED(context);
	ARG_UNUSED(pkt);
	ARG_UNUSED(ip_hdr);
	ARG_UNUSED(proto_hdr);
	ARG_UNUSED(status);
	ARG_UNUSED(user_data);

	recv_cb_called = true;

	k_sem_give(&wait_data);
}

static void net_ctx_recv(struct net_context *ctx)
{
	int ret;

	ret = net_context_recv(ctx, recv_cb, K_NO_WAIT, NULL);
	zassert_equal(ret, 0, "Context recv IPv6 UDP failed");
}

static void join_group(struct in6_addr *mcast_addr)
{
	int ret;

	ret = net_ipv6_mld_join(TEST_NET_IF, mcast_addr);
	zassert_equal(ret, 0, "Cannot join IPv6 multicast group");
}

static void leave_group(struct in6_addr *mcast_addr)
{
	int ret;

	ret = net_ipv6_mld_leave(TEST_NET_IF, mcast_addr);
	zassert_equal(ret, 0, "Cannot leave IPv6 multicast group");
}

ZTEST(net_ipv6, test_dst_site_scope_mcast_recv_ok)
{
	struct in6_addr mcast_all_dhcp = { { { 0xff, 0x05, 0, 0, 0, 0, 0, 0,
					    0, 0, 0, 0x01, 0, 0, 0, 0x03 } } };
	struct in6_addr addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	enum net_verdict verdict;
	struct net_context *ctx;

	/* The packet will be dropped unless we have a listener and joined the
	 * group.
	 */
	join_group(&mcast_all_dhcp);

	net_ctx_create(&ctx);
	net_ctx_bind_mcast(ctx, &mcast_all_dhcp);
	net_ctx_listen(ctx);
	net_ctx_recv(ctx);

	verdict = recv_msg(&addr, &mcast_all_dhcp);
	zassert_equal(verdict, NET_OK,
		      "All DHCP site scope multicast packet was dropped (%d)",
		      verdict);

	net_context_put(ctx);

	leave_group(&mcast_all_dhcp);
}

ZTEST(net_ipv6, test_dst_org_scope_mcast_recv)
{
	struct in6_addr mcast_org = { { { 0xff, 0x08, 0, 0, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0 } } };
	struct in6_addr addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	enum net_verdict verdict;

	verdict = recv_msg(&addr, &mcast_org);
	zassert_equal(verdict, NET_DROP,
		      "Organisation scope multicast packet was not dropped");
}

ZTEST(net_ipv6, test_dst_iface_scope_mcast_send)
{
	struct in6_addr mcast_iface = { { { 0xff, 0x01, 0, 0, 0, 0, 0, 0,
					    0, 0, 0, 0, 0, 0, 0, 0 } } };
	struct in6_addr addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	struct net_if_mcast_addr *maddr;
	struct net_context *ctx;
	int ret;

	/* Note that there is no need to join the multicast group as the
	 * interface local scope multicast address packet will not leave the
	 * device. But we will still need to add proper multicast address to
	 * the network interface.
	 */
	maddr = net_if_ipv6_maddr_add(TEST_NET_IF, &mcast_iface);
	zassert_not_null(maddr, "Cannot add multicast address to interface");

	net_ctx_create(&ctx);
	net_ctx_bind_mcast(ctx, &mcast_iface);
	net_ctx_listen(ctx);
	net_ctx_recv(ctx);

	ret = send_msg(&addr, &mcast_iface);
	zassert_equal(ret, 0,
		      "Interface local scope multicast packet was dropped (%d)",
		      ret);

	k_sem_take(&wait_data, K_MSEC(WAIT_TIME));

	zassert_true(recv_cb_called, "No data received on time, "
		     "IPv6 recv test failed");
	recv_cb_called = false;

	net_context_put(ctx);

	net_if_ipv6_maddr_rm(TEST_NET_IF, &mcast_iface);
}

ZTEST(net_ipv6, test_dst_unknown_group_mcast_recv)
{
	struct in6_addr mcast_unknown_group = {
		{ { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0x01, 0x02, 0x03, 0x04, 0x05,
		    0x06, 0x07, 0x08 } }
	};
	struct in6_addr in6_addr_any = IN6ADDR_ANY_INIT;
	struct in6_addr addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0x1 } } };
	struct net_context *ctx;
	enum net_verdict verdict;

	/* Create listening socket that is bound to all incoming traffic. */
	net_ctx_create(&ctx);
	net_ctx_bind_mcast(ctx, &in6_addr_any);
	net_ctx_listen(ctx);
	net_ctx_recv(ctx);

	/* Don't join multicast group before receiving packet.
	 * Expectation: packet should be dropped by receiving interface on IP
	 * Layer and not be received in listening socket.
	 */
	verdict = recv_msg(&addr, &mcast_unknown_group);

	zassert_equal(verdict, NET_DROP,
		      "Packet sent to unknown multicast group was not dropped");

	net_context_put(ctx);
}

ZTEST(net_ipv6, test_y_dst_unjoined_group_mcast_recv)
{
	struct in6_addr mcast_unjoined_group = {
		{ { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0x42, 0x42, 0x42, 0x42, 0x42,
		    0x42, 0x42, 0x42 } }
	};
	struct in6_addr in6_addr_any = IN6ADDR_ANY_INIT;
	struct in6_addr addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0x1 } } };
	struct net_if_mcast_addr *maddr;
	struct net_context *ctx;
	enum net_verdict verdict;

	/* Create listening socket that is bound to all incoming traffic. */
	net_ctx_create(&ctx);
	net_ctx_bind_mcast(ctx, &in6_addr_any);
	net_ctx_listen(ctx);
	net_ctx_recv(ctx);

	/* add multicast address to interface but do not join the group yet */
	maddr = net_if_ipv6_maddr_add(TEST_NET_IF, &mcast_unjoined_group);

	net_if_ipv6_maddr_leave(maddr);

	/* receive multicast on interface that did not join the group yet.
	 * Expectation: packet should be dropped by first interface on IP
	 * Layer and not be received in listening socket.
	 */
	verdict = recv_msg(&addr, &mcast_unjoined_group);

	zassert_equal(verdict, NET_DROP,
		      "Packet sent to unjoined multicast group was not "
		      "dropped.");

	/* now join the multicast group and attempt to receive again */
	net_if_ipv6_maddr_join(maddr);
	verdict = recv_msg(&addr, &mcast_unjoined_group);

	zassert_equal(verdict, NET_OK,
		      "Packet sent to joined multicast group was not "
		      "received.");

	net_if_ipv6_maddr_rm(TEST_NET_IF, &mcast_unjoined_group);

	net_context_put(ctx);
}

ZTEST(net_ipv6, test_dst_is_other_iface_mcast_recv)
{
	struct in6_addr mcast_iface2 = { { { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0x01,
					     0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
					     0x08 } } };
	struct in6_addr in6_addr_any = IN6ADDR_ANY_INIT;
	struct in6_addr addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0x1 } } };
	struct net_if_mcast_addr *maddr;
	struct net_context *ctx;
	enum net_verdict verdict;

	/* Create listening socket that is bound to all incoming traffic. */
	net_ctx_create(&ctx);
	net_ctx_bind_mcast(ctx, &in6_addr_any);
	net_ctx_listen(ctx);
	net_ctx_recv(ctx);

	/* Join multicast group on second interface. */
	maddr = net_if_ipv6_maddr_add(
		net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)),
		&mcast_iface2);
	zassert_not_null(maddr, "Cannot add multicast address to interface");
	net_if_ipv6_maddr_join(maddr);

	/* Receive multicast on first interface that did not join the group.
	 * Expectation: packet should be dropped by first interface on IP
	 * Layer and not be received in listening socket.
	 *
	 * Furthermore, multicast scope is link-local thus it should not cross
	 * interface boundaries.
	 */
	verdict = recv_msg(&addr, &mcast_iface2);

	zassert_equal(verdict, NET_DROP,
		      "Packet sent to multicast group joined by second "
		      "interface not dropped");

	net_if_ipv6_maddr_leave(maddr);

	net_if_ipv6_maddr_rm(net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)),
			     &mcast_iface2);

	net_context_put(ctx);
}

ZTEST_SUITE(net_ipv6, NULL, ipv6_setup, NULL, NULL, ipv6_teardown);
