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

#include <fcntl.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>

struct fake_dev_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_if *iface;
};

static const char testing_data[] = "Tappara";

static int fake_dev_send(const struct device *dev, struct net_pkt *pkt)
{
	struct net_pkt *recv_pkt;
	int ret;

	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	/* Loopback the data back to stack: */
	NET_DBG("Dummy device: Loopbacking data (%zd bytes) to iface %d\n", net_pkt_get_len(pkt),
	    net_if_get_by_iface(net_pkt_iface(pkt)));

	recv_pkt = net_pkt_clone(pkt, K_NO_WAIT);

	k_sleep(K_MSEC(10)); /* Let the receiver run */

	ret = net_recv_data(net_pkt_iface(recv_pkt), recv_pkt);
	zassert_equal(ret, 0, "Cannot receive data (%d)", ret);
	return 0;
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
		ctx->mac_addr[5] = sys_rand32_get();
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

ZTEST(net_sckt_packet_raw_ip, test_sckt_raw_packet_raw_ip)
{
	/* A test case for testing socket combo: AF_PACKET & SOCK_RAW & IPPROTO_RAW: */
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	int recv_data_len, ret;
	struct sockaddr_ll dst = { 0 };
	char receive_buffer[128];
	int sock;

	sock = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
	zassert_true(sock >= 0, "Could not create a socket");

	dst.sll_ifindex = net_if_get_by_iface(iface);
	dst.sll_family = AF_PACKET;

	ret = bind(sock, (const struct sockaddr *)&dst, sizeof(struct sockaddr_ll));
	zassert_true(ret >= 0, "Could not bind the socket");

	/* Let's send some data: */
	ret = sendto(sock, testing_data, ARRAY_SIZE(testing_data), 0, (const struct sockaddr *)&dst,
		     sizeof(struct sockaddr_ll));
	zassert_true(ret > 0, "Could not send data");

	/* Receive the same data back: */
	recv_data_len = recv(sock, receive_buffer, sizeof(receive_buffer), 0);
	zassert_true(recv_data_len == ARRAY_SIZE(testing_data), "Expected data not received");

	NET_DBG("Received successfully data %s", receive_buffer);

	close(sock);
}

ZTEST_SUITE(net_sckt_packet_raw_ip, NULL, test_setup, NULL, NULL, NULL);
