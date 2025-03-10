/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <zephyr/ztest.h>
#include <zephyr/random/random.h>

#include <zephyr/posix/fcntl.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>

struct fake_dev_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_if *iface;
};

static int fake_dev_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return -ENETDOWN;
}

static uint8_t *fake_dev_get_mac(struct fake_dev_context *ctx)
{
	if (ctx->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		ctx->mac_addr[0] = 0x00;
		ctx->mac_addr[1] = 0x00;
		ctx->mac_addr[2] = 0x5E;
		ctx->mac_addr[3] = 0x00;
		ctx->mac_addr[4] = 0x53;
		ctx->mac_addr[5] = sys_rand8_get();
	}

	return ctx->mac_addr;
}

static void fake_dev_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct fake_dev_context *ctx = dev->data;
	uint8_t *mac = fake_dev_get_mac(ctx);

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);

	ctx->iface = iface;
}

int fake_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

struct fake_dev_context fake_dev_context_data;

static struct dummy_api fake_dev_if_api = {
	.iface_api.init = fake_dev_iface_init,
	.send = fake_dev_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(fake_dev, "fake_dev", fake_dev_init, NULL, &fake_dev_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &fake_dev_if_api, _ETH_L2_LAYER,
		_ETH_L2_CTX_TYPE, 127);

static void *test_setup(void)
{
	struct net_if *iface;
	struct in_addr in4addr_my = { { { 192, 168, 0, 2 } } };
	struct net_if_addr *ifaddr;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Could not get dummy iface");

	net_if_up(iface);

	ifaddr = net_if_ipv4_addr_add(iface, &in4addr_my, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Could not add iface address");

	return NULL;
}

ZTEST(net_sock_packet_raw_ip, test_sock_raw_packet_raw_ip)
{
	int sock;

	sock = zsock_socket(AF_PACKET, SOCK_RAW, htons(IPPROTO_RAW));
	zassert_true(sock < 0, "Could create a socket");

	zsock_close(sock);
}

ZTEST_SUITE(net_sock_packet_raw_ip, NULL, test_setup, NULL, NULL, NULL);
