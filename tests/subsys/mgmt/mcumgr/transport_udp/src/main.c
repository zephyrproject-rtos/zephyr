/*
 * Copyright (c) 2026 Sergei Shirokov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The UDP SMP transport answers on the interface a request came in on. Two
 * interfaces have addresses on one subnet: an SMP echo request to either
 * interface's address must be answered through that interface, from that
 * address. Replying by destination alone would send both answers through the
 * same interface, and a client would drop the one from the wrong address.
 */

#include <zephyr/kernel.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt_defines.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/udp.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include <string.h>

#include "ipv4.h"
#include "ipv6.h"
#include "udp_internal.h"

#define PEER_PORT   4343
#define WAIT_TIME   K_MSEC(1000)
#define SMP_HDR_LEN 8

static const struct net_in_addr my_addr4[] = {
	{ { { 192, 0, 2, 1 } } },
	{ { { 192, 0, 2, 2 } } },
};
static const struct net_in_addr netmask4 = { { { 255, 255, 255, 0 } } };
static const struct net_in_addr peer_addr4 = { { { 192, 0, 2, 10 } } };

static const struct net_in6_addr my_addr6[] = {
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } },
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2 } } },
};
static const struct net_in6_addr prefix6 = {
	{ { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } };
static const struct net_in6_addr peer_addr6 = {
	{ { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x10 } } };

/* Neighbour discovery is off, so the peer is added to the cache by hand. */
static const struct net_linkaddr peer_lladdr = {
	.addr = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x10 },
	.len = NET_ETH_ADDR_LEN,
	.type = NET_LINK_ETHERNET,
};

/* What the last UDP packet sent looked like. */
static struct {
	struct net_if *iface;
	uint8_t src[NET_IPV6_ADDR_SIZE];
	uint16_t src_port;
	uint16_t dst_port;
	uint8_t smp[SMP_HDR_LEN];
} sent;

static K_SEM_DEFINE(sent_sem, 0, 1);

struct test_dev_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
};

static struct test_dev_context test_dev_ctx[2] = {
	{ .mac_addr = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x01 } },
	{ .mac_addr = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x02 } },
};

static void test_iface_init(struct net_if *iface)
{
	struct test_dev_context *ctx = net_if_get_device(iface)->data;

	net_if_set_link_addr(iface, ctx->mac_addr, sizeof(ctx->mac_addr), NET_LINK_ETHERNET);
}

static int test_send(const struct device *dev, struct net_pkt *pkt)
{
	bool v4 = (net_pkt_family(pkt) == NET_AF_INET);
	uint8_t proto = v4 ? NET_IPV4_HDR(pkt)->proto : NET_IPV6_HDR(pkt)->nexthdr;
	struct net_udp_hdr udp;

	ARG_UNUSED(dev);

	if (proto != NET_IPPROTO_UDP) {
		return 0;
	}

	sent.iface = net_pkt_iface(pkt);
	if (v4) {
		memcpy(sent.src, NET_IPV4_HDR(pkt)->src, NET_IPV4_ADDR_SIZE);
	} else {
		memcpy(sent.src, NET_IPV6_HDR(pkt)->src, NET_IPV6_ADDR_SIZE);
	}

	net_pkt_cursor_init(pkt);
	if (net_pkt_skip(pkt, v4 ? NET_IPV4H_LEN : NET_IPV6H_LEN) == 0 &&
	    net_pkt_read(pkt, &udp, sizeof(udp)) == 0 &&
	    net_pkt_read(pkt, sent.smp, sizeof(sent.smp)) == 0) {
		sent.src_port = net_ntohs(udp.src_port);
		sent.dst_port = net_ntohs(udp.dst_port);
	}

	k_sem_give(&sent_sem);

	return 0;
}

static DEVICE_API(dummy, test_if_api) = {
	.iface_api.init = test_iface_init,
	.send = test_send,
};

NET_DEVICE_INIT_INSTANCE(test_dev0, "test_dev0", 0, NULL, NULL, &test_dev_ctx[0], NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &test_if_api, DUMMY_L2,
			 NET_L2_GET_CTX_TYPE(DUMMY_L2), 1280);

NET_DEVICE_INIT_INSTANCE(test_dev1, "test_dev1", 1, NULL, NULL, &test_dev_ctx[1], NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &test_if_api, DUMMY_L2,
			 NET_L2_GET_CTX_TYPE(DUMMY_L2), 1280);

static struct net_if *ifaces[2];
static uint8_t seq;

/* Deliver an SMP echo request from the peer to ifaces[n]'s address, as if it
 * came in on that interface: an 8-byte SMP header (version 1, write, OS group,
 * echo) and the CBOR map {"d": "hi"}.
 */
static void inject_echo(int n, net_sa_family_t family)
{
	static const uint8_t cbor[] = { 0xa1, 0x61, 'd', 0x62, 'h', 'i' };
	uint8_t frame[SMP_HDR_LEN + sizeof(cbor)] = {
		MGMT_OP_WRITE, 0, 0, sizeof(cbor), 0, MGMT_GROUP_ID_OS, ++seq, OS_MGMT_ID_ECHO,
	};
	struct net_pkt *pkt = net_pkt_rx_alloc_with_buffer(ifaces[n], sizeof(frame), family,
							   NET_IPPROTO_UDP, K_NO_WAIT);

	memcpy(&frame[SMP_HDR_LEN], cbor, sizeof(cbor));
	zassert_not_null(pkt, "Cannot allocate packet");

	if (family == NET_AF_INET) {
		zassert_ok(net_ipv4_create(pkt, &peer_addr4, &my_addr4[n]),
			   "Cannot create IPv4 header");
	} else {
		zassert_ok(net_ipv6_create(pkt, &peer_addr6, &my_addr6[n]),
			   "Cannot create IPv6 header");
	}

	zassert_ok(net_udp_create(pkt, net_htons(PEER_PORT),
				  net_htons(CONFIG_MCUMGR_TRANSPORT_UDP_PORT)),
		   "Cannot create UDP header");
	zassert_ok(net_pkt_write(pkt, frame, sizeof(frame)), "Cannot write request");
	net_pkt_cursor_init(pkt);

	if (family == NET_AF_INET) {
		zassert_ok(net_ipv4_finalize(pkt, NET_IPPROTO_UDP), "Cannot finalize packet");
	} else {
		zassert_ok(net_ipv6_finalize(pkt, NET_IPPROTO_UDP), "Cannot finalize packet");
	}

	zassert_ok(net_recv_data(ifaces[n], pkt), "Cannot receive packet");
}

/* The transport opens its sockets once an interface is up, from its own
 * threads: retry the first request until the transport answers it.
 */
static void wait_for_transport(net_sa_family_t family)
{
	for (int i = 0; i < 20; i++) {
		k_sem_reset(&sent_sem);
		inject_echo(0, family);
		if (k_sem_take(&sent_sem, K_MSEC(100)) == 0) {
			return;
		}
	}

	zassert_unreachable("The UDP SMP transport never answered");
}

static void *setup(void)
{
	ifaces[0] = NET_IF_GET(test_dev0, 0);
	ifaces[1] = NET_IF_GET(test_dev1, 1);

	for (int i = 0; i < ARRAY_SIZE(ifaces); i++) {
		zassert_not_null(net_if_ipv4_addr_add(ifaces[i], &my_addr4[i], NET_ADDR_MANUAL, 0),
				 "Cannot add IPv4 address %d", i);
		zassert_true(net_if_ipv4_set_netmask_by_addr(ifaces[i], &my_addr4[i], &netmask4),
			     "Cannot set IPv4 netmask %d", i);

		zassert_not_null(net_if_ipv6_addr_add(ifaces[i], &my_addr6[i], NET_ADDR_MANUAL, 0),
				 "Cannot add IPv6 address %d", i);
		zassert_not_null(net_if_ipv6_prefix_add(ifaces[i], &prefix6, 64, UINT32_MAX),
				 "Cannot add IPv6 prefix %d", i);
		zassert_not_null(net_ipv6_nbr_add(ifaces[i], &peer_addr6, &peer_lladdr, false,
						  NET_IPV6_NBR_STATE_REACHABLE),
				 "Cannot add IPv6 neighbour %d", i);

		(void)net_if_up(ifaces[i]);
	}

	wait_for_transport(NET_AF_INET);
	wait_for_transport(NET_AF_INET6);

	return NULL;
}

/* Each interface in turn, in both orders, so that neither wins only by being
 * first.
 */
static void check_reply_iface(net_sa_family_t family)
{
	for (int i = 0; i < 2 * ARRAY_SIZE(ifaces); i++) {
		int n = (i == 1 || i == 2) ? 1 : 0;

		k_sem_reset(&sent_sem);
		memset(&sent, 0, sizeof(sent));

		inject_echo(n, family);

		zassert_ok(k_sem_take(&sent_sem, WAIT_TIME), "No reply");
		zassert_equal(sent.src_port, CONFIG_MCUMGR_TRANSPORT_UDP_PORT, "Not an SMP reply");
		zassert_equal(sent.dst_port, PEER_PORT, "Not sent to the peer");
		zassert_equal(sent.smp[0] & 0x07, MGMT_OP_WRITE_RSP, "Not a write response");
		zassert_equal(sent.smp[6], seq, "Reply to another request");

		zassert_equal_ptr(sent.iface, ifaces[n], "Reply on interface %d, expected %d",
				  net_if_get_by_iface(sent.iface), net_if_get_by_iface(ifaces[n]));

		if (family == NET_AF_INET) {
			zassert_mem_equal(sent.src, &my_addr4[n], NET_IPV4_ADDR_SIZE,
					  "Wrong source address for interface %d", n);
		} else {
			zassert_mem_equal(sent.src, &my_addr6[n], NET_IPV6_ADDR_SIZE,
					  "Wrong source address for interface %d", n);
		}
	}
}

ZTEST(mcumgr_transport_udp, test_v4_reply_on_request_iface)
{
	check_reply_iface(NET_AF_INET);
}

ZTEST(mcumgr_transport_udp, test_v6_reply_on_request_iface)
{
	check_reply_iface(NET_AF_INET6);
}

ZTEST_SUITE(mcumgr_transport_udp, NULL, setup, NULL, NULL, NULL);
