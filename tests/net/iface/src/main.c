/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_IF_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/linker/sections.h>
#include <zephyr/random/random.h>

#include <zephyr/ztest.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_NET_IF_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

/* Interface 1 addresses */
static struct in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static ZTEST_BMEM struct in_addr my_ipv4_addr1 = { { { 192, 0, 2, 1 } } };

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

static struct in_addr inaddr_mcast = { { { 224, 0, 0, 1 } } };
static struct in6_addr in6addr_mcast;

static struct net_if *iface1;
static struct net_if *iface2;
static struct net_if *iface3;
static struct net_if *iface4;

static bool test_failed;
static bool test_started;
static struct k_sem wait_data;
static bool device_ok;

#define WAIT_TIME 250

struct net_if_test {
	uint8_t idx;
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

static uint8_t *net_iface_get_mac(const struct device *dev)
{
	struct net_if_test *data = dev->data;

	if (data->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		data->mac_addr[0] = 0x00;
		data->mac_addr[1] = 0x00;
		data->mac_addr[2] = 0x5E;
		data->mac_addr[3] = 0x00;
		data->mac_addr[4] = 0x53;
		data->mac_addr[5] = sys_rand8_get();
	}

	data->ll_addr.addr = data->mac_addr;
	data->ll_addr.len = 6U;

	return data->mac_addr;
}

static void net_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_iface_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static int dev_init(const struct device *dev)
{
	if (device_ok == false) {
		return -EAGAIN;
	}

	return 0;
}

static int sender_iface(const struct device *dev, struct net_pkt *pkt)
{
	if (!pkt->buffer) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	if (test_started) {
		struct net_if_test *data = dev->data;

		DBG("Sending at iface %d %p\n",
		    net_if_get_by_iface(net_pkt_iface(pkt)),
		    net_pkt_iface(pkt));

		if (net_if_get_by_iface(net_pkt_iface(pkt)) != data->idx) {
			DBG("Invalid interface %d index, expecting %d\n",
			    data->idx, net_if_get_by_iface(net_pkt_iface(pkt)));
			test_failed = true;
		}
	}

	k_sem_give(&wait_data);

	return 0;
}

struct net_if_test net_iface1_data;
struct net_if_test net_iface2_data;
struct net_if_test net_iface3_data;

static struct dummy_api net_iface_api = {
	.iface_api.init = net_iface_init,
	.send = sender_iface,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(net_iface1_test,
			 "iface1",
			 iface1,
			 dev_init,
			 NULL,
			 &net_iface1_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE,
			 127);

NET_DEVICE_INIT_INSTANCE(net_iface2_test,
			 "iface2",
			 iface2,
			 NULL,
			 NULL,
			 &net_iface2_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE,
			 127);

NET_DEVICE_INIT_INSTANCE(net_iface3_test,
			 "iface3",
			 iface3,
			 NULL,
			 NULL,
			 &net_iface3_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE,
			 127);

struct eth_fake_context {
	struct net_if *iface;
	uint8_t mac_address[6];
	bool promisc_mode;
};

static struct eth_fake_context eth_fake_data;

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

ETH_NET_DEVICE_INIT(eth_fake, "eth_fake", eth_fake_init, NULL,
		    &eth_fake_data, NULL, CONFIG_ETH_INIT_PRIORITY,
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

		/* As native_sim board will introduce another ethernet
		 * interface, make sure that we only use our own in this test.
		 */
		if (api->get_capabilities ==
		    eth_fake_api_funcs.get_capabilities) {
			iface4 = iface;
		}
	} else {
		switch (if_count) {
		case 0:
			iface1 = iface;
			break;
		case 1:
			iface2 = iface;
			break;
		case 2:
			iface3 = iface;
			break;
		}

		if_count++;
	}
}

static void *iface_setup(void)
{
	struct net_if_mcast_addr *maddr;
	struct net_if_addr *ifaddr;
	const struct device *dev;
	bool status;
	int idx, ret;

	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);

	net_if_foreach(iface_cb, NULL);

	idx = net_if_get_by_iface(iface1);
	((struct net_if_test *)
	 net_if_get_device(iface1)->data)->idx = idx;

	idx = net_if_get_by_iface(iface2);
	((struct net_if_test *)
	 net_if_get_device(iface2)->data)->idx = idx;

	idx = net_if_get_by_iface(iface3);
	((struct net_if_test *)
	 net_if_get_device(iface3)->data)->idx = idx;

	DBG("Interfaces: [%d] iface1 %p, [%d] iface2 %p, [%d] iface3 %p\n",
	    net_if_get_by_iface(iface1), iface1,
	    net_if_get_by_iface(iface2), iface2,
	    net_if_get_by_iface(iface3), iface3);

	zassert_not_null(iface1, "Interface 1");
	zassert_not_null(iface2, "Interface 2");
	zassert_not_null(iface3, "Interface 3");

	/* Make sure that the first interface device is not ready */
	dev = net_if_get_device(iface1);
	zassert_not_null(dev, "Device is not set!");

	status = device_is_ready(dev);
	zassert_equal(status, false,  "Device %s (%p) is ready!",
		      dev->name, dev);

	/* Trying to take the interface up will fail */
	ret = net_if_up(iface1);
	zassert_equal(ret, -ENXIO, "Interface 1 is up (%d)", ret);

	/* Try to set dormant state */
	net_if_dormant_on(iface1);

	/* Operational state should be "oper down" */
	zassert_equal(iface1->if_dev->oper_state, NET_IF_OPER_DOWN,
		      "Invalid operational state (%d)",
		      iface1->if_dev->oper_state);

	/* Mark the device ready and take the interface up */
	dev->state->init_res = 0;
	device_ok = true;

	ret = net_if_up(iface1);
	zassert_equal(ret, 0, "Interface 1 is not up (%d)", ret);

	zassert_equal(iface1->if_dev->oper_state, NET_IF_OPER_DORMANT,
		      "Invalid operational state (%d)",
		      iface1->if_dev->oper_state);

	net_if_dormant_off(iface1);
	zassert_equal(iface1->if_dev->oper_state, NET_IF_OPER_UP,
		      "Invalid operational state (%d)",
		      iface1->if_dev->oper_state);

	ifaddr = net_if_ipv6_addr_add(iface1, &my_addr1,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr1));
		zassert_not_null(ifaddr, "addr1");
	}

	ifaddr = net_if_ipv4_addr_add(iface1, &my_ipv4_addr1,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv4 address %s\n",
		       net_sprint_ipv4_addr(&my_ipv4_addr1));
		zassert_not_null(ifaddr, "ipv4 addr1");
	}

	/* For testing purposes we need to set the addresses preferred */
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

	maddr = net_if_ipv4_maddr_add(iface1, &inaddr_mcast);
	if (!maddr) {
		DBG("Cannot add multicast IPv4 address %s\n",
		       net_sprint_ipv4_addr(&inaddr_mcast));
		zassert_not_null(maddr, "mcast");
	}

	net_ipv6_addr_create(&in6addr_mcast, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);

	maddr = net_if_ipv6_maddr_add(iface1, &in6addr_mcast);
	if (!maddr) {
		DBG("Cannot add multicast IPv6 address %s\n",
		       net_sprint_ipv6_addr(&in6addr_mcast));
		zassert_not_null(maddr, "mcast");
	}

	net_if_up(iface1);
	net_if_up(iface2);
	net_if_up(iface3);
	net_if_up(iface4);

	/* The interface might receive data which might fail the checks
	 * in the iface sending function, so we need to reset the failure
	 * flag.
	 */
	test_failed = false;

	test_started = true;

	return NULL;
}

static void iface_teardown(void *dummy)
{
	ARG_UNUSED(dummy);
	net_if_ipv6_addr_rm(iface1, &my_addr1);
	net_if_ipv6_addr_rm(iface1, &ll_addr);
	net_if_ipv6_addr_rm(iface2, &my_addr2);
	net_if_ipv6_addr_rm(iface2, &my_addr3);
	net_if_ipv6_maddr_rm(iface1, &in6addr_mcast);
	net_if_down(iface1);
	net_if_down(iface2);
	net_if_down(iface3);
	net_if_down(iface4);
}

static void test_iface_init(struct net_if *iface, bool carrier, bool dormant)
{
	net_if_down(iface);

	if (carrier) {
		net_if_carrier_on(iface);
	} else {
		net_if_carrier_off(iface);
	}

	if (dormant) {
		net_if_dormant_on(iface);
	} else {
		net_if_dormant_off(iface);
	}

	net_if_up(iface);
}

ZTEST(net_iface, test_oper_state)
{
	/* Carrier OFF, Dormant OFF - interface should remain down */
	test_iface_init(iface1, false, false);
	zassert_false(net_if_is_up(iface1), "Interface should be down");
	zassert_equal(net_if_oper_state(iface1), NET_IF_OPER_DOWN,
		      "Wrong operational state");

	/* Carrier ON transition - interface should go up */
	net_if_carrier_on(iface1);
	zassert_true(net_if_is_up(iface1), "Interface should be up");
	zassert_equal(net_if_oper_state(iface1), NET_IF_OPER_UP,
		      "Wrong operational state");

	/* Carrier ON, Dormant ON - interface should remain down */
	test_iface_init(iface1, true, true);
	zassert_false(net_if_is_up(iface1), "Interface should be down");
	zassert_equal(net_if_oper_state(iface1), NET_IF_OPER_DORMANT,
		      "Wrong operational state");

	/* Dormant OFF transition - interface should go up */
	net_if_dormant_off(iface1);
	zassert_true(net_if_is_up(iface1), "Interface should be up");
	zassert_equal(net_if_oper_state(iface1), NET_IF_OPER_UP,
		      "Wrong operational state");

	/* Carrier ON, Dormant OFF - interface should go up right away */
	test_iface_init(iface1, true, false);
	zassert_true(net_if_is_up(iface1), "Interface should be up");
	zassert_equal(net_if_oper_state(iface1), NET_IF_OPER_UP,
		      "Wrong operational state");

	/* Carrier OFF transition - interface should go down */
	net_if_carrier_off(iface1);
	zassert_false(net_if_is_up(iface1), "Interface should be down");
	zassert_equal(net_if_oper_state(iface1), NET_IF_OPER_DOWN,
		      "Wrong operational state");

	/* Carrier ON, Dormant OFF - interface should go up right away */
	test_iface_init(iface1, true, false);
	zassert_true(net_if_is_up(iface1), "Interface should be up");
	zassert_equal(net_if_oper_state(iface1), NET_IF_OPER_UP,
		      "Wrong operational state");

	/* Dormant ON transition - interface should go down */
	net_if_dormant_on(iface1);
	zassert_false(net_if_is_up(iface1), "Interface should be down");
	zassert_equal(net_if_oper_state(iface1), NET_IF_OPER_DORMANT,
		      "Wrong operational state");

	/* Carrier ON, Dormant OFF - interface should go up right away */
	test_iface_init(iface1, true, false);
	zassert_true(net_if_is_up(iface1), "Interface should be up");
	zassert_equal(net_if_oper_state(iface1), NET_IF_OPER_UP,
		      "Wrong operational state");

	/* Admin down transition - interface should go down */
	net_if_down(iface1);
	zassert_false(net_if_is_up(iface1), "Interface should be down");
	zassert_equal(net_if_oper_state(iface1), NET_IF_OPER_DOWN,
		      "Wrong operational state");

	/* Bring the interface back up */
	net_if_up(iface1);
	zassert_true(net_if_is_up(iface1), "Interface should be up");
}

static bool send_iface(struct net_if *iface, int val, bool expect_fail)
{
	static uint8_t data[] = { 't', 'e', 's', 't', '\0' };
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(data),
					AF_UNSPEC, 0, K_FOREVER);
	if (!pkt) {
		DBG("Cannot allocate pkt\n");
		return false;
	}

	net_pkt_write(pkt, data, sizeof(data));
	net_pkt_cursor_init(pkt);

	ret = net_send_data(pkt);
	if (!expect_fail && ret < 0) {
		DBG("Cannot send test packet (%d)\n", ret);
		return false;
	}

	if (!expect_fail && k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		DBG("Timeout while waiting interface %d data\n", val);
		return false;
	}

	return true;
}

ZTEST(net_iface, test_send_iface1)
{
	bool ret;

	DBG("Sending data to iface 1 %p\n", iface1);

	ret = send_iface(iface1, 1, false);

	zassert_true(ret, "iface 1");
}

ZTEST(net_iface, test_send_iface2)
{
	bool ret;

	DBG("Sending data to iface 2 %p\n", iface2);

	ret = send_iface(iface2, 2, false);

	zassert_true(ret, "iface 2");
}

ZTEST(net_iface, test_send_iface3)
{
	bool ret;

	DBG("Sending data to iface 3 %p\n", iface3);

	ret = send_iface(iface3, 3, false);

	zassert_true(ret, "iface 3");
}

static void send_iface1_down(void)
{
	bool ret;

	DBG("Sending data to iface 1 %p while down\n", iface1);

	net_if_down(iface1);

	ret = send_iface(iface1, 1, true);

	zassert_true(ret, "iface 1 down");
}

static void send_iface1_up(void)
{
	bool ret;

	DBG("Sending data to iface 1 %p again\n", iface1);

	net_if_up(iface1);

	ret = send_iface(iface1, 1, false);

	zassert_true(ret, "iface 1 up again");
}

ZTEST(net_iface, test_send_iface1_down_up)
{
	send_iface1_down();
	send_iface1_up();
}

ZTEST(net_iface, test_select_src_iface)
{
	struct in6_addr dst_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x2 } } };
	struct in6_addr ll_addr1 = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
					 0, 0, 0x09, 0x12, 0xaa, 0x29, 0x02,
					 0x88 } } };
	struct in6_addr dst_addr3 = { { { 0x20, 0x01, 0x0d, 0xb8, 3, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x99 } } };
	struct in6_addr in6addr_mcast1 = { { { 0x00 } } };
	struct in_addr dst_addr_2 = { { { 192, 0, 2, 2 } } };

	struct net_if_addr *ifaddr;
	struct net_if *iface;
	struct sockaddr_in ipv4;
	struct sockaddr_in6 ipv6;

	iface = net_if_ipv6_select_src_iface(&dst_addr1);
	zassert_equal_ptr(iface, iface1, "Invalid interface %p vs %p selected",
			  iface, iface1);

	iface = net_if_ipv6_select_src_iface(&ll_addr1);
	zassert_equal_ptr(iface, iface1, "Invalid interface %p vs %p selected",
			  iface, iface1);

	net_ipv6_addr_create(&in6addr_mcast1, 0xff02, 0, 0, 0, 0, 0, 0, 0x0002);

	iface = net_if_ipv6_select_src_iface(&in6addr_mcast1);
	zassert_equal_ptr(iface, iface1, "Invalid interface %p vs %p selected",
			  iface, iface1);

	iface = net_if_ipv6_select_src_iface(&dst_addr3);
	zassert_equal_ptr(iface, iface2, "Invalid interface %p vs %p selected",
			  iface, iface2);

	ifaddr = net_if_ipv6_addr_lookup(&ll_addr, NULL);
	zassert_not_null(ifaddr, "No such ll_addr found");

	ifaddr->addr_state = NET_ADDR_TENTATIVE;

	/* We should now get default interface */
	iface = net_if_ipv6_select_src_iface(&ll_addr1);
	zassert_equal_ptr(iface, net_if_get_default(),
			  "Invalid interface %p vs %p selected",
			  iface, net_if_get_default());

	net_ipaddr_copy(&ipv4.sin_addr, &dst_addr_2);
	ipv4.sin_family = AF_INET;
	ipv4.sin_port = 0U;

	iface = net_if_select_src_iface((struct sockaddr *)&ipv4);
	zassert_equal_ptr(iface, iface1, "Invalid interface %p vs %p selected",
			  iface, iface1);

	net_ipaddr_copy(&ipv6.sin6_addr, &dst_addr1);
	ipv6.sin6_family = AF_INET6;
	ipv6.sin6_port = 0U;

	iface = net_if_select_src_iface((struct sockaddr *)&ipv6);
	zassert_equal_ptr(iface, iface1, "Invalid interface %p vs %p selected",
			  iface, iface1);
}

static void check_promisc_mode_off(void)
{
	bool ret;

	DBG("Make sure promiscuous mode is OFF (%p)\n", iface4);

	ret = net_if_is_promisc(iface4);

	zassert_false(ret, "iface 1 promiscuous mode ON");
}

static void check_promisc_mode_on(void)
{
	bool ret;

	DBG("Make sure promiscuous mode is ON (%p)\n", iface4);

	ret = net_if_is_promisc(iface4);

	zassert_true(ret, "iface 1 promiscuous mode OFF");
}

static void set_promisc_mode_on_again(void)
{
	int ret;

	DBG("Make sure promiscuous mode is ON (%p)\n", iface4);

	ret = net_if_set_promisc(iface4);

	zassert_equal(ret, -EALREADY, "iface 1 promiscuous mode OFF");
}

static void set_promisc_mode_on(void)
{
	bool ret;

	DBG("Setting promiscuous mode ON (%p)\n", iface4);

	ret = net_if_set_promisc(iface4);

	zassert_equal(ret, 0, "iface 1 promiscuous mode set failed");
}

static void set_promisc_mode_off(void)
{
	DBG("Setting promiscuous mode OFF (%p)\n", iface4);

	net_if_unset_promisc(iface4);
}

ZTEST(net_iface, test_promisc_mode)
{
	check_promisc_mode_off();
	set_promisc_mode_on();
	check_promisc_mode_on();
	set_promisc_mode_on_again();
	set_promisc_mode_off();
	check_promisc_mode_off();
}

static struct in_addr my_ipv4_addr_test = { { { 10, 0, 0, 1 } } };
static struct in_addr my_ipv4_addr_not_found = { { { 1, 2, 3, 4 } } };

static void v4_addr_add(void)
{
	bool ret;

	ret = net_if_ipv4_addr_add_by_index(1, &my_ipv4_addr_test,
					    NET_ADDR_MANUAL, 0);
	zassert_true(ret, "Cannot add IPv4 address");
}

static void v4_addr_lookup(void)
{
	int ret;

	ret = net_if_ipv4_addr_lookup_by_index(&my_ipv4_addr_test);
	zassert_equal(ret, 1, "IPv4 address not found");

	ret = net_if_ipv4_addr_lookup_by_index(&my_ipv4_addr_not_found);
	zassert_not_equal(ret, 1, "IPv4 address found");
}

static void v4_addr_rm(void)
{
	bool ret;

	ret = net_if_ipv4_addr_rm_by_index(1, &my_ipv4_addr_test);
	zassert_true(ret, "Cannot remove IPv4 address");
}

ZTEST(net_iface, test_v4_addr_add_rm)
{
	v4_addr_add();
	v4_addr_lookup();
	v4_addr_rm();
}

#define MY_ADDR_V4_USER      { { { 10, 0, 0, 2 } } }
#define UNKNOWN_ADDR_V4_USER { { { 5, 6, 7, 8 } } }

static void v4_addr_add_user(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct in_addr my_addr = MY_ADDR_V4_USER;
	bool ret;

	ret = net_if_ipv4_addr_add_by_index(1, &my_addr, NET_ADDR_MANUAL, 0);
	zassert_true(ret, "Could not add IPv4 address");
}

static void v4_addr_add_user_from_userspace(void)
{
	k_thread_access_grant(k_current_get(), net_if_get_by_index(1));
	k_thread_user_mode_enter(v4_addr_add_user, NULL,
				 NULL, NULL);
}

static void v4_addr_lookup_user(void)
{
	struct in_addr my_addr = MY_ADDR_V4_USER;
	struct in_addr unknown_addr = UNKNOWN_ADDR_V4_USER;
	int ret;

	ret = net_if_ipv4_addr_lookup_by_index(&my_addr);
	zassert_equal(ret, 1, "IPv4 address not found (%d)", ret);

	ret = net_if_ipv4_addr_lookup_by_index(&unknown_addr);
	zassert_equal(ret, 0, "IPv4 address found");
}

static void v4_addr_rm_user(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct in_addr my_addr = MY_ADDR_V4_USER;
	bool ret;

	ret = net_if_ipv4_addr_rm_by_index(1, &my_addr);
	zassert_true(ret, "Cannot remove IPv4 address");
}

static void v4_addr_rm_user_from_userspace(void)
{
	k_thread_access_grant(k_current_get(), net_if_get_by_index(1));
	k_thread_user_mode_enter(v4_addr_rm_user, NULL,
				 NULL, NULL);
}

ZTEST(net_iface, test_v4_addr_add_rm_user_from_userspace)
{
	v4_addr_add_user_from_userspace();
	v4_addr_lookup_user();
	v4_addr_rm_user_from_userspace();
}

static
struct in6_addr my_ipv6_addr_test = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

static
struct in6_addr my_ipv6_addr_not_found = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0,
					    0, 0, 0, 0, 0, 0, 0, 0, 0x64 } } };

static void v6_addr_add(void)
{
	bool ret;

	ret = net_if_ipv6_addr_add_by_index(1, &my_ipv6_addr_test,
					    NET_ADDR_MANUAL, 0);
	zassert_true(ret, "Cannot add IPv6 address");
}

static void v6_addr_add_mcast_twice(void)
{
	struct net_if_mcast_addr *maddr;

	maddr = net_if_ipv6_maddr_add(iface1, &in6addr_mcast);
	zassert_equal(maddr, NULL, "Address was added twice");
}

static void v6_addr_lookup(void)
{
	int ret;

	ret = net_if_ipv6_addr_lookup_by_index(&my_ipv6_addr_test);
	zassert_equal(ret, 1, "IPv6 address not found");

	ret = net_if_ipv6_addr_lookup_by_index(&my_ipv6_addr_not_found);
	zassert_not_equal(ret, 1, "IPv6 address found");
}

static void v6_addr_rm(void)
{
	bool ret;

	ret = net_if_ipv6_addr_rm_by_index(1, &my_ipv6_addr_test);
	zassert_true(ret, "Cannot remove IPv6 address");
}

ZTEST(net_iface, test_v6_addr_add_rm)
{
	v6_addr_add();
	v6_addr_add_mcast_twice();
	v6_addr_lookup();
	v6_addr_rm();
}

ZTEST(net_iface, test_v6_addr_add_rm_solicited)
{
	const struct in6_addr prefix = { { { 0x20, 0x01, 0x1b, 0x98, 0x24, 0xb8, 0x7e, 0xbb,
					     0, 0, 0, 0, 0, 0, 0, 0 } } };
	struct in6_addr iid_addr = { };
	struct in6_addr iid_addr_mcast = { };
	struct in6_addr unicast_addr = { };
	struct in6_addr unicast_addr_mcast = { };
	struct net_if_addr *ifaddr;
	struct net_if_mcast_addr *maddr;
	bool ret;

	/* Add a link-local address based on the interface identifier */
	net_ipv6_addr_create_iid(&iid_addr, net_if_get_link_addr(iface4));
	ifaddr = net_if_ipv6_addr_add(iface4, &iid_addr,
				      NET_ADDR_AUTOCONF, 0);
	zassert_not_null(ifaddr, "Cannot add IPv6 link-local address");

	/* Add the corresponding solicited-node multicast address */
	net_ipv6_addr_create_solicited_node(&iid_addr, &iid_addr_mcast);
	maddr = net_if_ipv6_maddr_add(iface4, &iid_addr_mcast);
	zassert_not_null(maddr, "Cannot add solicited-node multicast address");

	/* Add an autoconfigured global unicast address */
	net_ipv6_addr_create_iid(&unicast_addr, net_if_get_link_addr(iface4));
	memcpy(&unicast_addr, &prefix, sizeof(prefix) / 2);
	ifaddr = net_if_ipv6_addr_add(iface4, &unicast_addr,
				      NET_ADDR_AUTOCONF, 0);
	zassert_not_null(ifaddr, "Cannot add IPv6 global unicast address");

	/* Add the corresponding solicited-node multicast address (should exist) */
	net_ipv6_addr_create_solicited_node(&unicast_addr, &unicast_addr_mcast);
	zassert_mem_equal(&unicast_addr_mcast, &iid_addr_mcast,
			  sizeof(struct in6_addr));
	maddr = net_if_ipv6_maddr_add(iface4, &unicast_addr_mcast);
	zassert_is_null(maddr, "Solicited-node multicast address was added twice");

	/* Remove the global unicast address */
	ret = net_if_ipv6_addr_rm(iface4, &unicast_addr);
	zassert_true(ret, "Cannot remove IPv6 global unicast address");

	/* The solicited-node multicast address should stay */
	maddr = net_if_ipv6_maddr_lookup(&iid_addr_mcast, &iface4);
	zassert_not_null(maddr, "Solicited-node multicast address was removed");

	/* Remove the link-local address */
	ret = net_if_ipv6_addr_rm(iface4, &iid_addr);
	zassert_true(ret, "Cannot remove IPv6 link-local address");

	/* The solicited-node multicast address should be gone */
	maddr = net_if_ipv6_maddr_lookup(&iid_addr_mcast, &iface4);
	zassert_is_null(maddr, "Solicited-node multicast address was not removed");
}

#define MY_ADDR_V6_USER { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, \
			      0, 0, 0, 0, 0, 0, 0, 0x65 } } }

#define UNKNOWN_ADDR_V6_USER { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, \
			      0, 0, 0, 0, 0, 0, 0, 0x66 } } }

static void v6_addr_add_user(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct in6_addr my_addr = MY_ADDR_V6_USER;
	bool ret;

	ret = net_if_ipv6_addr_add_by_index(1, &my_addr, NET_ADDR_MANUAL, 0);
	zassert_true(ret, "Could not add IPv6 address");
}

static void v6_addr_add_user_from_userspace(void)
{
	k_thread_access_grant(k_current_get(), net_if_get_by_index(1));
	k_thread_user_mode_enter(v6_addr_add_user, NULL,
				 NULL, NULL);
}

static void v6_addr_lookup_user(void)
{
	struct in6_addr my_addr = MY_ADDR_V6_USER;
	struct in6_addr unknown_addr = UNKNOWN_ADDR_V6_USER;
	int ret;

	ret = net_if_ipv6_addr_lookup_by_index(&my_addr);
	zassert_equal(ret, 1, "IPv6 address not found (%d)", ret);

	ret = net_if_ipv6_addr_lookup_by_index(&unknown_addr);
	zassert_equal(ret, 0, "IPv6 address found");
}

static void v6_addr_rm_user(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct in6_addr my_addr = MY_ADDR_V6_USER;
	bool ret;

	/* Check also that add is enabled so that we can remove something
	 * that was already added.
	 */
	ret = net_if_ipv6_addr_rm_by_index(1, &my_addr);
	zassert_true(ret, "Cannot remove IPv6 address");
}

static void v6_addr_rm_user_from_userspace(void)
{
	k_thread_access_grant(k_current_get(), net_if_get_by_index(1));
	k_thread_user_mode_enter(v6_addr_rm_user, NULL,
				 NULL, NULL);
}

ZTEST(net_iface, test_v6_addr_add_rm_user_from_userspace)
{
	v6_addr_add_user_from_userspace();
	v6_addr_lookup_user();
	v6_addr_rm_user_from_userspace();
}

static void netmask_addr_add(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct in_addr my_netmask = { { { 255, 255, 255, 0 } } };
	bool ret;

	ret = net_if_ipv4_set_netmask_by_addr_by_index(1, &my_ipv4_addr1, &my_netmask);
	zassert_true(ret, "Cannot add IPv4 netmask");
}

ZTEST(net_iface, test_netmask_addr_add)
{
	netmask_addr_add(NULL, NULL, NULL);
}

static void netmask_addr_add_from_userspace(void)
{
	k_thread_access_grant(k_current_get(), net_if_get_by_index(1));
	k_thread_user_mode_enter(netmask_addr_add, NULL,
				 NULL, NULL);
}

ZTEST(net_iface, test_netmask_addr_add_from_userspace)
{
	netmask_addr_add_from_userspace();
}

static void gw_addr_add(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct in_addr my_gw = { { { 192, 0, 2, 254 } } };
	bool ret;

	ret = net_if_ipv4_set_gw_by_index(1, &my_gw);
	zassert_true(ret, "Cannot add IPv4 gateway");
}

ZTEST(net_iface, test_gw_addr_add)
{
	gw_addr_add(NULL, NULL, NULL);
}

static void gw_addr_add_from_userspace(void)
{
	k_thread_access_grant(k_current_get(), net_if_get_by_index(1));
	k_thread_user_mode_enter(gw_addr_add, NULL,
				 NULL, NULL);
}

ZTEST(net_iface, test_gw_addr_add_from_userspace)
{
	gw_addr_add_from_userspace();
}

static void get_by_index(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	zassert_not_null(net_if_get_by_index(1),
			 "Cannot get interface at index 1");
}

ZTEST(net_iface, test_get_by_index)
{
	get_by_index(NULL, NULL, NULL);
}

static void get_by_index_from_userspace(void)
{
	k_thread_access_grant(k_current_get(), net_if_get_by_index(1));
	k_thread_user_mode_enter(get_by_index, NULL,
				 NULL, NULL);
}

ZTEST(net_iface, test_get_by_index_from_userspace)
{
	get_by_index_from_userspace();
}

static void foreach_ipv4_addr_check(struct net_if *iface,
				    struct net_if_addr *if_addr,
				    void *user_data)
{
	int *count = (int *)user_data;

	(*count)++;

	zassert_equal_ptr(iface, iface1, "Callback called on wrong interface");
	zassert_mem_equal(&if_addr->address.in_addr, &my_ipv4_addr1,
			  sizeof(struct in_addr), "Wrong IPv4 address");
}

ZTEST(net_iface, test_ipv4_addr_foreach)
{
	int count = 0;

	/* iface1 has one IPv4 address configured */
	net_if_ipv4_addr_foreach(iface1, foreach_ipv4_addr_check, &count);
	zassert_equal(count, 1, "Incorrect number of callback calls");

	count = 0;

	/* iface4 has no IPv4 address configured */
	net_if_ipv4_addr_foreach(iface4, foreach_ipv4_addr_check, &count);
	zassert_equal(count, 0, "Incorrect number of callback calls");
}

static void foreach_ipv4_maddr_check(struct net_if *iface,
				     struct net_if_mcast_addr *if_addr,
				     void *user_data)
{
	int *count = (int *)user_data;

	(*count)++;

	zassert_equal_ptr(iface, iface1, "Callback called on wrong interface");
	zassert_mem_equal(&if_addr->address.in_addr, &inaddr_mcast,
			  sizeof(struct in_addr), "Wrong IPv4 multicast address");
}

ZTEST(net_iface, test_ipv4_maddr_foreach)
{
	int count = 0;

	/* iface1 has one IPv4 multicast address configured */
	net_if_ipv4_maddr_foreach(iface1, foreach_ipv4_maddr_check, &count);
	zassert_equal(count, 1, "Incorrect number of callback calls");

	count = 0;

	/* iface4 has no IPv4 multicast address configured */
	net_if_ipv4_maddr_foreach(iface4, foreach_ipv4_maddr_check, &count);
	zassert_equal(count, 0, "Incorrect number of callback calls");
}

static void foreach_ipv6_addr_check(struct net_if *iface,
				    struct net_if_addr *if_addr,
				    void *user_data)
{
	int *count = (int *)user_data;

	(*count)++;

	zassert_equal_ptr(iface, iface1, "Callback called on wrong interface");

	if (net_ipv6_is_ll_addr(&if_addr->address.in6_addr)) {
		zassert_mem_equal(&if_addr->address.in6_addr, &ll_addr,
				  sizeof(struct in6_addr), "Wrong IPv6 address");
	} else {
		zassert_mem_equal(&if_addr->address.in6_addr, &my_addr1,
				  sizeof(struct in6_addr), "Wrong IPv6 address");
	}
}

ZTEST(net_iface, test_ipv6_addr_foreach)
{
	int count = 0;

	/* iface1 has two IPv6 addresses configured */
	net_if_ipv6_addr_foreach(iface1, foreach_ipv6_addr_check, &count);
	zassert_equal(count, 2, "Incorrect number of callback calls");

	count = 0;

	/* iface4 has no IPv6 address configured */
	net_if_ipv6_addr_foreach(iface4, foreach_ipv6_addr_check, &count);
	zassert_equal(count, 0, "Incorrect number of callback calls");
}

static void foreach_ipv6_maddr_check(struct net_if *iface,
				     struct net_if_mcast_addr *if_addr,
				     void *user_data)
{
	int *count = (int *)user_data;

	(*count)++;

	zassert_equal_ptr(iface, iface1, "Callback called on wrong interface");
	zassert_mem_equal(&if_addr->address.in6_addr, &in6addr_mcast, sizeof(struct in6_addr),
			  "Wrong IPv6 multicast address");
}

ZTEST(net_iface, test_ipv6_maddr_foreach)
{
	int count = 0;

	/* iface1 has one IPv6 multicast address configured */
	net_if_ipv6_maddr_foreach(iface1, foreach_ipv6_maddr_check, &count);
	zassert_equal(count, 1, "Incorrect number of callback calls");

	count = 0;

	/* iface4 has no IPv6 multicast address configured */
	net_if_ipv6_maddr_foreach(iface4, foreach_ipv6_maddr_check, &count);
	zassert_equal(count, 0, "Incorrect number of callback calls");
}

ZTEST(net_iface, test_interface_name)
{
	int ret;

#if defined(CONFIG_NET_INTERFACE_NAME)
	char buf[CONFIG_NET_INTERFACE_NAME_LEN + 1];
	struct net_if *iface;
	char *name;

	iface = net_if_get_default();
	memset(buf, 0, sizeof(buf));

	ret = net_if_get_name(NULL, NULL, -1);
	zassert_equal(ret, -EINVAL, "Unexpected value returned");

	ret = net_if_get_name(iface, NULL, -1);
	zassert_equal(ret, -EINVAL, "Unexpected value returned");

	ret = net_if_get_name(iface, buf, 0);
	zassert_equal(ret, -EINVAL, "Unexpected value returned");

	name = "mynetworkiface0";
	ret = net_if_set_name(iface, name);
	zassert_equal(ret, -ENAMETOOLONG, "Unexpected value (%d) returned", ret);

	name = "abc0";
	ret = net_if_set_name(iface, name);
	zassert_equal(ret, 0, "Unexpected value (%d) returned", ret);

	name = "abc0";
	ret = net_if_set_name(iface2, name);
	zassert_equal(ret, -EALREADY, "Unexpected value (%d) returned", ret);

	name = "abc";
	ret = net_if_set_name(iface2, name);
	zassert_equal(ret, 0, "Unexpected value (%d) returned", ret);

	name = "abc0";

	ret = net_if_get_name(iface, buf, 1);
	zassert_equal(ret, -ERANGE, "Unexpected value (%d) returned", ret);

	ret = net_if_get_name(iface, buf, strlen(name) - 1);
	zassert_equal(ret, -ERANGE, "Unexpected value (%d) returned", ret);

	ret = net_if_get_name(iface, buf, sizeof(buf) - 1);
	zassert_equal(ret, strlen(name), "Unexpected value (%d) returned, expected %d",
		      ret, strlen(name));

	ret = net_if_get_by_name(name);
	zassert_equal(ret, net_if_get_by_iface(iface), "Unexpected value (%d) returned", ret);

	ret = net_if_get_by_name("ENOENT");
	zassert_equal(ret, -ENOENT, "Unexpected value (%d) returned", ret);
#else
	ret = net_if_get_name(NULL, NULL, -1);
	zassert_equal(ret, -ENOTSUP, "Invalid value returned");
#endif
}

ZTEST_SUITE(net_iface, NULL, iface_setup, NULL, NULL, iface_teardown);
