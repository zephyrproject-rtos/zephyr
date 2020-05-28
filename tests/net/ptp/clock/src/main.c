/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_L2_ETHERNET_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <linker/sections.h>

#include <ztest.h>

#include <ptp_clock.h>
#include <net/ptp_time.h>

#include <net/ethernet.h>
#include <net/buf.h>
#include <net/net_ip.h>
#include <net/net_l2.h>

#include <random/rand32.h>

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
static struct in6_addr my_addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Interface 3 addresses */
static struct in6_addr my_addr3 = { { { 0x20, 0x01, 0x0d, 0xb8, 2, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Extra address is assigned to ll_addr */
static struct in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
				       0x04 } } };

#define MAX_NUM_INTERFACES 3

/* Keep track of all ethernet interfaces */
static struct net_if *eth_interfaces[MAX_NUM_INTERFACES];

static ZTEST_BMEM int ptp_clocks[MAX_NUM_INTERFACES - 1];
static int ptp_interface[MAX_NUM_INTERFACES - 1];
static int non_ptp_interface;
static bool test_failed;
static bool test_started;

static K_SEM_DEFINE(wait_data, 0, UINT_MAX);

#define WAIT_TIME K_SECONDS(1)

struct eth_context {
	struct net_if *iface;
	uint8_t mac_addr[6];

	struct net_ptp_time time;
	struct device *ptp_clock;
};

static struct eth_context eth_context_1;
static struct eth_context eth_context_2;
static struct eth_context eth_context_3;

static void eth_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct eth_context *context = dev->data;

	net_if_set_link_addr(iface, context->mac_addr,
			     sizeof(context->mac_addr),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_tx(struct device *dev, struct net_pkt *pkt)
{
	struct eth_context *context = dev->data;

	if (&eth_context_1 != context && &eth_context_2 != context) {
		zassert_true(false, "Context pointers do not match\n");
	}

	if (!pkt->frags) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	if (test_started) {
		k_sem_give(&wait_data);
	}


	return 0;
}

static enum ethernet_hw_caps eth_capabilities(struct device *dev)
{
	return ETHERNET_PTP;
}

static struct device *eth_get_ptp_clock(struct device *dev)
{
	struct eth_context *context = dev->data;

	return context->ptp_clock;
}

static struct ethernet_api api_funcs = {
	.iface_api.init = eth_iface_init,

	.get_capabilities = eth_capabilities,
	.get_ptp_clock = eth_get_ptp_clock,
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
	mac_addr[5] = sys_rand32_get();
}

static int eth_init(struct device *dev)
{
	struct eth_context *context = dev->data;

	generate_mac(context->mac_addr);

	return 0;
}

ETH_NET_DEVICE_INIT(eth_test_1, "eth_test_1", eth_init, device_pm_control_nop,
		    &eth_context_1, NULL, CONFIG_ETH_INIT_PRIORITY, &api_funcs,
		    NET_ETH_MTU);

ETH_NET_DEVICE_INIT(eth_test_2, "eth_test_2", eth_init, device_pm_control_nop,
		    &eth_context_2, NULL, CONFIG_ETH_INIT_PRIORITY, &api_funcs,
		    NET_ETH_MTU);

ETH_NET_DEVICE_INIT(eth_test_3, "eth_test_3", eth_init, device_pm_control_nop,
		    &eth_context_3, NULL, CONFIG_ETH_INIT_PRIORITY, &api_funcs,
		    NET_ETH_MTU);

static uint64_t timestamp_to_nsec(struct net_ptp_time *ts)
{
	if (!ts) {
		return 0;
	}

	return (ts->second * NSEC_PER_SEC) + ts->nanosecond;
}

struct ptp_context {
	struct eth_context *eth_context;
};

static int my_ptp_clock_set(struct device *dev, struct net_ptp_time *tm)
{
	struct ptp_context *ptp_ctx = dev->data;
	struct eth_context *eth_ctx = ptp_ctx->eth_context;

	if (&eth_context_1 != eth_ctx && &eth_context_2 != eth_ctx) {
		zassert_true(false, "Context pointers do not match\n");
	}

	memcpy(&eth_ctx->time, tm, sizeof(struct net_ptp_time));

	return 0;
}

static int my_ptp_clock_get(struct device *dev, struct net_ptp_time *tm)
{
	struct ptp_context *ptp_ctx = dev->data;
	struct eth_context *eth_ctx = ptp_ctx->eth_context;

	memcpy(tm, &eth_ctx->time, sizeof(struct net_ptp_time));

	return 0;
}

static int my_ptp_clock_adjust(struct device *dev, int increment)
{
	struct ptp_context *ptp_ctx = dev->data;
	struct eth_context *eth_ctx = ptp_ctx->eth_context;

	eth_ctx->time.nanosecond += increment;

	return 0;
}

static int my_ptp_clock_rate_adjust(struct device *dev, float ratio)
{
	return 0;
}

static struct ptp_context ptp_test_1_context;
static struct ptp_context ptp_test_2_context;

static const struct ptp_clock_driver_api api = {
	.set = my_ptp_clock_set,
	.get = my_ptp_clock_get,
	.adjust = my_ptp_clock_adjust,
	.rate_adjust = my_ptp_clock_rate_adjust,
};

static int ptp_test_1_init(struct device *port)
{
	struct device *eth_dev = DEVICE_GET(eth_test_1);
	struct eth_context *context = eth_dev->data;
	struct ptp_context *ptp_context = port->data;

	context->ptp_clock = port;
	ptp_context->eth_context = context;

	return 0;
}

DEVICE_AND_API_INIT(ptp_clock_1, PTP_CLOCK_NAME, ptp_test_1_init,
		    &ptp_test_1_context, NULL, POST_KERNEL,
		    CONFIG_APPLICATION_INIT_PRIORITY, &api);

static int ptp_test_2_init(struct device *port)
{
	struct device *eth_dev = DEVICE_GET(eth_test_2);
	struct eth_context *context = eth_dev->data;
	struct ptp_context *ptp_context = port->data;

	context->ptp_clock = port;
	ptp_context->eth_context = context;

	return 0;
}

DEVICE_AND_API_INIT(ptp_clock_2, PTP_CLOCK_NAME, ptp_test_2_init,
		    &ptp_test_2_context, NULL, POST_KERNEL,
		    CONFIG_APPLICATION_INIT_PRIORITY, &api);

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
		static int ptp_iface_idx;
		struct device *clk;

		if (ud->eth_if_count >= ARRAY_SIZE(eth_interfaces)) {
			DBG("Invalid interface %p\n", iface);
			return;
		}

		clk = net_eth_get_ptp_clock(iface);
		if (!clk) {
			non_ptp_interface = ud->eth_if_count;
		} else {
			ptp_interface[ptp_iface_idx] = ud->eth_if_count;
			ptp_clocks[ptp_iface_idx] = net_if_get_by_iface(iface);
			ptp_iface_idx++;
		}

		eth_interfaces[ud->eth_if_count++] = iface;
	}

	/* By default all interfaces are down initially */
	net_if_down(iface);

	ud->total_if_count++;
}

static void test_check_interfaces(void)
{
	struct user_data ud = { 0 };

	/* Make sure we have enough interfaces */
	net_if_foreach(iface_cb, &ud);

	zassert_equal(ud.eth_if_count, MAX_NUM_INTERFACES,
		      "Invalid numer of ethernet interfaces %d vs %d\n",
		      ud.eth_if_count, MAX_NUM_INTERFACES);

	zassert_equal(ud.total_if_count, ud.eth_if_count,
		      "Invalid numer of interfaces %d vs %d\n",
		      ud.total_if_count, ud.eth_if_count);
}

/* As we are testing the ethernet controller clock, the IP addresses are not
 * relevant for this testing. Anyway, set the IP addresses to the interfaces so
 * we have a real life scenario.
 */
static void test_address_setup(void)
{
	struct net_if_addr *ifaddr;
	struct net_if *iface1, *iface2, *iface3;

	iface1 = eth_interfaces[0];
	iface2 = eth_interfaces[1];
	iface3 = eth_interfaces[2];

	zassert_not_null(iface1, "Interface 1\n");
	zassert_not_null(iface2, "Interface 2\n");
	zassert_not_null(iface3, "Interface 3\n");

	ifaddr = net_if_ipv6_addr_add(iface1, &my_addr1,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr1));
		zassert_not_null(ifaddr, "addr1\n");
	}

	/* For testing purposes we need to set the adddresses preferred */
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

	ifaddr = net_if_ipv6_addr_add(iface3, &my_addr3,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr3));
		zassert_not_null(ifaddr, "addr3\n");
	}

	net_if_up(iface1);
	net_if_up(iface2);
	net_if_up(iface3);

	test_failed = false;
}

static void test_ptp_clock_interfaces(void)
{
	struct device *clk_by_index;
	struct device *clk;
	int idx;

	idx = ptp_interface[0];
	clk = net_eth_get_ptp_clock(eth_interfaces[idx]);
	zassert_not_null(clk, "Clock not found for interface %p\n",
			 eth_interfaces[idx]);

	idx = ptp_interface[1];
	clk = net_eth_get_ptp_clock(eth_interfaces[idx]);
	zassert_not_null(clk, "Clock not found for interface %p\n",
			 eth_interfaces[idx]);

	clk = net_eth_get_ptp_clock(eth_interfaces[non_ptp_interface]);
	zassert_is_null(clk, "Clock found for interface %p\n",
			eth_interfaces[non_ptp_interface]);

	clk_by_index = net_eth_get_ptp_clock_by_index(ptp_clocks[0]);
	zassert_not_null(clk_by_index,
			 "Clock not found for interface index %d\n",
			 ptp_clocks[0]);
}

static void test_ptp_clock_iface(int idx)
{
	int rnd_value = sys_rand32_get();
	struct net_ptp_time tm = {
		.second = 1,
		.nanosecond = 1,
	};
	struct device *clk;
	uint64_t orig, new_value;

	clk = net_eth_get_ptp_clock(eth_interfaces[idx]);

	zassert_not_null(clk, "Clock not found for interface %p\n",
			 eth_interfaces[idx]);

	ptp_clock_set(clk, &tm);

	orig = timestamp_to_nsec(&tm);

	if (rnd_value == 0 || rnd_value < 0) {
		rnd_value = 2;
	}

	ptp_clock_adjust(clk, rnd_value);

	(void)memset(&tm, 0, sizeof(tm));
	ptp_clock_get(clk, &tm);

	new_value = timestamp_to_nsec(&tm);

	/* The clock value must be the same after incrementing it */
	zassert_equal(orig + rnd_value, new_value,
		      "Time adjust failure (%llu vs %llu)\n",
		      orig + rnd_value, new_value);
}

static void test_ptp_clock_iface_1(void)
{
	test_ptp_clock_iface(ptp_interface[0]);
}

static void test_ptp_clock_iface_2(void)
{
	test_ptp_clock_iface(ptp_interface[1]);
}

static ZTEST_BMEM struct device *clk0;
static ZTEST_BMEM struct device *clk1;

static void test_ptp_clock_get_by_index(void)
{
	struct device *clk, *clk_by_index;
	int idx;

	idx = ptp_interface[0];

	clk = net_eth_get_ptp_clock(eth_interfaces[idx]);
	zassert_not_null(clk, "PTP 0 not found");

	clk0 = clk;

	clk_by_index = net_eth_get_ptp_clock_by_index(ptp_clocks[0]);
	zassert_not_null(clk_by_index, "PTP 0 not found");

	zassert_equal(clk, clk_by_index, "Interface index %d invalid", idx);

	idx = ptp_interface[1];

	clk = net_eth_get_ptp_clock(eth_interfaces[idx]);
	zassert_not_null(clk, "PTP 1 not found");

	clk1 = clk;

	clk_by_index = net_eth_get_ptp_clock_by_index(ptp_clocks[1]);
	zassert_not_null(clk_by_index, "PTP 1 not found");

	zassert_equal(clk, clk_by_index, "Interface index %d invalid", idx);
}

static void test_ptp_clock_get_by_index_user(void)
{
	struct device *clk_by_index;

	clk_by_index = net_eth_get_ptp_clock_by_index(ptp_clocks[0]);
	zassert_not_null(clk_by_index, "PTP 0 not found");
	zassert_equal(clk0, clk_by_index, "Invalid PTP clock 0");

	clk_by_index = net_eth_get_ptp_clock_by_index(ptp_clocks[1]);
	zassert_not_null(clk_by_index, "PTP 1 not found");
	zassert_equal(clk1, clk_by_index, "Invalid PTP clock 1");
}

static ZTEST_BMEM struct net_ptp_time tm;
static ZTEST_BMEM struct net_ptp_time empty;

static void test_ptp_clock_get_by_xxx(const char *who)
{
	struct device *clk_by_index;
	int ret;

	clk_by_index = net_eth_get_ptp_clock_by_index(ptp_clocks[0]);
	zassert_not_null(clk_by_index, "PTP 0 not found (%s)", who);
	zassert_equal(clk0, clk_by_index, "Invalid PTP clock 0 (%s)", who);

	(void)memset(&tm, 0, sizeof(tm));
	ptp_clock_get(clk_by_index, &tm);

	ret = memcmp(&tm, &empty, sizeof(tm));
	zassert_not_equal(ret, 0, "ptp_clock_get() failed in %s mode", who);
}

static void test_ptp_clock_get_kernel(void)
{
	struct device *clk;

	/* Make sure that this function is really run in kernel mode by
	 * calling a function that will not work in user mode.
	 */
	clk = net_eth_get_ptp_clock(eth_interfaces[0]);

	test_ptp_clock_get_by_xxx("kernel");
}

static void test_ptp_clock_get_user(void)
{
	test_ptp_clock_get_by_xxx("user");
}

void test_main(void)
{
	struct device *clk;

	clk = device_get_binding(PTP_CLOCK_NAME);
	if (clk != NULL) {
		k_object_access_grant(clk, k_current_get());
	}

	ztest_test_suite(ptp_clock_test,
			 ztest_unit_test(test_check_interfaces),
			 ztest_unit_test(test_address_setup),
			 ztest_unit_test(test_ptp_clock_interfaces),
			 ztest_unit_test(test_ptp_clock_iface_1),
			 ztest_unit_test(test_ptp_clock_iface_2),
			 ztest_unit_test(test_ptp_clock_get_by_index),
			 ztest_user_unit_test(test_ptp_clock_get_by_index_user),
			 ztest_unit_test(test_ptp_clock_get_kernel),
			 ztest_user_unit_test(test_ptp_clock_get_user)
			 );

	ztest_run_test_suite(ptp_clock_test);
}
