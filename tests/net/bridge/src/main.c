/*
 * Copyright (c) 2021 BayLibre SAS
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

#include <ztest.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_bridge.h>

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

struct eth_fake_context {
	struct net_if *iface;
	struct net_pkt *sent_pkt;
	uint8_t mac_address[6];
	bool promisc_mode;
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
	 * Ignore packets we don't care about for this test, like
	 * the IP autoconfig related ones, etc.
	 */
	if (eth_hdr->type != htons(NET_ETH_PTYPE_ALL)) {
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

static enum ethernet_hw_caps eth_fake_get_capabilities(const struct device *dev)
{
	return ETHERNET_PROMISC_MODE;
}

static int eth_fake_set_config(const struct device *dev,
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
}

static int orig_rx_num_blocks;
static int orig_tx_num_blocks;

static void get_free_packet_count(void)
{
	struct k_mem_slab *rx, *tx;

	net_pkt_get_info(&rx, &tx, NULL, NULL);
	orig_rx_num_blocks = rx->num_blocks;
	orig_tx_num_blocks = tx->num_blocks;
}

static void check_free_packet_count(void)
{
	struct k_mem_slab *rx, *tx;

	net_pkt_get_info(&rx, &tx, NULL, NULL);
	zassert_equal(rx->num_blocks, orig_rx_num_blocks, "");
	zassert_equal(tx->num_blocks, orig_tx_num_blocks, "");
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
					   AF_UNSPEC, 0, K_FOREVER);
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

	eth_hdr.type = htons(NET_ETH_PTYPE_ALL);

	ret = net_pkt_write(pkt, &eth_hdr, sizeof(eth_hdr));
	zassert_equal(ret, 0, "");

	ret = net_pkt_write(pkt, data, sizeof(data));
	zassert_equal(ret, 0, "");

	DBG("Fake recv pkt %p\n", pkt);
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

static ETH_BRIDGE_INIT(test_bridge);

static void test_setup_bridge(void)
{
	int ret;

	/* add our interfaces to the bridge */
	ret = eth_bridge_iface_add(&test_bridge, fake_iface[0]);
	zassert_equal(ret, 0, "");
	ret = eth_bridge_iface_add(&test_bridge, fake_iface[1]);
	zassert_equal(ret, 0, "");
	ret = eth_bridge_iface_add(&test_bridge, fake_iface[2]);
	zassert_equal(ret, 0, "");

	/* enable tx for them except fake_iface[1] */
	ret = eth_bridge_iface_allow_tx(fake_iface[0], true);
	zassert_equal(ret, 0, "");
	ret = eth_bridge_iface_allow_tx(fake_iface[2], true);
	zassert_equal(ret, 0, "");
}

static void test_recv_with_bridge(void)
{
	int i, j;

	for (i = 0; i < 3; i++) {
		int src_if_idx = net_if_get_by_iface(fake_iface[i]);

		/* fake reception of packets */
		_recv_data(fake_iface[i]);

		/* give time to the processing threads to run */
		k_sleep(K_MSEC(100));

		/* nothing should have been transmitted on fake_iface[1] */
		zassert_is_null(eth_fake_data[1].sent_pkt, "");

		/*
		 * fake_iface[0] and fake_iface[2] should have sent the packet
		 * but only if it didn't come from them.
		 * We skip fake_iface[1] handled above.
		 */
		for (j = 0; j < 3; j += 2) {
			struct net_pkt *pkt = eth_fake_data[j].sent_pkt;

			if (eth_fake_data[j].iface == fake_iface[i]) {
				zassert_is_null(pkt, "");
				continue;
			}

			eth_fake_data[j].sent_pkt = NULL;
			zassert_not_null(pkt, "");

			/* make sure nothing messed up our ethernet header */
			struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);

			zassert_equal(hdr->dst.addr[0], 0xb2, "");
			zassert_equal(hdr->src.addr[0], 0xa2, "");
			zassert_equal(hdr->dst.addr[4], src_if_idx, "");
			zassert_equal(hdr->src.addr[3], src_if_idx, "");

			net_pkt_unref(pkt);
		}
	}

	check_free_packet_count();
}

static void test_recv_after_bridging(void)
{
	int ret;

	/* remove our interfaces from the bridge */
	ret = eth_bridge_iface_remove(&test_bridge, fake_iface[0]);
	zassert_equal(ret, 0, "");
	ret = eth_bridge_iface_remove(&test_bridge, fake_iface[1]);
	zassert_equal(ret, 0, "");
	ret = eth_bridge_iface_remove(&test_bridge, fake_iface[2]);
	zassert_equal(ret, 0, "");

	/* things should have returned to the pre-bridging state */
	test_recv_before_bridging();
}

ZTEST(net_eth_bridge, test_net_eth_bridge)
{
	test_iface_setup();
	test_recv_before_bridging();
	test_setup_bridge();
	test_recv_with_bridge();
	test_recv_after_bridging();
}

ZTEST_SUITE(net_eth_bridge, NULL, NULL, NULL, NULL, NULL);
