/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ieee802154_radio.h>

#define TEST_FLAG_SET (NET_L2_MULTICAST | NET_L2_PROMISC_MODE)
#define TEST_PAYLOAD "TEST PAYLOAD"

static struct test_data {
	bool state;
	struct net_pkt *tx_pkt;
	struct net_pkt *rx_pkt;
} test_data;

static inline enum net_verdict custom_l2_recv(struct net_if *iface,
					      struct net_pkt *pkt)
{
	test_data.rx_pkt = pkt;

	return NET_OK;
}

static inline int custom_l2_send(struct net_if *iface, struct net_pkt *pkt)
{
	test_data.tx_pkt = pkt;

	return net_pkt_get_len(pkt);
}

static int custom_l2_enable(struct net_if *iface, bool state)
{
	test_data.state = state;

	return 0;
}

static enum net_l2_flags custom_l2_flags(struct net_if *iface)
{
	return TEST_FLAG_SET;
}

NET_L2_INIT(CUSTOM_IEEE802154_L2, custom_l2_recv, custom_l2_send,
	    custom_l2_enable, custom_l2_flags);

static void dummy_iface_init(struct net_if *iface)
{
	static uint8_t mac[8] = { 0x00, 0x11, 0x22, 0x33,
				  0x44, 0x55, 0x66, 0x77 };

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);
}

static int dummy_init(const struct device *dev)
{
	return 0;
}

static struct ieee802154_radio_api dummy_radio_api = {
	.iface_api.init	= dummy_iface_init,
};

NET_DEVICE_INIT(dummy, "dummy_ieee802154",
		dummy_init, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&dummy_radio_api, CUSTOM_IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(CUSTOM_IEEE802154_L2),
		CONFIG_NET_L2_CUSTOM_IEEE802154_MTU);

ZTEST(ieee802154_custom_l2, test_send)
{
	int ret;
	struct net_pkt *tx_pkt;
	struct net_if *iface = net_if_get_first_by_type(
					&NET_L2_GET_NAME(CUSTOM_IEEE802154));

	zassert_not_null(net_if_l2(iface), "No L2 found");
	zassert_not_null(net_if_l2(iface)->send, "No send() found");

	tx_pkt = net_pkt_alloc_with_buffer(iface, sizeof(TEST_PAYLOAD),
					   AF_UNSPEC, 0, K_NO_WAIT);
	zassert_not_null(tx_pkt, "Failed to allocate packet");

	ret = net_pkt_write(tx_pkt, TEST_PAYLOAD, sizeof(TEST_PAYLOAD));
	zassert_equal(0, ret, "Failed to write payload");

	ret = net_send_data(tx_pkt);
	zassert_equal(0, ret, "Failed to process TX packet");
	zassert_equal(tx_pkt, test_data.tx_pkt, "TX packet did not reach L2");

	net_pkt_unref(tx_pkt);
}

ZTEST(ieee802154_custom_l2, test_recv)
{
	int ret;
	struct net_pkt *rx_pkt;
	struct net_if *iface = net_if_get_first_by_type(
					&NET_L2_GET_NAME(CUSTOM_IEEE802154));

	zassert_not_null(net_if_l2(iface), "No L2 found");
	zassert_not_null(net_if_l2(iface)->recv, "No recv () found");

	rx_pkt = net_pkt_rx_alloc_with_buffer(iface, sizeof(TEST_PAYLOAD),
					      AF_UNSPEC, 0, K_NO_WAIT);
	zassert_not_null(rx_pkt, "Failed to allocate packet");

	ret = net_pkt_write(rx_pkt, TEST_PAYLOAD, sizeof(TEST_PAYLOAD));
	zassert_equal(0, ret, "Failed to write payload");

	ret = net_recv_data(iface, rx_pkt);
	zassert_equal(0, ret, "Failed to process RX packet");
	zassert_equal(rx_pkt, test_data.rx_pkt, "RX packet did not reach L2");

	net_pkt_unref(rx_pkt);
}

ZTEST(ieee802154_custom_l2, test_enable)
{
	int ret;
	struct net_if *iface = net_if_get_first_by_type(
					&NET_L2_GET_NAME(CUSTOM_IEEE802154));

	zassert_not_null(net_if_l2(iface), "No L2 found");
	zassert_not_null(net_if_l2(iface)->enable, "No enable() found");

	ret = net_if_down(iface);
	zassert_equal(0, ret, "Failed to set iface down");
	zassert_false(test_data.state, "L2 up");

	ret = net_if_up(iface);
	zassert_equal(0, ret, "Failed to set iface up");
	zassert_true(test_data.state, "L2 down");
}

ZTEST(ieee802154_custom_l2, test_flags)
{
	enum net_l2_flags flags;
	struct net_if *iface = net_if_get_first_by_type(
					&NET_L2_GET_NAME(CUSTOM_IEEE802154));
	zassert_not_null(net_if_l2(iface), "No L2 found");
	zassert_not_null(net_if_l2(iface)->get_flags, "No get_flags() found");

	flags = net_if_l2(iface)->get_flags(iface);
	zassert_equal(TEST_FLAG_SET, flags, "Invalid flags");
}

ZTEST_SUITE(ieee802154_custom_l2, NULL, NULL, NULL, NULL, NULL);
