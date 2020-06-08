/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <sys/mutex.h>
#include <ztest_assert.h>

#include <net/socket.h>
#include <net/ethernet.h>

#if defined(CONFIG_NET_SOCKETS_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static u8_t lladdr1[] = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
static u8_t lladdr2[] = { 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 };

struct eth_fake_context {
	struct net_if *iface;
	u8_t *mac_address;
};

static struct eth_fake_context eth_fake_data1 = {
	.mac_address = lladdr1
};
static struct eth_fake_context eth_fake_data2 = {
	.mac_address = lladdr2
};

static int eth_fake_send(struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static void eth_fake_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->driver_data;

	ctx->iface = iface;

	net_if_set_link_addr(iface, ctx->mac_address, 6, NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static struct ethernet_api eth_fake_api_funcs = {
	.iface_api.init = eth_fake_iface_init,
	.send = eth_fake_send,
};

static int eth_fake_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

ETH_NET_DEVICE_INIT(eth_fake1, "eth_fake1", eth_fake_init,
		    device_pm_control_nop, &eth_fake_data1, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_fake_api_funcs,
		    NET_ETH_MTU);

ETH_NET_DEVICE_INIT(eth_fake2, "eth_fake2", eth_fake_init,
		    device_pm_control_nop, &eth_fake_data2, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_fake_api_funcs,
		    NET_ETH_MTU);

static int setup_socket(struct net_if *iface)
{
	int sock;

	sock = socket(AF_PACKET, SOCK_RAW, ETH_P_ALL);
	zassert_true(sock >= 0, "Cannot create packet socket (%d)", sock);

	return sock;
}

static int bind_socket(int sock, struct net_if *iface)
{
	struct sockaddr_ll addr;

	memset(&addr, 0, sizeof(addr));

	addr.sll_ifindex = net_if_get_by_iface(iface);
	addr.sll_family = AF_PACKET;

	return bind(sock, (struct sockaddr *)&addr, sizeof(addr));
}

struct user_data {
	struct net_if *first;
	struct net_if *second;
};

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct user_data *ud = user_data;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (ud->first == NULL) {
		ud->first = iface;
		return;
	}

	ud->second = iface;
}

static void test_packet_sockets(void)
{
	struct user_data ud = { 0 };
	int ret, sock1, sock2;

	net_if_foreach(iface_cb, &ud);

	zassert_not_null(ud.first, "1st Ethernet interface not found");
	zassert_not_null(ud.second, "2nd Ethernet interface not found");

	sock1 = setup_socket(ud.first);
	zassert_true(sock1 >= 0, "Cannot create 1st socket (%d)", sock1);

	sock2 = setup_socket(ud.second);
	zassert_true(sock2 >= 0, "Cannot create 2nd socket (%d)", sock2);

	ret = bind_socket(sock1, ud.first);
	zassert_equal(ret, 0, "Cannot bind 1st socket (%d)", -errno);

	ret = bind_socket(sock2, ud.second);
	zassert_equal(ret, 0, "Cannot bind 2nd socket (%d)", -errno);
}

void test_main(void)
{
	ztest_test_suite(socket_packet,
			 ztest_unit_test(test_packet_sockets));
	ztest_run_test_suite(socket_packet);
}
