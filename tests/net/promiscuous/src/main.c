/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_IF_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>

#include <ztest.h>

#include <net/ethernet.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/promiscuous.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

/* Interface 1 addresses */
static struct in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Interface 2 addresses */
static struct in6_addr my_addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 2, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Interface 3 addresses */
static struct in6_addr my_addr3 = { { { 0x20, 0x01, 0x0d, 0xb8, 3, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Extra address is assigned to ll_addr */
static struct in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
				       0x04 } } };

static struct in6_addr in6addr_mcast = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					     0, 0, 0, 0, 0, 0, 0, 0x1 } } };

static struct net_if *iface1;
static struct net_if *iface2;

#define WAIT_TIME 250

struct net_if_test {
	uint8_t idx;
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

struct eth_fake_context {
	struct net_if *iface;
	uint8_t mac_address[6];
	bool promisc_mode;
};

static struct eth_fake_context eth_fake_data1;
static struct eth_fake_context eth_fake_data2;

static void eth_fake_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->data;

	ctx->iface = iface;

	net_if_set_link_addr(iface, ctx->mac_address,
			     sizeof(ctx->mac_address),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_fake_send(const struct device *dev,
			 struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

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

static struct ethernet_api eth_fake_api_funcs = {
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

ETH_NET_DEVICE_INIT(eth_fake1, "eth_fake1",
		    eth_fake_init, device_pm_control_nop,
		    &eth_fake_data1, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &eth_fake_api_funcs, NET_ETH_MTU);

ETH_NET_DEVICE_INIT(eth_fake2, "eth_fake2",
		    eth_fake_init, device_pm_control_nop,
		    &eth_fake_data2, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &eth_fake_api_funcs, NET_ETH_MTU);

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static const char *iface2str(struct net_if *iface)
{
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		return "Ethernet";
	}

	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		return "Dummy";
	}

	return "<unknown type>";
}
#endif

static void iface_cb(struct net_if *iface, void *user_data)
{
	static int if_count;

	DBG("Interface %p (%s) [%d]\n", iface, iface2str(iface),
	    net_if_get_by_iface(iface));

	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		const struct ethernet_api *api =
			net_if_get_device(iface)->api;

		/* As native_posix board will introduce another ethernet
		 * interface, make sure that we only use our own in this test.
		 */
		if (api->get_capabilities ==
		    eth_fake_api_funcs.get_capabilities) {
			switch (if_count) {
			case 0:
				iface1 = iface;
				break;
			case 1:
				iface2 = iface;
				break;
			}

			if_count++;
		}
	}
}

static void test_iface_setup(void)
{
	struct net_if_mcast_addr *maddr;
	struct net_if_addr *ifaddr;
	int idx;

	net_if_foreach(iface_cb, NULL);

	idx = net_if_get_by_iface(iface1);
	((struct net_if_test *)
	 net_if_get_device(iface1)->data)->idx = idx;

	idx = net_if_get_by_iface(iface2);
	((struct net_if_test *)
	 net_if_get_device(iface2)->data)->idx = idx;

	zassert_not_null(iface1, "Interface 1");
	zassert_not_null(iface2, "Interface 2");

	DBG("Interfaces: [%d] iface1 %p, [%d] iface2 %p\n",
	    net_if_get_by_iface(iface1), iface1,
	    net_if_get_by_iface(iface2), iface2);

	ifaddr = net_if_ipv6_addr_add(iface1, &my_addr1,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr1));
		zassert_not_null(ifaddr, "addr1");
	}

	/* For testing purposes we need to set the adddresses preferred */
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface1, &ll_addr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&ll_addr));
		zassert_not_null(ifaddr, "ll_addr");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface2, &my_addr2,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr2));
		zassert_not_null(ifaddr, "addr2");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface2, &my_addr3,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr3));
		zassert_not_null(ifaddr, "addr3");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_ipv6_addr_create(&in6addr_mcast, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);

	maddr = net_if_ipv6_maddr_add(iface1, &in6addr_mcast);
	if (!maddr) {
		DBG("Cannot add multicast IPv6 address %s\n",
		       net_sprint_ipv6_addr(&in6addr_mcast));
		zassert_not_null(maddr, "mcast");
	}

	net_if_up(iface1);
	net_if_up(iface2);
}

static void _set_promisc_mode_on_again(struct net_if *iface)
{
	int ret;

	DBG("Make sure promiscuous mode is ON (%p)\n", iface);

	ret = net_promisc_mode_on(iface);

	zassert_equal(ret, -EALREADY, "iface %p promiscuous mode ON", iface);
}

static void _set_promisc_mode_on(struct net_if *iface)
{
	bool ret;

	DBG("Setting promiscuous mode ON (%p)\n", iface);

	ret = net_promisc_mode_on(iface);

	zassert_equal(ret, 0, "iface %p promiscuous mode set ON failed",
		      iface);
}

static void _set_promisc_mode_off_again(struct net_if *iface)
{
	int ret;

	DBG("Make sure promiscuous mode is OFF (%p)\n", iface);

	ret = net_promisc_mode_off(iface);

	zassert_equal(ret, -EALREADY, "iface %p promiscuous mode OFF", iface);
}

static void _set_promisc_mode_off(struct net_if *iface)
{
	int ret;

	DBG("Setting promiscuous mode OFF (%p)\n", iface);

	ret = net_promisc_mode_off(iface);

	zassert_equal(ret, 0, "iface %p promiscuous mode set OFF failed",
		      iface);
}

static void test_set_promisc_mode_on_again(void)
{
	_set_promisc_mode_on_again(iface1);
	_set_promisc_mode_on_again(iface2);
}

static void test_set_promisc_mode_on(void)
{
	_set_promisc_mode_on(iface1);
	_set_promisc_mode_on(iface2);
}

static void test_set_promisc_mode_off_again(void)
{
	_set_promisc_mode_off_again(iface1);
	_set_promisc_mode_off_again(iface2);
}

static void test_set_promisc_mode_off(void)
{
	_set_promisc_mode_off(iface1);
	_set_promisc_mode_off(iface2);
}

static void _recv_data(struct net_if *iface, struct net_pkt **pkt)
{
	static uint8_t data[] = { 't', 'e', 's', 't', '\0' };
	int ret;

	*pkt = net_pkt_rx_alloc_with_buffer(iface, sizeof(data),
					    AF_UNSPEC, 0, K_FOREVER);

	net_pkt_write(*pkt, data, sizeof(data));

	ret = net_recv_data(iface, *pkt);
	zassert_equal(ret, 0, "Data receive failure");
}

static struct net_pkt *pkt1;
static struct net_pkt *pkt2;

static void test_recv_data(void)
{
	_recv_data(iface1, &pkt1);
	_recv_data(iface2, &pkt2);
}

static void test_verify_data(void)
{
	struct net_pkt *pkt;

	pkt = net_promisc_mode_wait_data(K_SECONDS(1));
	zassert_equal_ptr(pkt, pkt1, "pkt %p != %p", pkt, pkt1);
	net_pkt_unref(pkt);

	pkt = net_promisc_mode_wait_data(K_SECONDS(1));
	zassert_equal_ptr(pkt, pkt2, "pkt %p != %p", pkt, pkt2);
	net_pkt_unref(pkt);
}

void test_main(void)
{
	ztest_test_suite(net_promisc_test,
			 ztest_unit_test(test_iface_setup),
			 ztest_unit_test(test_set_promisc_mode_on),
			 ztest_unit_test(test_set_promisc_mode_on_again),
			 ztest_unit_test(test_recv_data),
			 ztest_unit_test(test_verify_data),
			 ztest_unit_test(test_set_promisc_mode_off),
			 ztest_unit_test(test_set_promisc_mode_off_again));

	ztest_run_test_suite(net_promisc_test);
}
