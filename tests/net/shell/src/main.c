/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_UDP_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/zephyr.h>
#include <zephyr/linker/sections.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/buf.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/udp.h>
#include <zephyr/random/rand32.h>

#include "ipv4.h"
#include "ipv6.h"

#include <ztest.h>

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#include "udp_internal.h"

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define NET_LOG_ENABLED 1
#endif
#include "net_private.h"
#include "ipv4.h"

static bool test_failed;
static struct k_sem recv_lock;

struct net_udp_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_udp_dev_init(const struct device *dev)
{
	struct net_udp_context *net_udp_context = dev->data;

	net_udp_context = net_udp_context;

	return 0;
}

static uint8_t *net_udp_get_mac(const struct device *dev)
{
	struct net_udp_context *context = dev->data;

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

static void net_udp_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_udp_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);
}

static int send_status = -EINVAL;

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	if (!pkt->frags) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	DBG("Data was sent successfully\n");

	send_status = 0;

	return 0;
}

static inline struct in_addr *if_get_addr(struct net_if *iface)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (iface->config.ip.ipv4->unicast[i].is_used &&
		    iface->config.ip.ipv4->unicast[i].address.family ==
								AF_INET &&
		    iface->config.ip.ipv4->unicast[i].addr_state ==
							NET_ADDR_PREFERRED) {
			return
			    &iface->config.ip.ipv4->unicast[i].address.in_addr;
		}
	}

	return NULL;
}

struct net_udp_context net_udp_context_data;

static struct dummy_api net_udp_if_api = {
	.iface_api.init = net_udp_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(net_udp_test, "net_udp_test",
		net_udp_dev_init, NULL,
		&net_udp_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_udp_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE, 127);

static void *test_setup(void)
{
	struct net_if *iface;
	struct net_if_addr *ifaddr;

	struct sockaddr_in6 any_addr6;
	const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;

	struct sockaddr_in6 my_addr6;
	struct in6_addr in6addr_my = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0x1 } } };

	struct sockaddr_in6 peer_addr6;
	struct in6_addr in6addr_peer = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0x4e, 0x11, 0, 0, 0x2 } } };

	struct sockaddr_in any_addr4;
	const struct in_addr in4addr_any = { { { 0 } } };

	struct sockaddr_in my_addr4;
	struct in_addr in4addr_my = { { { 192, 0, 2, 1 } } };

	struct sockaddr_in peer_addr4;
	struct in_addr in4addr_peer = { { { 192, 0, 2, 9 } } };

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	test_failed = false;

	net_ipaddr_copy(&any_addr6.sin6_addr, &in6addr_any);
	any_addr6.sin6_family = AF_INET6;

	net_ipaddr_copy(&my_addr6.sin6_addr, &in6addr_my);
	my_addr6.sin6_family = AF_INET6;

	net_ipaddr_copy(&peer_addr6.sin6_addr, &in6addr_peer);
	peer_addr6.sin6_family = AF_INET6;

	net_ipaddr_copy(&any_addr4.sin_addr, &in4addr_any);
	any_addr4.sin_family = AF_INET;

	net_ipaddr_copy(&my_addr4.sin_addr, &in4addr_my);
	my_addr4.sin_family = AF_INET;

	net_ipaddr_copy(&peer_addr4.sin_addr, &in4addr_peer);
	peer_addr4.sin_family = AF_INET;

	k_sem_init(&recv_lock, 0, UINT_MAX);

	ifaddr = net_if_ipv6_addr_add(iface, &in6addr_my, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		printk("Cannot add %s to interface %p\n",
		       net_sprint_ipv6_addr(&in6addr_my), iface);
		zassert_true(0, "exiting");
	}

	ifaddr = net_if_ipv4_addr_add(iface, &in4addr_my, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		printk("Cannot add %s to interface %p\n",
		       net_sprint_ipv4_addr(&in4addr_my), iface);
		zassert_true(0, "exiting");
	}

	return NULL;
}

ZTEST(net_shell_test_suite, test_net_shell)
{
	int ret;

	/* Test that command exists */
	ret = shell_execute_cmd(NULL, "net iface");
	zassert_equal(ret, 0, "");

	/* There is no foobar command */
	ret = shell_execute_cmd(NULL, "net foobar");
	zassert_equal(ret, 1, "");
}

ZTEST_SUITE(net_shell_test_suite, NULL, test_setup, NULL, NULL, NULL);
