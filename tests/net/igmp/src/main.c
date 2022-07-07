/* main.c - Application main entry point */

/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_IPV4_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/linker/sections.h>

#include <ztest.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/igmp.h>

#include <zephyr/random/rand32.h>

#include "ipv4.h"

#define THREAD_SLEEP 50 /* ms */

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_NET_IPV4_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static struct in_addr my_addr = { { { 192, 0, 2, 1 } } };
static struct in_addr mcast_addr = { { { 224, 0, 2, 63 } } };

static struct net_if *iface;
static bool is_group_joined;
static bool is_group_left;
static bool is_join_msg_ok;
static bool is_leave_msg_ok;
static bool is_query_received;
static bool is_report_sent;
static bool ignore_already;
K_SEM_DEFINE(wait_data, 0, UINT_MAX);

#define WAIT_TIME 500
#define WAIT_TIME_LONG MSEC_PER_SEC
#define MY_PORT 1969
#define PEER_PORT 13856

struct net_test_igmp {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_test_dev_init(const struct device *dev)
{
	return 0;
}

static uint8_t *net_test_get_mac(const struct device *dev)
{
	struct net_test_igmp *context = dev->data;

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

static void net_test_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_test_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static struct net_ipv4_igmp_v2_query *get_igmp_hdr(struct net_pkt *pkt)
{
	net_pkt_cursor_init(pkt);

	net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
		     net_pkt_ipv4_opts_len(pkt));

	return (struct net_ipv4_igmp_v2_query *)net_pkt_cursor_get_pos(pkt);
}

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	struct net_ipv4_igmp_v2_query *igmp;

	if (!pkt->buffer) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	igmp = get_igmp_hdr(pkt);

	if (igmp->type == NET_IPV4_IGMP_QUERY) {
		NET_DBG("Received query....");
		is_query_received = true;
		k_sem_give(&wait_data);
	} else if (igmp->type == NET_IPV4_IGMP_REPORT_V2) {
		NET_DBG("Received v2 report....");
		is_join_msg_ok = true;
		is_report_sent = true;
		k_sem_give(&wait_data);
	} else if (igmp->type == NET_IPV4_IGMP_LEAVE) {
		NET_DBG("Received leave....");
		is_leave_msg_ok = true;
		k_sem_give(&wait_data);
	}

	return 0;
}

struct net_test_igmp net_test_data;

static struct dummy_api net_test_if_api = {
	.iface_api.init = net_test_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(net_test_igmp, "net_test_igmp",
		net_test_dev_init, NULL, &net_test_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_test_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE,
		127);

static void group_joined(struct net_mgmt_event_callback *cb,
			 uint32_t nm_event, struct net_if *iface)
{
	if (nm_event != NET_EVENT_IPV4_MCAST_JOIN) {
		/* Spurious callback. */
		return;
	}

	is_group_joined = true;

	k_sem_give(&wait_data);
}

static void group_left(struct net_mgmt_event_callback *cb,
			 uint32_t nm_event, struct net_if *iface)
{
	if (nm_event != NET_EVENT_IPV4_MCAST_LEAVE) {
		/* Spurious callback. */
		return;
	}

	is_group_left = true;

	k_sem_give(&wait_data);
}

static struct mgmt_events {
	uint32_t event;
	net_mgmt_event_handler_t handler;
	struct net_mgmt_event_callback cb;
} mgmt_events[] = {
	{ .event = NET_EVENT_IPV4_MCAST_JOIN, .handler = group_joined },
	{ .event = NET_EVENT_IPV4_MCAST_LEAVE, .handler = group_left },
	{ 0 }
};

static void setup_mgmt_events(void)
{
	int i;

	for (i = 0; mgmt_events[i].event; i++) {
		net_mgmt_init_event_callback(&mgmt_events[i].cb,
					     mgmt_events[i].handler,
					     mgmt_events[i].event);

		net_mgmt_add_event_callback(&mgmt_events[i].cb);
	}
}

static void test_igmp_setup(void)
{
	struct net_if_addr *ifaddr;

	setup_mgmt_events();

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	zassert_not_null(iface, "Interface is NULL");

	ifaddr = net_if_ipv4_addr_add(iface, &my_addr, NET_ADDR_MANUAL, 0);

	zassert_not_null(ifaddr, "Cannot add IPv4 address");
}

static void test_join_group(void)
{
	int ret;

	ret = net_ipv4_igmp_join(iface, &mcast_addr);

	if (ignore_already) {
		zassert_true(ret == 0 || ret == -EALREADY,
			     "Cannot join IPv4 multicast group");
	} else {
		zassert_equal(ret, 0, "Cannot join IPv4 multicast group");
	}

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);
}

static void test_leave_group(void)
{
	int ret;

	ret = net_ipv4_igmp_leave(iface, &mcast_addr);

	zassert_equal(ret, 0, "Cannot leave IPv4 multicast group");

	if (IS_ENABLED(CONFIG_NET_TC_THREAD_PREEMPTIVE)) {
		/* Let the network stack to proceed */
		k_msleep(THREAD_SLEEP);
	} else {
		k_yield();
	}
}

static void test_catch_join_group(void)
{
	is_group_joined = false;

	ignore_already = false;

	test_join_group();

	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting join event");
	}

	if (!is_group_joined) {
		zassert_true(0, "Did not catch join event");
	}

	is_group_joined = false;
}

static void test_catch_leave_group(void)
{
	is_group_joined = false;

	test_leave_group();

	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting leave event");
	}

	if (!is_group_left) {
		zassert_true(0, "Did not catch leave event");
	}

	is_group_left = false;
}

static void test_verify_join_group(void)
{
	is_join_msg_ok = false;

	ignore_already = false;

	test_join_group();

	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting join event");
	}

	if (!is_join_msg_ok) {
		zassert_true(0, "Join msg invalid");
	}

	is_join_msg_ok = false;
}

static void test_verify_leave_group(void)
{
	is_leave_msg_ok = false;

	test_leave_group();

	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting leave event");
	}

	if (!is_leave_msg_ok) {
		zassert_true(0, "Leave msg invalid");
	}

	is_leave_msg_ok = false;
}

void test_main(void)
{
	ztest_test_suite(net_igmp_test,
			 ztest_unit_test(test_igmp_setup),
			 ztest_unit_test(test_join_group),
			 ztest_unit_test(test_leave_group),
			 ztest_unit_test(test_catch_join_group),
			 ztest_unit_test(test_catch_leave_group),
			 ztest_unit_test(test_verify_join_group),
			 ztest_unit_test(test_verify_leave_group)
			 );

	ztest_run_test_suite(net_igmp_test);
}
