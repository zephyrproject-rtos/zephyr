/* main.c - Application main entry point */

/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_PMTU_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/linker/sections.h>

#include <zephyr/ztest.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>

#include <zephyr/random/random.h>

#include "pmtu.h"

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_NET_PMTU_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

/* Small sleep between tests makes sure that the PMTU destination
 * cache entries are separated from each other.
 */
#define SMALL_SLEEP K_MSEC(5)

static struct in_addr my_ipv4_addr = { { { 192, 0, 2, 1 } } };
static struct in_addr dest_ipv4_addr1 = { { { 198, 51, 100, 1 } } };
static struct in_addr dest_ipv4_addr2 = { { { 198, 51, 100, 2 } } };
static struct in_addr dest_ipv4_addr3 = { { { 198, 51, 100, 3 } } };
static struct in_addr dest_ipv4_addr4 = { { { 198, 51, 100, 4 } } };
static struct in_addr any_ipv4_addr = INADDR_ANY_INIT;

static struct in6_addr my_ipv6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct in6_addr dest_ipv6_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 0x01, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct in6_addr dest_ipv6_addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 0x01, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x2 } } };
static struct in6_addr dest_ipv6_addr3 = { { { 0x20, 0x01, 0x0d, 0xb8, 0x01, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x3 } } };
static struct in6_addr dest_ipv6_addr4 = { { { 0x20, 0x01, 0x0d, 0xb8, 0x01, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x4 } } };
static struct in6_addr any_ipv6_addr = IN6ADDR_ANY_INIT;

K_SEM_DEFINE(wait_data, 0, UINT_MAX);

#define WAIT_TIME 500
#define WAIT_TIME_LONG MSEC_PER_SEC
#define MY_PORT 1969
#define PEER_PORT 13856

struct net_test_pmtu {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_test_dev_init(const struct device *dev)
{
	return 0;
}

static uint8_t *net_test_get_mac(const struct device *dev)
{
	struct net_test_pmtu *context = dev->data;

	if (context->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		context->mac_addr[0] = 0x00;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x5E;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x53;
		context->mac_addr[5] = sys_rand8_get();
	}

	return context->mac_addr;
}

static void net_test_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_test_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	if (!pkt->buffer) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	return 0;
}

struct net_test_pmtu net_test_data;

static struct ethernet_api net_test_if_api = {
	.iface_api.init = net_test_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER ETHERNET_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(ETHERNET_L2)

NET_DEVICE_INIT(net_test_pmtu, "net_test_pmtu",
		net_test_dev_init, NULL, &net_test_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_test_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE,
		127);

ZTEST(net_pmtu_test_suite, test_pmtu_01_ipv4_get_entry)
{
#if defined(CONFIG_NET_IPV4_PMTU)
	struct net_pmtu_entry *entry;
	struct sockaddr_in dest_ipv4;

	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr1);
	dest_ipv4.sin_family = AF_INET;

	entry = net_pmtu_get_entry((struct sockaddr *)&dest_ipv4);
	zassert_is_null(entry, "PMTU IPv4 entry is not NULL");

	k_sleep(SMALL_SLEEP);
#else
	ztest_test_skip();
#endif
}

ZTEST(net_pmtu_test_suite, test_pmtu_01_ipv6_get_entry)
{
#if defined(CONFIG_NET_IPV6_PMTU)
	struct net_pmtu_entry *entry;
	struct sockaddr_in6 dest_ipv6;

	net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr1);
	dest_ipv6.sin6_family = AF_INET6;

	entry = net_pmtu_get_entry((struct sockaddr *)&dest_ipv6);
	zassert_is_null(entry, "PMTU IPv6 entry is not NULL");

	k_sleep(SMALL_SLEEP);
#else
	ztest_test_skip();
#endif
}

ZTEST(net_pmtu_test_suite, test_pmtu_02_ipv4_update_entry)
{
#if defined(CONFIG_NET_IPV4_PMTU)
	struct sockaddr_in dest_ipv4;
	int ret;

	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr1);
	dest_ipv4.sin_family = AF_INET;

	ret = net_pmtu_update_mtu((struct sockaddr *)&dest_ipv4, 1300);
	zassert_equal(ret, 0, "PMTU IPv4 MTU update failed (%d)", ret);

	k_sleep(SMALL_SLEEP);
#else
	ztest_test_skip();
#endif
}

ZTEST(net_pmtu_test_suite, test_pmtu_02_ipv6_update_entry)
{
#if defined(CONFIG_NET_IPV6_PMTU)
	struct sockaddr_in6 dest_ipv6;
	int ret;

	net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr1);
	dest_ipv6.sin6_family = AF_INET6;

	ret = net_pmtu_update_mtu((struct sockaddr *)&dest_ipv6, 1600);
	zassert_equal(ret, 0, "PMTU IPv6 MTU update failed (%d)", ret);

	k_sleep(SMALL_SLEEP);
#else
	ztest_test_skip();
#endif
}

ZTEST(net_pmtu_test_suite, test_pmtu_03_ipv4_create_more_entries)
{
#if defined(CONFIG_NET_IPV4_PMTU)
	struct sockaddr_in dest_ipv4;
	struct net_pmtu_entry *entry;
	int ret;

	dest_ipv4.sin_family = AF_INET;

	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr1);
	ret = net_pmtu_update_mtu((struct sockaddr *)&dest_ipv4, 1300);
	zassert_equal(ret, 1300, "PMTU IPv4 MTU update failed (%d)", ret);
	entry = net_pmtu_get_entry((struct sockaddr *)&dest_ipv4);
	zassert_equal(entry->mtu, 1300, "PMTU IPv4 MTU is not correct (%d)",
		      entry->mtu);

	k_sleep(SMALL_SLEEP);

	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr2);
	ret = net_pmtu_update_mtu((struct sockaddr *)&dest_ipv4, 1400);
	zassert_equal(ret, 0, "PMTU IPv4 MTU update failed (%d)", ret);
	entry = net_pmtu_get_entry((struct sockaddr *)&dest_ipv4);
	zassert_equal(entry->mtu, 1400, "PMTU IPv4 MTU is not correct (%d)",
		      entry->mtu);


	k_sleep(SMALL_SLEEP);

	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr3);
	ret = net_pmtu_update_mtu((struct sockaddr *)&dest_ipv4, 1500);
	zassert_equal(ret, 0, "PMTU IPv4 MTU update failed (%d)", ret);
	entry = net_pmtu_get_entry((struct sockaddr *)&dest_ipv4);
	zassert_equal(entry->mtu, 1500, "PMTU IPv4 MTU is not correct (%d)",
		      entry->mtu);
#else
	ztest_test_skip();
#endif
}

ZTEST(net_pmtu_test_suite, test_pmtu_03_ipv6_create_more_entries)
{
#if defined(CONFIG_NET_IPV6_PMTU)
	struct sockaddr_in6 dest_ipv6;
	struct net_pmtu_entry *entry;
	int ret;

	dest_ipv6.sin6_family = AF_INET6;

	net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr1);
	ret = net_pmtu_update_mtu((struct sockaddr *)&dest_ipv6, 1600);
	zassert_equal(ret, 1600, "PMTU IPv6 MTU update failed (%d)", ret);
	entry = net_pmtu_get_entry((struct sockaddr *)&dest_ipv6);
	zassert_equal(entry->mtu, 1600, "PMTU IPv6 MTU is not correct (%d)",
		      entry->mtu);

	k_sleep(SMALL_SLEEP);

	net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr2);
	ret = net_pmtu_update_mtu((struct sockaddr *)&dest_ipv6, 1700);
	zassert_equal(ret, 0, "PMTU IPv6 MTU update failed (%d)", ret);
	entry = net_pmtu_get_entry((struct sockaddr *)&dest_ipv6);
	zassert_equal(entry->mtu, 1700, "PMTU IPv6 MTU is not correct (%d)",
		      entry->mtu);

	k_sleep(SMALL_SLEEP);

	net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr3);
	ret = net_pmtu_update_mtu((struct sockaddr *)&dest_ipv6, 1800);
	zassert_equal(ret, 0, "PMTU IPv6 MTU update failed (%d)", ret);
	entry = net_pmtu_get_entry((struct sockaddr *)&dest_ipv6);
	zassert_equal(entry->mtu, 1800, "PMTU IPv6 MTU is not correct (%d)",
		      entry->mtu);
#else
	ztest_test_skip();
#endif
}

ZTEST(net_pmtu_test_suite, test_pmtu_04_ipv4_overflow)
{
#if defined(CONFIG_NET_IPV4_PMTU)
	struct sockaddr_in dest_ipv4;
	struct net_pmtu_entry *entry;
	int ret;

	dest_ipv4.sin_family = AF_INET;

	/* Create more entries than we have space */
	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr4);
	ret = net_pmtu_update_mtu((struct sockaddr *)&dest_ipv4, 1450);
	zassert_equal(ret, 0, "PMTU IPv4 MTU update failed (%d)", ret);

	entry = net_pmtu_get_entry((struct sockaddr *)&dest_ipv4);
	zassert_equal(entry->mtu, 1450, "PMTU IPv4 MTU is not correct (%d)",
		      entry->mtu);

	k_sleep(SMALL_SLEEP);

	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr1);
	entry = net_pmtu_get_entry((struct sockaddr *)&dest_ipv4);
	zassert_is_null(entry, "PMTU IPv4 MTU found when it should not be");
#else
	ztest_test_skip();
#endif
}

ZTEST(net_pmtu_test_suite, test_pmtu_04_ipv6_overflow)
{
#if defined(CONFIG_NET_IPV6_PMTU)
	struct sockaddr_in6 dest_ipv6;
	struct net_pmtu_entry *entry;
	int ret;

	dest_ipv6.sin6_family = AF_INET6;

	/* Create more entries than we have space */
	net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr4);
	ret = net_pmtu_update_mtu((struct sockaddr *)&dest_ipv6, 1650);
	zassert_equal(ret, 0, "PMTU IPv6 MTU update failed (%d)", ret);

	entry = net_pmtu_get_entry((struct sockaddr *)&dest_ipv6);
	zassert_equal(entry->mtu, 1650, "PMTU IPv6 MTU is not correct (%d)",
		      entry->mtu);

	k_sleep(SMALL_SLEEP);

	/* If we have IPv4 PMTU enabled, then the oldest one is an IPv4 entry.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV4_PMTU)) {
		struct sockaddr_in dest_ipv4;

		dest_ipv4.sin_family = AF_INET;

		net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr2);
		entry = net_pmtu_get_entry((struct sockaddr *)&dest_ipv4);
	} else {
		net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr1);
		entry = net_pmtu_get_entry((struct sockaddr *)&dest_ipv6);
	}

	zassert_is_null(entry, "PMTU IPv6 MTU found when it should not be");
#else
	ztest_test_skip();
#endif
}

ZTEST_SUITE(net_pmtu_test_suite, NULL, NULL, NULL, NULL, NULL);
