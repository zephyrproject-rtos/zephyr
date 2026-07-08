/*
 * Copyright (c) 2021 BayLibre SAS
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_ETHERNET_BRIDGE_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>

#include <zephyr/ztest.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_bridge.h>
#include <zephyr/net/ethernet_bridge_fdb.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/promiscuous.h>

#include "arp.h"

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static struct net_if *bridge;

struct eth_fake_context {
	struct net_if *iface;
	struct net_pkt *sent_pkt;
	uint8_t mac_address[6];
	bool promisc_mode;
	/* Destination MAC of the last locally originated IPv4 frame seen on
	 * this interface, used by the local-TX regression test.
	 */
	struct net_eth_addr last_ipv4_dst;
	bool last_ipv4_seen;
	/* Set when an ARP frame egressed this interface (used by the
	 * cache-miss variant of the local-TX regression test).
	 */
	bool arp_seen;
};

static void eth_fake_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->data;

	ctx->iface = iface;

	ctx->mac_address[0] = 0xc2;
	ctx->mac_address[1] = 0xaa;
	ctx->mac_address[2] = 0xbb;
	ctx->mac_address[3] = 0xcc;
	ctx->mac_address[4] = 0xdd;
	ctx->mac_address[5] = 0xee;

	net_if_set_link_addr(iface, ctx->mac_address,
			     sizeof(ctx->mac_address),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_fake_send(const struct device *dev,
			 struct net_pkt *pkt)
{
	struct eth_fake_context *ctx = dev->data;
	struct net_eth_hdr *eth_hdr = NET_ETH_HDR(pkt);

	/*
	 * Record the destination MAC of locally originated IPv4 frames so the
	 * local-TX regression test can check it was resolved to a unicast
	 * address instead of defaulting to broadcast.
	 */
	if (eth_hdr->type == net_htons(NET_ETH_PTYPE_IP)) {
		memcpy(&ctx->last_ipv4_dst, &eth_hdr->dst,
		       sizeof(ctx->last_ipv4_dst));
		ctx->last_ipv4_seen = true;
		return 0;
	}

	if (eth_hdr->type == net_htons(NET_ETH_PTYPE_ARP)) {
		ctx->arp_seen = true;
		return 0;
	}

	/*
	 * Ignore packets we don't care about for this test, like
	 * the IP autoconfig related ones, etc.
	 */
	if (eth_hdr->type != net_htons(NET_ETH_PTYPE_ALL)) {
		DBG("Fake send ignoring pkt %p\n", pkt);
		return 0;
	}

	if (ctx->sent_pkt != NULL) {
		DBG("Fake send found pkt %p while sending %p\n",
		    ctx->sent_pkt, pkt);
		return -EBUSY;
	}
	ctx->sent_pkt = net_pkt_shallow_clone(pkt, K_NO_WAIT);
	if (ctx->sent_pkt == NULL) {
		DBG("Fake send out of mem while sending pkt %p\n", pkt);
		return -ENOMEM;
	}

	DBG("Fake send pkt %p kept locally as %p\n", pkt, ctx->sent_pkt);
	return 0;
}

static enum ethernet_hw_caps eth_fake_get_capabilities(const struct device *dev,
						       struct net_if *iface __unused)
{
	return ETHERNET_PROMISC_MODE;
}

static int eth_fake_set_config(const struct device *dev,
			       struct net_if *iface __unused,
			       enum ethernet_config_type type,
			       const struct ethernet_config *config)
{
	struct eth_fake_context *ctx = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (config->promisc_mode == ctx->promisc_mode) {
			return -EALREADY;
		}

		ctx->promisc_mode = config->promisc_mode;

		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static const struct ethernet_api eth_fake_api_funcs = {
	.iface_api.init = eth_fake_iface_init,
	.get_capabilities = eth_fake_get_capabilities,
	.set_config = eth_fake_set_config,
	.send = eth_fake_send,
};

static int eth_fake_init(const struct device *dev)
{
	struct eth_fake_context *ctx = dev->data;

	ctx->promisc_mode = false;

	return 0;
}

static struct eth_fake_context eth_fake_data[3];

ETH_NET_DEVICE_INIT(eth_fake0, "eth_fake0",
		    eth_fake_init, NULL,
		    &eth_fake_data[0], NULL, CONFIG_ETH_INIT_PRIORITY,
		    &eth_fake_api_funcs, NET_ETH_MTU);

ETH_NET_DEVICE_INIT(eth_fake1, "eth_fake1",
		    eth_fake_init, NULL,
		    &eth_fake_data[1], NULL, CONFIG_ETH_INIT_PRIORITY,
		    &eth_fake_api_funcs, NET_ETH_MTU);

ETH_NET_DEVICE_INIT(eth_fake2, "eth_fake2",
		    eth_fake_init, NULL,
		    &eth_fake_data[2], NULL, CONFIG_ETH_INIT_PRIORITY,
		    &eth_fake_api_funcs, NET_ETH_MTU);

static struct net_if *fake_iface[3];

static void iface_cb(struct net_if *iface, void *user_data)
{
	static int if_count;

	if (if_count >= ARRAY_SIZE(fake_iface)) {
		return;
	}

	DBG("Interface %p [%d]\n", iface, net_if_get_by_iface(iface));

	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		const struct ethernet_api *api =
			net_if_get_device(iface)->api;

		/*
		 * We want to only use struct net_if devices defined in
		 * this test as board on which it is run can have its
		 * own set of interfaces.
		 */
		if (api->get_capabilities ==
		    eth_fake_api_funcs.get_capabilities) {
			fake_iface[if_count++] = iface;
		}
	}

	if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		enum virtual_interface_caps caps;

		caps = net_virtual_get_iface_capabilities(iface);
		if (caps & VIRTUAL_INTERFACE_BRIDGE) {
			bridge = iface;
		}
	}
}

static int orig_rx_num_blocks;
static int orig_tx_num_blocks;

static void get_free_packet_count(void)
{
	struct k_mem_slab *rx, *tx;

	net_pkt_get_info(&rx, &tx, NULL, NULL);
	orig_rx_num_blocks = rx->info.num_blocks;
	orig_tx_num_blocks = tx->info.num_blocks;
}

static void check_free_packet_count(void)
{
	struct k_mem_slab *rx, *tx;

	net_pkt_get_info(&rx, &tx, NULL, NULL);
	zassert_equal(rx->info.num_blocks, orig_rx_num_blocks, "");
	zassert_equal(tx->info.num_blocks, orig_tx_num_blocks, "");
}

static void test_iface_setup(void)
{
	net_if_foreach(iface_cb, NULL);

	zassert_not_null(fake_iface[0], "");
	zassert_not_null(fake_iface[1], "");
	zassert_not_null(fake_iface[2], "");

	DBG("Interfaces: [%d] iface0 %p, [%d] iface1 %p, [%d] iface2 %p\n",
	    net_if_get_by_iface(fake_iface[0]), fake_iface[0],
	    net_if_get_by_iface(fake_iface[1]), fake_iface[1],
	    net_if_get_by_iface(fake_iface[2]), fake_iface[2]);

	net_if_up(fake_iface[0]);
	net_if_up(fake_iface[1]);
	net_if_up(fake_iface[2]);

	/* Remember the initial number of free packets in the pool. */
	get_free_packet_count();
}

/*
 * Simulate a packet reception from the outside world
 */
static void _recv_data(struct net_if *iface)
{
	struct net_pkt *pkt;
	struct net_eth_hdr eth_hdr;
	static uint8_t data[] = { 't', 'e', 's', 't', '\0' };
	int ret;

	pkt = net_pkt_rx_alloc_with_buffer(iface, sizeof(eth_hdr) + sizeof(data),
					   NET_AF_UNSPEC, 0, K_FOREVER);
	zassert_not_null(pkt, "");

	/*
	 * The source and destination MAC addresses are completely arbitrary
	 * except for the U/L and I/G bits. However, the index of the faked
	 * incoming interface is mixed in as well to create some variation,
	 * and to help with validation on the transmit side.
	 */

	eth_hdr.dst.addr[0] = 0xb2;
	eth_hdr.dst.addr[1] = 0x11;
	eth_hdr.dst.addr[2] = 0x22;
	eth_hdr.dst.addr[3] = 0x33;
	eth_hdr.dst.addr[4] = net_if_get_by_iface(iface);
	eth_hdr.dst.addr[5] = 0x55;

	eth_hdr.src.addr[0] = 0xa2;
	eth_hdr.src.addr[1] = 0x11;
	eth_hdr.src.addr[2] = 0x22;
	eth_hdr.src.addr[3] = net_if_get_by_iface(iface);
	eth_hdr.src.addr[4] = 0x77;
	eth_hdr.src.addr[5] = 0x88;

	eth_hdr.type = net_htons(NET_ETH_PTYPE_ALL);

	ret = net_pkt_write(pkt, &eth_hdr, sizeof(eth_hdr));
	zassert_equal(ret, 0, "");

	ret = net_pkt_write(pkt, data, sizeof(data));
	zassert_equal(ret, 0, "");

	DBG("[%d] Fake recv pkt %p\n", net_if_get_by_iface(iface), pkt);
	ret = net_recv_data(iface, pkt);
	zassert_equal(ret, 0, "");
}

static void test_recv_before_bridging(void)
{
	/* fake some packet reception */
	_recv_data(fake_iface[0]);
	_recv_data(fake_iface[1]);
	_recv_data(fake_iface[2]);

	/* give time to the processing threads to run */
	k_sleep(K_MSEC(100));

	/* nothing should have been transmitted at this point */
	zassert_is_null(eth_fake_data[0].sent_pkt, "");
	zassert_is_null(eth_fake_data[1].sent_pkt, "");
	zassert_is_null(eth_fake_data[2].sent_pkt, "");

	/* and everything already dropped. */
	check_free_packet_count();
}

static void test_setup_bridge(void)
{
	int ret;

	/* add our interfaces to the bridge */
	ret = eth_bridge_iface_add(bridge, fake_iface[0]);
	zassert_equal(ret, 0, "");
	ret = eth_bridge_iface_add(bridge, fake_iface[1]);
	zassert_equal(ret, 0, "");

	/* Try to add the bridge twice, there should be no error */
	ret = eth_bridge_iface_add(bridge, fake_iface[1]);
	zassert_equal(ret, 0, "");

	ret = eth_bridge_iface_add(bridge, fake_iface[2]);
	zassert_equal(ret, 0, "");

	ret = net_if_up(bridge);
	zassert_equal(ret, 0, "");
}

/*
 * When the bridge interface owns an IPv4 address it can originate traffic of
 * its own (e.g. a TCP server bound to the bridge). Such locally originated
 * IPv4 packets used to be flooded to the member ports with an unresolved
 * destination link address, which the Ethernet layer then defaulted to
 * broadcast (ff:ff:ff:ff:ff:ff). Verify the destination is resolved to the
 * neighbor's unicast MAC instead.
 */
#if defined(CONFIG_NET_IPV4)
static const struct net_in_addr bridge_ip = { { { 192, 0, 2, 1 } } };

/* Give the bridge an IPv4 address so a same-subnet peer is considered on-link.
 * Idempotent: safe to call from more than one test.
 */
static void ensure_bridge_ipv4(void)
{
	struct net_in_addr netmask = { { { 255, 255, 255, 0 } } };

	zassert_not_null(net_if_ipv4_addr_add(bridge, (struct net_in_addr *)&bridge_ip,
					      NET_ADDR_MANUAL, 0), "");
	zassert_true(net_if_ipv4_set_netmask_by_addr(bridge,
						     (struct net_in_addr *)&bridge_ip,
						     &netmask), "");
}

/* Craft a locally originated IPv4 unicast packet for the peer and hand it to
 * the bridge TX path, mimicking the bridge's own IP stack.
 */
static void send_local_ipv4(const struct net_in_addr *peer_ip)
{
	struct net_ipv4_hdr ip_hdr = { 0 };
	static uint8_t payload[] = { 'p', 'i', 'n', 'g' };
	struct net_pkt *pkt;
	int i, ret;

	for (i = 0; i < 3; i++) {
		eth_fake_data[i].last_ipv4_seen = false;
		eth_fake_data[i].arp_seen = false;
	}

	pkt = net_pkt_alloc_with_buffer(bridge, sizeof(ip_hdr) + sizeof(payload),
					NET_AF_INET, NET_IPPROTO_UDP, K_FOREVER);
	zassert_not_null(pkt, "");

	ip_hdr.vhl = 0x45;
	ip_hdr.ttl = 64;
	ip_hdr.proto = NET_IPPROTO_UDP;
	ip_hdr.len = net_htons(sizeof(ip_hdr) + sizeof(payload));
	net_ipv4_addr_copy_raw(ip_hdr.src, (uint8_t *)&bridge_ip);
	net_ipv4_addr_copy_raw(ip_hdr.dst, (uint8_t *)peer_ip);

	ret = net_pkt_write(pkt, &ip_hdr, sizeof(ip_hdr));
	zassert_equal(ret, 0, "");
	ret = net_pkt_write(pkt, payload, sizeof(payload));
	zassert_equal(ret, 0, "");

	net_pkt_cursor_init(pkt);
	net_pkt_set_family(pkt, NET_AF_INET);
	net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_IP);

	net_if_queue_tx(bridge, pkt);

	/* give time to the processing threads to run */
	k_sleep(K_MSEC(100));
}

/* Every IPv4 frame that egressed a member port must carry the peer's unicast
 * MAC, never broadcast. At least one copy must have egressed.
 */
static void check_ipv4_unicast(const struct net_eth_addr *peer_mac)
{
	bool checked = false;
	int i;

	for (i = 0; i < 3; i++) {
		if (!eth_fake_data[i].last_ipv4_seen) {
			continue;
		}

		checked = true;

		zassert_false(net_eth_is_addr_broadcast(&eth_fake_data[i].last_ipv4_dst),
			      "iface %d: IPv4 frame sent to broadcast MAC",
			      net_if_get_by_iface(eth_fake_data[i].iface));
		zassert_mem_equal(&eth_fake_data[i].last_ipv4_dst, peer_mac,
				  sizeof(*peer_mac),
				  "iface %d: IPv4 frame dst MAC not resolved to peer",
				  net_if_get_by_iface(eth_fake_data[i].iface));
	}

	zassert_true(checked, "no IPv4 frame egressed any member port");
}

/* Cache hit: the neighbor is already in the bridge ARP cache, so the packet is
 * resolved and flooded to the members straight away.
 */
static void test_local_ipv4_tx_unicast(void)
{
	struct net_in_addr peer_ip = { { { 192, 0, 2, 2 } } };
	struct net_eth_addr peer_mac = {
		.addr = { 0x02, 0x11, 0x22, 0x33, 0x44, 0x55 }
	};

	ensure_bridge_ipv4();

	/* Seed the bridge ARP cache as if the peer had already been resolved
	 * (this is what an incoming SYN/ARP from the peer would have done).
	 */
	net_arp_update(bridge, &peer_ip, &peer_mac, false, true);

	send_local_ipv4(&peer_ip);

	check_ipv4_unicast(&peer_mac);

	check_free_packet_count();
}

/* Cache miss: the neighbor is unknown, so the bridge must flood an ARP request
 * (not the data frame) and hold the packet until the reply resolves it. Once
 * resolved, the held packet is flushed to the members with the unicast MAC.
 */
static void test_local_ipv4_tx_unicast_arp_miss(void)
{
	struct net_in_addr peer_ip = { { { 192, 0, 2, 3 } } };
	struct net_eth_addr peer_mac = {
		.addr = { 0x02, 0x66, 0x77, 0x88, 0x99, 0xaa }
	};
	bool arp_requested = false;
	int i;

	ensure_bridge_ipv4();

	/* Make sure the peer is not resolved yet. */
	send_local_ipv4(&peer_ip);

	/* The data frame must not have been sent yet: it is queued pending ARP.
	 * Instead an ARP request must have been flooded to the member ports.
	 */
	for (i = 0; i < 3; i++) {
		zassert_false(eth_fake_data[i].last_ipv4_seen,
			      "iface %d: IPv4 frame sent before ARP resolved",
			      net_if_get_by_iface(eth_fake_data[i].iface));
		arp_requested |= eth_fake_data[i].arp_seen;
	}

	zassert_true(arp_requested, "no ARP request flooded on cache miss");

	/* Simulate the peer's ARP reply arriving: this resolves the entry and
	 * flushes the queued packet back through the bridge TX path.
	 */
	net_arp_update(bridge, &peer_ip, &peer_mac, false, false);

	/* give time to the processing threads to run */
	k_sleep(K_MSEC(100));

	check_ipv4_unicast(&peer_mac);

	check_free_packet_count();
}
#endif /* CONFIG_NET_IPV4 */

static void test_recv_with_bridge(void)
{
	int i, j;

	for (i = 0; i < 3; i++) {
		int src_if_idx = net_if_get_by_iface(fake_iface[i]);

		/* fake reception of packets */
		_recv_data(fake_iface[i]);

		/* give time to the processing threads to run */
		k_sleep(K_MSEC(100));

		/*
		 * fake_iface[0] and fake_iface[2] should have sent the packet
		 * but only if it didn't come from them.
		 * We skip fake_iface[1] handled above.
		 */
		for (j = 0; j < 3; j += 2) {
			struct net_pkt *pkt = eth_fake_data[j].sent_pkt;
			struct net_eth_hdr *hdr;

			if (eth_fake_data[j].iface == fake_iface[i]) {
				zassert_is_null(pkt, "");
				continue;
			}

			eth_fake_data[j].sent_pkt = NULL;
			zassert_not_null(pkt, "");

			/* make sure nothing messed up our ethernet header */
			hdr = NET_ETH_HDR(pkt);

			zassert_equal(hdr->dst.addr[0], 0xb2, "");
			zassert_equal(hdr->src.addr[0], 0xa2, "");
			zassert_equal(hdr->dst.addr[4], src_if_idx, "");
			zassert_equal(hdr->src.addr[3], src_if_idx, "");

			net_pkt_unref(pkt);
		}
	}

	check_free_packet_count();
}

static void test_recv_with_bridge_fdb(void)
{
	struct net_eth_addr mac;
	struct net_pkt *pkt;
	struct net_eth_hdr *hdr;
	uint8_t iface0_index = net_if_get_by_iface(fake_iface[0]);
	int ret;

	/* Add FDB entry: forward fake_iface[0] rx pkt to fake_iface[1] tx path */
	mac.addr[0] = 0xb2;
	mac.addr[1] = 0x11;
	mac.addr[2] = 0x22;
	mac.addr[3] = 0x33;
	mac.addr[4] = iface0_index;
	mac.addr[5] = 0x55;

	ret = eth_bridge_fdb_add(&mac, fake_iface[1]);
	zassert_equal(ret, 0, "");

	/* fake reception of packets */
	_recv_data(fake_iface[0]);

	/* give time to the processing threads to run */
	k_sleep(K_MSEC(100));

	/* check fake_iface[0] tx path */
	pkt = eth_fake_data[0].sent_pkt;
	zassert_is_null(pkt, "");

	/* check fake_iface[2] tx path */
	pkt = eth_fake_data[2].sent_pkt;
	zassert_is_null(pkt, "");

	/* check fake_iface[1] tx path */
	pkt = eth_fake_data[1].sent_pkt;

	eth_fake_data[1].sent_pkt = NULL;
	zassert_not_null(pkt, "");

	/* make sure nothing messed up our ethernet header */
	hdr = NET_ETH_HDR(pkt);

	zassert_equal(hdr->dst.addr[0], 0xb2, "");
	zassert_equal(hdr->src.addr[0], 0xa2, "");
	zassert_equal(hdr->dst.addr[4], iface0_index, "");
	zassert_equal(hdr->src.addr[3], iface0_index, "");

	net_pkt_unref(pkt);

	check_free_packet_count();
}

static void test_recv_after_bridging(void)
{
	int ret;

	ret = net_if_down(bridge);
	zassert_equal(ret, 0, "");

	/* remove our interfaces from the bridge */
	ret = eth_bridge_iface_remove(bridge, fake_iface[0]);
	zassert_equal(ret, 0, "");
	ret = eth_bridge_iface_remove(bridge, fake_iface[1]);
	zassert_equal(ret, 0, "");
	ret = eth_bridge_iface_remove(bridge, fake_iface[2]);
	zassert_equal(ret, 0, "");

	/* If there are not enough interfaces in the bridge, it is not created */
	ret = net_if_up(bridge);
	zassert_equal(ret, -ENOENT, "");

	eth_fake_data[0].sent_pkt = eth_fake_data[1].sent_pkt =
		eth_fake_data[2].sent_pkt = NULL;

	/* things should have returned to the pre-bridging state */
	test_recv_before_bridging();
}

/* Make sure bridge interface support promiscuous API */
ZTEST(net_eth_bridge, test_verify_promisc_mode)
{
	int ret;

	ret = net_promisc_mode_on(bridge);
	zassert_equal(ret, 0, "");
}

ZTEST(net_eth_bridge, test_net_eth_bridge)
{
	DBG("Before bridging\n");
	test_iface_setup();
	test_recv_before_bridging();
	DBG("With bridging\n");
	test_setup_bridge();
#if defined(CONFIG_NET_IPV4)
	test_local_ipv4_tx_unicast();
	test_local_ipv4_tx_unicast_arp_miss();
#endif
	test_recv_with_bridge();
	test_recv_with_bridge_fdb();
	DBG("After bridging\n");
	test_recv_after_bridging();
}

ZTEST_SUITE(net_eth_bridge, NULL, NULL, NULL, NULL, NULL);
