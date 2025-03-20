/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_L2_ETHERNET_LOG_LEVEL

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

#include <zephyr/net_buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_l2.h>

#include "ipv6.h"

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#define TEST_PORT 9999

static char *test_data = "Test data to be sent";

/* Interface 1 addresses */
static struct in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Interface 2 addresses */
static struct in6_addr my_addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Destination address for test packets */
static struct in6_addr dst_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 9, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Extra address is assigned to ll_addr */
static struct in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
				       0x04 } } };

/* Keep track of all ethernet interfaces */
static struct net_if *eth_interfaces[2];

static struct net_context *udp_v6_ctx;

static bool test_failed;
static bool test_started;
static bool do_timestamp;
static bool timestamp_cb_called;
static struct net_if_timestamp_cb timestamp_cb;
static struct net_if_timestamp_cb timestamp_cb_2;
static struct net_if_timestamp_cb timestamp_cb_3;

static K_SEM_DEFINE(wait_data, 0, UINT_MAX);

#define WAIT_TIME K_SECONDS(1)

struct eth_context {
	struct net_if *iface;
	uint8_t mac_addr[6];
};

static struct eth_context eth_context;
static struct eth_context eth_context2;

static void eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_context *context = dev->data;

	net_if_set_link_addr(iface, context->mac_addr,
			     sizeof(context->mac_addr),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	if (!pkt->buffer) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	if (test_started) {
		if (do_timestamp) {
			/* Simulate the clock advancing */
			pkt->timestamp.nanosecond = pkt->timestamp.second + 1;

			net_if_add_tx_timestamp(pkt);
		} else {
			k_sem_give(&wait_data);
		}
	}

	test_started = false;

	return 0;
}

static enum ethernet_hw_caps eth_get_capabilities(const struct device *dev)
{
	return 0;
}

static struct ethernet_api api_funcs = {
	.iface_api.init = eth_iface_init,

	.get_capabilities = eth_get_capabilities,
	.send = eth_tx,
};

static void generate_mac(uint8_t *mac_addr)
{
	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	mac_addr[0] = 0x00;
	mac_addr[1] = 0x00;
	mac_addr[2] = 0x5E;
	mac_addr[3] = 0x00;
	mac_addr[4] = 0x53;
	mac_addr[5] = sys_rand8_get();
}

static int eth_init(const struct device *dev)
{
	struct eth_context *context = dev->data;

	generate_mac(context->mac_addr);

	return 0;
}

ETH_NET_DEVICE_INIT(eth_test, "eth_test", eth_init, NULL,
		    &eth_context, NULL, CONFIG_ETH_INIT_PRIORITY, &api_funcs,
		    NET_ETH_MTU);

ETH_NET_DEVICE_INIT(eth_test2, "eth_test2", eth_init, NULL,
		    &eth_context2, NULL, CONFIG_ETH_INIT_PRIORITY, &api_funcs,
		    NET_ETH_MTU);

static void timestamp_callback(struct net_pkt *pkt)
{
	timestamp_cb_called = true;

	if (do_timestamp) {
		/* This is very artificial test but make sure that we
		 * have advanced the time a bit.
		 */
		zassert_true(pkt->timestamp.nanosecond > pkt->timestamp.second,
			     "Timestamp not working ok (%d < %d)\n",
			     pkt->timestamp.nanosecond, pkt->timestamp.second);
	}

	/* The pkt was ref'ed in send_some_data()() */
	net_pkt_unref(pkt);

	if (do_timestamp) {
		k_sem_give(&wait_data);
	}
}

void test_timestamp_setup(void)
{
	struct net_if *iface;
	struct net_pkt *pkt;

	iface = eth_interfaces[0];

	net_if_register_timestamp_cb(&timestamp_cb, NULL, iface,
				     timestamp_callback);

	timestamp_cb_called = false;
	do_timestamp = false;

	pkt = net_pkt_alloc_on_iface(iface, K_FOREVER);

	/* Make sure that the callback function is called */
	net_if_call_timestamp_cb(pkt);

	zassert_true(timestamp_cb_called, "Timestamp callback not called\n");
	zassert_equal(atomic_get(&pkt->atomic_ref), 0, "Pkt %p not released\n");
}

static void timestamp_callback_2(struct net_pkt *pkt)
{
	timestamp_cb_called = true;

	if (do_timestamp) {
		/* This is very artificial test but make sure that we
		 * have advanced the time a bit.
		 */
		zassert_true(pkt->timestamp.nanosecond > pkt->timestamp.second,
			     "Timestamp not working ok (%d < %d)\n",
			     pkt->timestamp.nanosecond, pkt->timestamp.second);
	}

	zassert_equal(eth_interfaces[1], net_pkt_iface(pkt),
		      "Invalid interface");

	/* The pkt was ref'ed in send_some_data()() */
	net_pkt_unref(pkt);

	if (do_timestamp) {
		k_sem_give(&wait_data);
	}
}

void test_timestamp_setup_2nd_iface(void)
{
	struct net_if *iface;
	struct net_pkt *pkt;

	iface = eth_interfaces[1];

	net_if_register_timestamp_cb(&timestamp_cb_2, NULL, iface,
				     timestamp_callback_2);

	timestamp_cb_called = false;
	do_timestamp = false;

	pkt = net_pkt_alloc_on_iface(iface, K_FOREVER);

	/* Make sure that the callback function is called */
	net_if_call_timestamp_cb(pkt);

	zassert_true(timestamp_cb_called, "Timestamp callback not called\n");
	zassert_equal(atomic_get(&pkt->atomic_ref), 0, "Pkt %p not released\n");
}

void test_timestamp_setup_all(void)
{
	struct net_pkt *pkt;

	net_if_register_timestamp_cb(&timestamp_cb_3, NULL, NULL,
				     timestamp_callback);

	timestamp_cb_called = false;
	do_timestamp = false;

	pkt = net_pkt_alloc_on_iface(eth_interfaces[0], K_FOREVER);

	/* The callback is called twice because we have two matching callbacks
	 * as the interface is set to NULL when registering cb. So we need to
	 * ref the pkt here because the callback releases pkt.
	 */
	net_pkt_ref(pkt);

	/* Make sure that the callback function is called */
	net_if_call_timestamp_cb(pkt);

	zassert_true(timestamp_cb_called, "Timestamp callback not called\n");
	zassert_equal(atomic_get(&pkt->atomic_ref), 0, "Pkt %p not released\n");

	net_if_unregister_timestamp_cb(&timestamp_cb_3);
}

void test_timestamp_cleanup(void)
{
	struct net_if *iface;
	struct net_pkt *pkt;

	net_if_unregister_timestamp_cb(&timestamp_cb);

	iface = eth_interfaces[0];

	timestamp_cb_called = false;
	do_timestamp = false;

	pkt = net_pkt_alloc_on_iface(iface, K_FOREVER);

	/* Make sure that the callback function is not called after unregister
	 */
	net_if_call_timestamp_cb(pkt);

	zassert_false(timestamp_cb_called, "Timestamp callback called\n");
	zassert_false(atomic_get(&pkt->atomic_ref) < 1, "Pkt %p released\n");

	net_pkt_unref(pkt);
}

struct user_data {
	int eth_if_count;
	int total_if_count;
};

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static const char *iface2str(struct net_if *iface)
{
#ifdef CONFIG_NET_L2_ETHERNET
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		return "Ethernet";
	}
#endif

	return "<unknown type>";
}
#endif

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct user_data *ud = user_data;

	DBG("Interface %p (%s) [%d]\n", iface, iface2str(iface),
	    net_if_get_by_iface(iface));

	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		if (ud->eth_if_count >= ARRAY_SIZE(eth_interfaces)) {
			DBG("Invalid interface %p\n", iface);
			return;
		}

		eth_interfaces[ud->eth_if_count++] = iface;
	}

	/* By default all interfaces are down initially */
	net_if_down(iface);

	ud->total_if_count++;
}

void test_address_setup(void)
{
	struct net_if_addr *ifaddr;
	struct net_if *iface1, *iface2;

	struct user_data ud = { 0 };

	net_if_foreach(iface_cb, &ud);

	iface1 = eth_interfaces[0];
	iface2 = eth_interfaces[1];

	zassert_not_null(iface1, "Interface 1\n");
	zassert_not_null(iface2, "Interface 2\n");

	ifaddr = net_if_ipv6_addr_add(iface1, &my_addr1,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr1));
		zassert_not_null(ifaddr, "addr1\n");
	}

	/* For testing purposes we need to set the addresses preferred */
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface1, &ll_addr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&ll_addr));
		zassert_not_null(ifaddr, "ll_addr\n");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface2, &my_addr2,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr2));
		zassert_not_null(ifaddr, "addr2\n");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_if_up(iface1);
	net_if_up(iface2);

	/* The interface might receive data which might fail the checks
	 * in the iface sending function, so we need to reset the failure
	 * flag.
	 */
	test_failed = false;
}

static bool add_neighbor(struct net_if *iface, struct in6_addr *addr)
{
	struct net_linkaddr lladdr;
	struct net_nbr *nbr;

	lladdr.addr[0] = 0x01;
	lladdr.addr[1] = 0x02;
	lladdr.addr[2] = 0x33;
	lladdr.addr[3] = 0x44;
	lladdr.addr[4] = 0x05;
	lladdr.addr[5] = 0x06;

	lladdr.len = 6U;
	lladdr.type = NET_LINK_ETHERNET;

	nbr = net_ipv6_nbr_add(iface, addr, &lladdr, false,
			       NET_IPV6_NBR_STATE_REACHABLE);
	if (!nbr) {
		DBG("Cannot add dst %s to neighbor cache\n",
		    net_sprint_ipv6_addr(addr));
		return false;
	}

	return true;
}

static void send_some_data(struct net_if *iface)
{
	struct sockaddr_in6 dst_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(TEST_PORT),
	};
	struct sockaddr_in6 src_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = 0,
	};
	int ret;

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_v6_ctx);
	zassert_equal(ret, 0, "Create IPv6 UDP context failed\n");

	memcpy(&src_addr6.sin6_addr, &my_addr1, sizeof(struct in6_addr));
	memcpy(&dst_addr6.sin6_addr, &dst_addr, sizeof(struct in6_addr));

	ret = net_context_bind(udp_v6_ctx, (struct sockaddr *)&src_addr6,
			       sizeof(struct sockaddr_in6));
	zassert_equal(ret, 0, "Context bind failure test failed\n");

	ret = add_neighbor(iface, &dst_addr);
	zassert_true(ret, "Cannot add neighbor\n");

	ret = net_context_sendto(udp_v6_ctx, test_data, strlen(test_data),
				 (struct sockaddr *)&dst_addr6,
				 sizeof(struct sockaddr_in6),
				 NULL, K_NO_WAIT, NULL);
	zassert_true(ret > 0, "Send UDP pkt failed\n");

	net_context_unref(udp_v6_ctx);
}

void test_check_timestamp_before_enabling(void)
{
	test_started = true;
	do_timestamp = false;

	send_some_data(eth_interfaces[0]);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout\n");
	}
}

void test_check_timestamp_after_enabling(void)
{
	test_started = true;
	do_timestamp = true;

	send_some_data(eth_interfaces[0]);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout\n");
	}
}

ZTEST(net_tx_timestamp, test_tx_timestamp)
{
	test_address_setup();
	test_check_timestamp_before_enabling();
	test_timestamp_setup();
	test_timestamp_setup_2nd_iface();
	test_timestamp_setup_all();
	test_check_timestamp_after_enabling();
	test_timestamp_cleanup();
}
ZTEST_SUITE(net_tx_timestamp, NULL, NULL, NULL, NULL, NULL);
