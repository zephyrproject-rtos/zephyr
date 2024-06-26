/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/linker/sections.h>

#include <zephyr/ztest.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include "conn_mgr_private.h"
#include "test_ifaces.h"
#include <zephyr/net/net_ip.h>

#include <zephyr/logging/log.h>

/* Time to wait for NET_MGMT events to finish firing */
#define EVENT_WAIT_TIME K_MSEC(1)


/* Time to wait for IPv6 DAD-gated events to finish.
 * Equivalent to EVENT_WAIT_TIME if DAD is dissabled.
 */
#if defined(CONFIG_NET_IPV6_DAD)
#define DAD_WAIT_TIME K_MSEC(110)
#else
#define DAD_WAIT_TIME EVENT_WAIT_TIME
#endif

/* IP addresses -- Two of each are needed because address sharing will cause address removal to
 * fail silently (Address is only removed from one iface).
 */
static struct in_addr test_ipv4_a = { { { 10, 0, 0, 1 } } };
static struct in_addr test_ipv4_b = { { { 10, 0, 0, 2 } } };
static struct in6_addr test_ipv6_a = { { {
	0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1
} } };
static struct in6_addr test_ipv6_b = { { {
	0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2
} } };


/* Helpers */
static void reset_test_iface(struct net_if *iface)
{
	struct in6_addr *ll_ipv6;

	if (net_if_is_admin_up(iface)) {
		(void)net_if_down(iface);
	}

	net_if_ipv4_addr_rm(iface, &test_ipv4_a);
	net_if_ipv4_addr_rm(iface, &test_ipv4_b);
	net_if_ipv6_addr_rm(iface, &test_ipv6_a);
	net_if_ipv6_addr_rm(iface, &test_ipv6_b);

	/* DAD adds the link-local address automatically. Check for it, and remove it if present. */
	ll_ipv6 = net_if_ipv6_get_ll(iface, NET_ADDR_ANY_STATE);
	if (ll_ipv6) {
		net_if_ipv6_addr_rm(iface, ll_ipv6);
	}

	conn_mgr_watch_iface(iface);
}

/* Thread-safe test statistics */
K_MUTEX_DEFINE(stats_mutex);
static struct test_stats {
	/** IPv4 connectivity event counters */
	int event_count_ipv4; /* any */
	int conn_count_ipv4;  /* connect */
	int dconn_count_ipv4; /* disconnect */

	/** IPv6 connectivity event counters */
	int event_count_ipv6; /* any */
	int conn_count_ipv6;  /* connect */
	int dconn_count_ipv6; /* disconnect */

	/** General connectivity event counters */
	int event_count_gen; /* any */
	int conn_count_gen;  /* connect */
	int dconn_count_gen; /* disconnect */

	/** The iface blamed for the last disconnect event */
	struct net_if *dconn_iface_gen;
	struct net_if *dconn_iface_ipv4;
	struct net_if *dconn_iface_ipv6;

	/** The iface blamed for the last connect event */
	struct net_if *conn_iface_gen;
	struct net_if *conn_iface_ipv4;
	struct net_if *conn_iface_ipv6;

} global_stats;

static void reset_stats(void)
{
	k_mutex_lock(&stats_mutex, K_FOREVER);

	global_stats.conn_count_gen = 0;
	global_stats.dconn_count_gen = 0;
	global_stats.event_count_gen = 0;
	global_stats.dconn_iface_gen = NULL;
	global_stats.conn_iface_gen = NULL;

	global_stats.conn_count_ipv4 = 0;
	global_stats.dconn_count_ipv4 = 0;
	global_stats.event_count_ipv4 = 0;
	global_stats.dconn_iface_ipv4 = NULL;
	global_stats.conn_iface_ipv4 = NULL;

	global_stats.conn_count_ipv6 = 0;
	global_stats.dconn_count_ipv6 = 0;
	global_stats.event_count_ipv6 = 0;
	global_stats.dconn_iface_ipv6 = NULL;
	global_stats.conn_iface_ipv6 = NULL;

	k_mutex_unlock(&stats_mutex);
}

/**
 * @brief Copy and then reset global test stats.
 *
 * @return struct test_stats -- The copy of the global test stats struct before it was reset
 */
static struct test_stats get_reset_stats(void)
{
	struct test_stats copy;

	k_mutex_lock(&stats_mutex, K_FOREVER);
	copy = global_stats;
	reset_stats();
	k_mutex_unlock(&stats_mutex);
	return copy;
}

/* Callback hooks */
struct net_mgmt_event_callback l4_callback;

void l4_handler(struct net_mgmt_event_callback *cb, uint32_t event, struct net_if *iface)
{
	if (event == NET_EVENT_L4_CONNECTED) {
		k_mutex_lock(&stats_mutex, K_FOREVER);
		global_stats.conn_count_gen += 1;
		global_stats.event_count_gen += 1;
		global_stats.conn_iface_gen = iface;
		k_mutex_unlock(&stats_mutex);
	} else if (event == NET_EVENT_L4_DISCONNECTED) {
		k_mutex_lock(&stats_mutex, K_FOREVER);
		global_stats.dconn_count_gen += 1;
		global_stats.event_count_gen += 1;
		global_stats.dconn_iface_gen = iface;
		k_mutex_unlock(&stats_mutex);
	}
}

struct net_mgmt_event_callback conn_callback;

void conn_handler(struct net_mgmt_event_callback *cb, uint32_t event, struct net_if *iface)
{
	if (event == NET_EVENT_L4_IPV6_CONNECTED) {
		k_mutex_lock(&stats_mutex, K_FOREVER);
		global_stats.conn_count_ipv6 += 1;
		global_stats.event_count_ipv6 += 1;
		global_stats.conn_iface_ipv6 = iface;
		k_mutex_unlock(&stats_mutex);
	} else if (event == NET_EVENT_L4_IPV6_DISCONNECTED) {
		k_mutex_lock(&stats_mutex, K_FOREVER);
		global_stats.dconn_count_ipv6 += 1;
		global_stats.event_count_ipv6 += 1;
		global_stats.dconn_iface_ipv6 = iface;
		k_mutex_unlock(&stats_mutex);
	} else if (event == NET_EVENT_L4_IPV4_CONNECTED) {
		k_mutex_lock(&stats_mutex, K_FOREVER);
		global_stats.conn_count_ipv4 += 1;
		global_stats.event_count_ipv4 += 1;
		global_stats.conn_iface_ipv4 = iface;
		k_mutex_unlock(&stats_mutex);
	} else if (event == NET_EVENT_L4_IPV4_DISCONNECTED) {
		k_mutex_lock(&stats_mutex, K_FOREVER);
		global_stats.dconn_count_ipv4 += 1;
		global_stats.event_count_ipv4 += 1;
		global_stats.dconn_iface_ipv4 = iface;
		k_mutex_unlock(&stats_mutex);
	}
}

/* Test suite shared functions & routines */

static void *conn_mgr_setup(void)
{
	net_mgmt_init_event_callback(
		&l4_callback, l4_handler, NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED
	);
	net_mgmt_add_event_callback(&l4_callback);

	net_mgmt_init_event_callback(
		&conn_callback, conn_handler,
		NET_EVENT_L4_IPV6_CONNECTED | NET_EVENT_L4_IPV6_DISCONNECTED |
		NET_EVENT_L4_IPV4_CONNECTED | NET_EVENT_L4_IPV4_DISCONNECTED
	);
	net_mgmt_add_event_callback(&conn_callback);
	return NULL;
}


static void conn_mgr_before(void *data)
{
	ARG_UNUSED(data);

	reset_test_iface(if_simp_a);
	reset_test_iface(if_simp_b);
	reset_test_iface(if_conn_a);
	reset_test_iface(if_conn_b);

	/* Allow any triggered events to shake out */
	k_sleep(EVENT_WAIT_TIME);

	reset_stats();
}

/**
 * @brief Cycles two ifaces through several transitions from readiness to unreadiness.
 *
 * Ifaces are assigned a single IPv4 address at the start, and cycled through oper-states, since
 * the other manners in which an iface can become L4-ready are covered by cycle_iface_pre_ready
 *
 * It is not necessary to cover all possible state transitions, only half of them, since
 * this will be called twice by the test suites for each combination of iface type (except
 * combinations where both ifaces are of the same type).
 */

static void cycle_ready_ifaces(struct net_if *ifa, struct net_if *ifb)
{
	struct test_stats stats;

	/* Add IPv4 addresses */
	net_if_ipv4_addr_add(ifa, &test_ipv4_a, NET_ADDR_MANUAL, 0);
	net_if_ipv4_addr_add(ifb, &test_ipv4_b, NET_ADDR_MANUAL, 0);

	/* Expect no events */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connectivity availability did not change.");

	/* Take A up */
	zassert_equal(net_if_up(ifa), 0, "net_if_up should succeed for ifa.");

	/* Expect connectivity gained */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.conn_count_gen, 1,
		"NET_EVENT_L4_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.event_count_gen, 1,
		"Only NET_EVENT_L4_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.conn_iface_gen, ifa, "ifa should be blamed.");

	/* Take B up */
	zassert_equal(net_if_up(ifb), 0, "net_if_up should succeed for ifb.");

	/* Expect no events */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connectivity availability did not change.");

	/* Take A down */
	zassert_equal(net_if_down(ifa), 0, "net_if_down should succeed for ifa.");

	/* Expect no events */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connectivity availability did not change.");

	/* Take B down */
	zassert_equal(net_if_down(ifb), 0, "net_if_down should succeed for ifb.");

	/* Expect connectivity loss */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.dconn_count_gen, 1,
		"NET_EVENT_L4_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.event_count_gen, 1,
		"Only NET_EVENT_L4_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.dconn_iface_gen, ifb, "ifb should be blamed.");
}

/**
 * @brief Ignores and then toggles ifb's readiness several times, ensuring no events are fired.
 *
 * At several points, change the readiness state of ifa and ensure events are fired.
 *
 * Steps which bring ifa or ifb online wait for the DAD delay to allow IPv6 events to finish.
 * For test builds that have DAD disabled, this is equivalent to the usual event wait time.
 *
 * @param ifa
 * @param ifb
 */
static void cycle_ignored_iface(struct net_if *ifa, struct net_if *ifb)
{
	struct test_stats stats;
	printk("cycle_ignored_iface\n");

	/* Ignore B */
	conn_mgr_ignore_iface(ifb);

	/* Add IPv4 and IPv6 addresses so that all possible event types are fired. */
	net_if_ipv4_addr_add(ifa, &test_ipv4_a, NET_ADDR_MANUAL, 0);
	net_if_ipv4_addr_add(ifb, &test_ipv4_b, NET_ADDR_MANUAL, 0);
	net_if_ipv6_addr_add(ifa, &test_ipv6_a, NET_ADDR_MANUAL, 0);
	net_if_ipv6_addr_add(ifb, &test_ipv6_b, NET_ADDR_MANUAL, 0);

	/* Set one: Change A state between B state toggles */

	/* Take B up */
	zassert_equal(net_if_up(ifb), 0, "net_if_up should succeed for ifb.");

	/* Expect no events.
	 * Wait for the DAD delay since IPv6 connected events might be delayed by this amount.
	 */
	printk("Expect no events.\n");
	k_sleep(DAD_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv4, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv6, 0,
		"No events should be fired if connectivity availability did not change.");

	/* Take B down */
	zassert_equal(net_if_down(ifb), 0, "net_if_down should succeed for ifb.");

	/* Expect no events */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv4, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv6, 0,
		"No events should be fired if connectivity availability did not change.");

	/* Take A up */
	zassert_equal(net_if_up(ifa), 0, "net_if_up should succeed for ifa.");

	/* Expect connectivity gained */
	k_sleep(DAD_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.conn_count_gen, 1,
		"NET_EVENT_L4_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.event_count_gen, 1,
		"Only NET_EVENT_L4_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.conn_count_ipv6, 1,
		"NET_EVENT_L4_IPV6_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.event_count_ipv6, 1,
		"Only NET_EVENT_L4_IPV6_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.conn_count_ipv4, 1,
		"NET_EVENT_L4_IPV4_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.event_count_ipv4, 1,
		"Only NET_EVENT_L4_IPV4_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.conn_iface_gen, ifa, "ifa should be blamed.");
	zassert_equal(stats.conn_iface_ipv4, ifa, "ifa should be blamed.");
	zassert_equal(stats.conn_iface_ipv6, ifa, "ifa should be blamed.");

	/* Take B up */
	zassert_equal(net_if_up(ifb), 0, "net_if_up should succeed for ifb.");

	/* Expect no events */
	k_sleep(DAD_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv4, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv6, 0,
		"No events should be fired if connectivity availability did not change.");

	/* Take B down */
	zassert_equal(net_if_down(ifb), 0, "net_if_down should succeed for ifb.");

	/* Expect no events */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv4, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv6, 0,
		"No events should be fired if connectivity availability did not change.");

	/* Take A down */
	zassert_equal(net_if_down(ifa), 0, "net_if_down should succeed for ifa.");

	/* Expect connectivity lost */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.dconn_count_gen, 1,
		"NET_EVENT_L4_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.event_count_gen, 1,
		"Only NET_EVENT_L4_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.dconn_count_ipv6, 1,
		"NET_EVENT_L4_IPV6_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.event_count_ipv6, 1,
		"Only NET_EVENT_L4_IPV6_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.dconn_count_ipv4, 1,
		"NET_EVENT_L4_IPV4_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.event_count_ipv4, 1,
		"Only NET_EVENT_L4_IPV4_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.dconn_iface_gen, ifa, "ifa should be blamed.");
	zassert_equal(stats.dconn_iface_ipv4, ifa, "ifa should be blamed.");
	zassert_equal(stats.dconn_iface_ipv6, ifa, "ifa should be blamed.");


	/* Set two: Change A state during B state toggles */

	/* Take B up */
	zassert_equal(net_if_up(ifb), 0, "net_if_up should succeed for ifb.");

	/* Expect no events */
	k_sleep(DAD_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv4, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv6, 0,
		"No events should be fired if connectivity availability did not change.");

	/* Take A up */
	zassert_equal(net_if_up(ifa), 0, "net_if_up should succeed for ifa.");

	/* Expect connectivity gained */
	k_sleep(DAD_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.conn_count_gen, 1,
		"NET_EVENT_L4_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.event_count_gen, 1,
		"Only NET_EVENT_L4_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.conn_count_ipv6, 1,
		"NET_EVENT_L4_IPV6_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.event_count_ipv6, 1,
		"Only NET_EVENT_L4_IPV6_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.conn_count_ipv4, 1,
		"NET_EVENT_L4_IPV4_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.event_count_ipv4, 1,
		"Only NET_EVENT_L4_IPV4_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.conn_iface_gen, ifa, "ifa should be blamed.");
	zassert_equal(stats.conn_iface_ipv4, ifa, "ifa should be blamed.");
	zassert_equal(stats.conn_iface_ipv6, ifa, "ifa should be blamed.");

	/* Take B down */
	zassert_equal(net_if_down(ifb), 0, "net_if_down should succeed for ifb.");

	/* Expect no events */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv4, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv6, 0,
		"No events should be fired if connectivity availability did not change.");


	/* Take B up */
	zassert_equal(net_if_up(ifb), 0, "net_if_up should succeed for ifb.");

	/* Expect no events */
	k_sleep(DAD_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv4, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv6, 0,
		"No events should be fired if connectivity availability did not change.");

	/* Take A down */
	zassert_equal(net_if_down(ifa), 0, "net_if_down should succeed for ifa.");

	/* Expect connectivity lost */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.dconn_count_gen, 1,
		"NET_EVENT_L4_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.event_count_gen, 1,
		"Only NET_EVENT_L4_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.dconn_count_ipv6, 1,
		"NET_EVENT_L4_IPV6_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.event_count_ipv6, 1,
		"Only NET_EVENT_L4_IPV6_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.dconn_count_ipv4, 1,
		"NET_EVENT_L4_IPV4_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.event_count_ipv4, 1,
		"Only NET_EVENT_L4_IPV4_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.dconn_iface_gen, ifa, "ifa should be blamed.");
	zassert_equal(stats.dconn_iface_ipv4, ifa, "ifa should be blamed.");
	zassert_equal(stats.dconn_iface_ipv6, ifa, "ifa should be blamed.");

	/* Take B down */
	zassert_equal(net_if_down(ifb), 0, "net_if_down should succeed for ifb.");

	/* Expect no events */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv4, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv6, 0,
		"No events should be fired if connectivity availability did not change.");
}

enum ip_order {
	IPV4_FIRST,
	IPV6_FIRST
};

/**
 * @brief Cycles a single iface through all possible ready and pre-ready states,
 * ensuring the correct events are observed and generated by conn_mgr_monitor.
 *
 * Ifaces can be in one of four states that are relevant to L4 readiness:
 * 00: oper-down, no IPs associated (unready state)
 * 01: Has IP, is oper-down (semi-ready state)
 * 10: Is oper-up, has no IP (semi-ready state)
 * 11: Has IP and is oper-up (ready state)
 *
 * In total there are eight possible state transitions:
 *
 * (00 -> 10): Gain oper-up from unready state
 * (10 -> 11): Gain IP from semi-ready state
 * (11 -> 10): Lose IP from ready state
 * (10 -> 00): Lose oper-up from semi-ready state
 * (00 -> 01): Gain IP from unready state
 * (01 -> 11): Gain Oper-up from semi-ready state
 * (11 -> 01): Lose oper-up from ready state
 * (01 -> 00): Lose IP from semi-ready state
 *
 * We test these state transitions in that order.
 *
 * This is slightly complicated by the fact that ifaces can be assigned multiple IPs, and multiple
 * types of IPs. Whenever IPs are assigned or removed, two of them, an IPv6 and IPv4 address is
 * added or removed.
 *
 * @param iface
 * @param ifa_ipm
 */
static void cycle_iface_states(struct net_if *iface, enum ip_order ifa_ipm)
{
	struct test_stats stats;

	/* (00 -> 10): Gain oper-up from unready state */

	/* Take iface up */
	zassert_equal(net_if_up(iface), 0, "net_if_up should succeed.");

	/* Verify that no events have been fired yet */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connectivity availability did not change.");

	/* (10 -> 11): Gain IP from semi-ready state */
	switch (ifa_ipm) {
	case IPV4_FIRST:
		/* Add IPv4 */
		net_if_ipv4_addr_add(iface, &test_ipv4_a, NET_ADDR_MANUAL, 0);

		/* Verify correct events */
		k_sleep(EVENT_WAIT_TIME);
		stats = get_reset_stats();
		zassert_equal(stats.conn_count_gen, 1,
			"NET_EVENT_L4_CONNECTED should be fired when connectivity is gained.");
		zassert_equal(stats.event_count_gen, 1,
			"Only NET_EVENT_L4_CONNECTED should be fired when connectivity is gained.");
		zassert_equal(stats.conn_count_ipv4, 1,
			"NET_EVENT_L4_IPV4_CONNECTED should be fired when IPv4 "
			"connectivity is gained.");
		zassert_equal(stats.event_count_ipv4, 1,
			"Only NET_EVENT_L4_IPV4_CONNECTED should be fired when IPv4 "
			"connectivity is gained.");
		zassert_equal(stats.event_count_ipv6, 0,
			"No IPv6 events should be fired when IPv4 connectivity is gained.");
		zassert_equal(stats.conn_iface_gen, iface, "The test iface should be blamed.");
		zassert_equal(stats.conn_iface_ipv4, iface, "The test iface should be blamed.");


		/* Add IPv6 */
		net_if_ipv6_addr_add(iface, &test_ipv6_a, NET_ADDR_MANUAL, 0);
		k_sleep(DAD_WAIT_TIME);

		/* Verify only IPv6 events */
		k_sleep(EVENT_WAIT_TIME);
		stats = get_reset_stats();
		zassert_equal(stats.event_count_gen, 0,
			"No events should be fired if connectivity availability did not change.");
		zassert_equal(stats.conn_count_ipv6, 1,
			"NET_EVENT_L4_IPV6_CONNECTED should be fired when IPv6 "
			"connectivity is gained.");
		zassert_equal(stats.event_count_ipv6, 1,
			"Only NET_EVENT_L4_IPV6_CONNECTED should be fired when IPv6 "
			"connectivity is gained.");
		zassert_equal(stats.event_count_ipv4, 0,
			"No IPv4 events should be fired when IPv6 connectivity is gained.");
		zassert_equal(stats.conn_iface_ipv6, iface, "The test iface should be blamed.");

		break;
	case IPV6_FIRST:
		/* Add IPv6 */
		net_if_ipv6_addr_add(iface, &test_ipv6_a, NET_ADDR_MANUAL, 0);
		k_sleep(DAD_WAIT_TIME);

		/* Verify correct events */
		k_sleep(EVENT_WAIT_TIME);
		stats = get_reset_stats();
		zassert_equal(stats.conn_count_gen, 1,
			"NET_EVENT_L4_CONNECTED should be fired when connectivity is gained.");
		zassert_equal(stats.event_count_gen, 1,
			"Only NET_EVENT_L4_CONNECTED should be fired when connectivity is gained.");
		zassert_equal(stats.conn_count_ipv6, 1,
			"NET_EVENT_L4_IPV6_CONNECTED should be fired when IPv6 "
			"connectivity is gained.");
		zassert_equal(stats.event_count_ipv6, 1,
			"Only NET_EVENT_L4_IPV6_CONNECTED should be fired when IPv6 "
			"connectivity is gained.");
		zassert_equal(stats.event_count_ipv4, 0,
			"No IPv4 events should be fired when IPv6 connectivity is gained.");
		zassert_equal(stats.conn_iface_gen, iface, "The test iface should be blamed.");
		zassert_equal(stats.conn_iface_ipv6, iface, "The test iface should be blamed.");

		/* Add IPv4 */
		net_if_ipv4_addr_add(iface, &test_ipv4_a, NET_ADDR_MANUAL, 0);

		/* Verify only IPv4 events */
		k_sleep(EVENT_WAIT_TIME);
		stats = get_reset_stats();
		zassert_equal(stats.event_count_gen, 0,
			"No events should be fired if connectivity availability did not change.");
		zassert_equal(stats.conn_count_ipv4, 1,
			"NET_EVENT_L4_IPV4_CONNECTED should be fired when IPv4 "
			"connectivity is gained.");
		zassert_equal(stats.event_count_ipv4, 1,
			"Only NET_EVENT_L4_IPV4_CONNECTED should be fired when IPv4 "
			"connectivity is gained.");
		zassert_equal(stats.event_count_ipv6, 0,
			"No IPv6 events should be fired when IPv4 connectivity is gained.");
		zassert_equal(stats.conn_iface_ipv4, iface, "The test iface should be blamed.");
		break;
	}

	/* (11 -> 10): Lose IP from ready state */
	switch (ifa_ipm) {
	case IPV4_FIRST:
		/* Remove IPv4 */
		zassert_true(net_if_ipv4_addr_rm(iface, &test_ipv4_a),
							"IPv4 removal should succeed.");

		/* Verify only IPv4 events */
		k_sleep(EVENT_WAIT_TIME);
		stats = get_reset_stats();
		zassert_equal(stats.event_count_gen, 0,
			"No events should be fired if connectivity availability did not change.");
		zassert_equal(stats.dconn_count_ipv4, 1,
			"NET_EVENT_L4_IPV4_DISCONNECTED should be fired when IPv4 "
			"connectivity is lost.");
		zassert_equal(stats.event_count_ipv4, 1,
			"Only NET_EVENT_L4_IPV4_DISCONNECTED should be fired when IPv4 "
			"connectivity is lost.");
		zassert_equal(stats.event_count_ipv6, 0,
			"No IPv6 events should be fired when IPv4 connectivity is gained.");
		zassert_equal(stats.dconn_iface_ipv4, iface, "The test iface should be blamed.");

		/* Remove IPv6 */
		zassert_true(net_if_ipv6_addr_rm(iface, &test_ipv6_a),
							"IPv6 removal should succeed.");

		/* Verify correct events */
		k_sleep(EVENT_WAIT_TIME);
		stats = get_reset_stats();
		zassert_equal(stats.dconn_count_gen, 1,
			"NET_EVENT_L4_DISCONNECTED should be fired when connectivity is lost.");
		zassert_equal(stats.event_count_gen, 1,
			"Only NET_EVENT_L4_DISCONNECTED should be fired when connectivity "
			"is lost.");
		zassert_equal(stats.dconn_count_ipv6, 1,
			"NET_EVENT_L4_IPV6_DISCONNECTED should be fired when IPv6 "
			"connectivity is lost.");
		zassert_equal(stats.event_count_ipv6, 1,
			"Only NET_EVENT_L4_IPV6_DISCONNECTED should be fired when IPv6 "
			"connectivity is lost.");
		zassert_equal(stats.event_count_ipv4, 0,
			"No IPv4 events should be fired when IPv6 connectivity is gained.");

		zassert_equal(stats.dconn_iface_gen, iface, "The test iface should be blamed.");
		zassert_equal(stats.dconn_iface_ipv6, iface, "The test iface should be blamed.");

		break;
	case IPV6_FIRST:
		/* Remove IPv6 */
		zassert_true(net_if_ipv6_addr_rm(iface, &test_ipv6_a),
							"IPv6 removal should succeed.");

		/* Verify only IPv6 events */
		k_sleep(EVENT_WAIT_TIME);
		stats = get_reset_stats();
		zassert_equal(stats.event_count_gen, 0,
			"No events should be fired if connectivity availability did not change.");
		zassert_equal(stats.dconn_count_ipv6, 1,
			"NET_EVENT_L4_IPV6_DISCONNECTED should be fired when IPv6 "
			"connectivity is lost.");
		zassert_equal(stats.event_count_ipv6, 1,
			"Only NET_EVENT_L4_IPV6_DISCONNECTED should be fired when IPv6 "
			"connectivity is lost.");
		zassert_equal(stats.event_count_ipv4, 0,
			"No IPv4 events should be fired when IPv6 connectivity is gained.");
		zassert_equal(stats.dconn_iface_ipv6, iface, "The test iface should be blamed.");

		/* Remove IPv4 */
		zassert_true(net_if_ipv4_addr_rm(iface, &test_ipv4_a),
							"IPv4 removal should succeed.");

		/* Verify correct events */
		k_sleep(EVENT_WAIT_TIME);
		stats = get_reset_stats();
		zassert_equal(stats.dconn_count_gen, 1,
			"NET_EVENT_L4_DISCONNECTED should be fired when connectivity is lost.");
		zassert_equal(stats.event_count_gen, 1,
			"Only NET_EVENT_L4_DISCONNECTED should be fired when connectivity "
			"is lost.");
		zassert_equal(stats.dconn_count_ipv4, 1,
			"NET_EVENT_L4_IPV4_DISCONNECTED should be fired when IPv4 "
			"connectivity is lost.");
		zassert_equal(stats.event_count_ipv4, 1,
			"Only NET_EVENT_L4_IPV4_DISCONNECTED should be fired when IPv4 "
			"connectivity is lost.");
		zassert_equal(stats.event_count_ipv6, 0,
			"No IPv6 events should be fired when IPv4 connectivity is gained.");
		zassert_equal(stats.dconn_iface_gen, iface, "The test iface should be blamed.");
		zassert_equal(stats.dconn_iface_ipv4, iface, "The test iface should be blamed.");

		break;
	}

	/* (10 -> 00): Lose oper-up from semi-ready state */

	/* Take iface down */
	zassert_equal(net_if_down(iface), 0, "net_if_down should succeed.");

	/* Verify there are no events fired */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv4, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv6, 0,
		"No events should be fired if connectivity availability did not change.");

	/* (00 -> 01): Gain IP from unready state */

	/* Add IP addresses to iface */
	switch (ifa_ipm) {
	case IPV4_FIRST:
		/* Add IPv4 and IPv6 */
		net_if_ipv4_addr_add(iface, &test_ipv4_a, NET_ADDR_MANUAL, 0);
		net_if_ipv6_addr_add(iface, &test_ipv6_a, NET_ADDR_MANUAL, 0);
		k_sleep(DAD_WAIT_TIME);
		break;
	case IPV6_FIRST:
		/* Add IPv6 then IPv4 */
		net_if_ipv6_addr_add(iface, &test_ipv6_a, NET_ADDR_MANUAL, 0);
		k_sleep(DAD_WAIT_TIME);
		net_if_ipv4_addr_add(iface, &test_ipv4_a, NET_ADDR_MANUAL, 0);
		break;
	}

	/* Verify that no events are fired */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv4, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv6, 0,
		"No events should be fired if connectivity availability did not change.");

	/* (01 -> 11): Gain Oper-up from semi-ready state */

	/* Take iface up */
	zassert_equal(net_if_up(iface), 0, "net_if_up should succeed.");

	/* Verify events are fired */
	k_sleep(DAD_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.conn_count_gen, 1,
		"NET_EVENT_L4_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.event_count_gen, 1,
		"Only NET_EVENT_L4_CONNECTED should be fired when connectivity is gained.");
	zassert_equal(stats.conn_count_ipv4, 1,
		"NET_EVENT_L4_IPV4_CONNECTED should be fired when IPv4 "
		"connectivity is gained.");
	zassert_equal(stats.event_count_ipv4, 1,
		"Only NET_EVENT_L4_IPV4_CONNECTED should be fired when IPv4 "
		"connectivity is gained.");
	zassert_equal(stats.conn_count_ipv6, 1,
		"NET_EVENT_L4_IPV6_CONNECTED should be fired when IPv6 "
		"connectivity is gained.");
	zassert_equal(stats.event_count_ipv6, 1,
		"Only NET_EVENT_L4_IPV6_CONNECTED should be fired when IPv6 "
		"connectivity is gained.");
	zassert_equal(stats.conn_iface_gen, iface, "The test iface should be blamed.");
	zassert_equal(stats.conn_iface_ipv4, iface, "The test iface should be blamed.");
	zassert_equal(stats.conn_iface_ipv6, iface, "The test iface should be blamed.");

	/* (11 -> 01): Lose oper-up from ready state */

	/* Take iface down */
	zassert_equal(net_if_down(iface), 0, "net_if_down should succeed.");

	/* Verify events are fired */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.dconn_count_gen, 1,
		"NET_EVENT_L4_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.event_count_gen, 1,
		"Only NET_EVENT_L4_DISCONNECTED should be fired when connectivity is lost.");
	zassert_equal(stats.dconn_count_ipv4, 1,
		"NET_EVENT_L4_IPV4_DISCONNECTED should be fired when IPv4 "
		"connectivity is lost.");
	zassert_equal(stats.event_count_ipv4, 1,
		"Only NET_EVENT_L4_IPV4_DISCONNECTED should be fired when IPv4 "
		"connectivity is lost.");
	zassert_equal(stats.dconn_count_ipv6, 1,
		"NET_EVENT_L4_IPV6_DISCONNECTED should be fired when IPv6 "
		"connectivity is lost.");
	zassert_equal(stats.event_count_ipv6, 1,
		"Only NET_EVENT_L4_IPV6_DISCONNECTED should be fired when IPv6 "
		"connectivity is lost.");
	zassert_equal(stats.dconn_iface_gen, iface, "The test iface should be blamed.");
	zassert_equal(stats.dconn_iface_ipv4, iface, "The test iface should be blamed.");
	zassert_equal(stats.dconn_iface_ipv6, iface, "The test iface should be blamed.");

	/* (01 -> 00): Lose IP from semi-ready state */

	/* Remove IPs */
	switch (ifa_ipm) {
	case IPV4_FIRST:
		/* Remove IPv4 then IPv6 */
		zassert_true(net_if_ipv4_addr_rm(iface, &test_ipv4_a),
							"IPv4 removal should succeed.");
		zassert_true(net_if_ipv6_addr_rm(iface, &test_ipv6_a),
							"IPv6 removal should succeed.");
		break;
	case IPV6_FIRST:
		/* Remove IPv6 then IPv4 */
		zassert_true(net_if_ipv6_addr_rm(iface, &test_ipv6_a),
							"IPv6 removal should succeed.");
		zassert_true(net_if_ipv4_addr_rm(iface, &test_ipv4_a),
							"IPv4 removal should succeed.");
		break;
	}

	/* Verify no events fired */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv4, 0,
		"No events should be fired if connectivity availability did not change.");
	zassert_equal(stats.event_count_ipv6, 0,
		"No events should be fired if connectivity availability did not change.");
}

/* Cases */

/* Make sure all readiness transitions of a pair of connectivity-enabled ifaces results in all
 * expected events.
 */
ZTEST(conn_mgr_monitor, test_cycle_ready_CC)
{
	cycle_ready_ifaces(if_conn_a, if_conn_b);
}

/* Make sure half of all readiness transitions of a connectivity-enabled iface and a simple
 * iface results in all expected events.
 */
ZTEST(conn_mgr_monitor, test_cycle_ready_CNC)
{
	cycle_ready_ifaces(if_conn_a, if_simp_a);
}

/* Make sure the other half of all readiness transitions of a connectivity-enabled iface and a
 * simple iface results in all expected events.
 */
ZTEST(conn_mgr_monitor, test_cycle_ready_NCC)
{
	cycle_ready_ifaces(if_simp_a, if_conn_a);
}

/* Make sure all readiness transitions of a pair of simple ifaces results in all expected events.
 */
ZTEST(conn_mgr_monitor, test_cycle_ready_NCNC)
{
	cycle_ready_ifaces(if_simp_a, if_simp_b);
}

/* Make sure that a simple iface can be successfully ignored without interfering with the events
 * fired by another simple iface
 */
ZTEST(conn_mgr_monitor, test_cycle_ready_NCINC)
{
	cycle_ignored_iface(if_simp_a, if_simp_b);
}

/* Make sure that a connectivity-enabled iface can be successfully ignored without interfering
 * with the events fired by another connectivity-enabled iface
 */
ZTEST(conn_mgr_monitor, test_cycle_ready_CIC)
{
	cycle_ignored_iface(if_conn_a, if_conn_b);
}

/* Make sure that a connectivity-enabled iface can be successfully ignored without interfering
 * with the events fired by a simple iface
 */
ZTEST(conn_mgr_monitor, test_cycle_ready_CINC)
{
	cycle_ignored_iface(if_conn_a, if_simp_a);
}

/* Make sure that a simple iface can be successfully ignored without interfering
 * with the events fired by a connectivity-enabled iface
 */
ZTEST(conn_mgr_monitor, test_cycle_ready_NCIC)
{
	cycle_ignored_iface(if_simp_a, if_conn_a);
}

/* Make sure that DAD readiness is actually verified by conn_mgr_monitor */
ZTEST(conn_mgr_monitor, test_DAD)
{
	struct test_stats stats;

	/* This test specifically requires DAD to function */
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6_DAD);

	/* Take iface up */
	zassert_equal(net_if_up(if_simp_a), 0, "net_if_up should succeed.");

	/* Add IPv6 */
	net_if_ipv6_addr_add(if_simp_a, &test_ipv6_a, NET_ADDR_MANUAL, 0);

	/* After a delay too short for DAD, ensure no events */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired before DAD success.");

	/* After a delay long enough for DAD, ensure connectivity acquired */
	k_sleep(DAD_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.conn_count_gen, 1,
			"NET_EVENT_L4_CONNECTED should be fired after DAD success.");
}

/* Test whether ignoring and un-ignoring a ready iface fires the appropriate events */
ZTEST(conn_mgr_monitor, test_ignore_while_ready)
{
	struct test_stats stats;

	/* Ignore iface */
	conn_mgr_ignore_iface(if_simp_a);

	/* Add IP and take iface up */
	net_if_ipv4_addr_add(if_simp_a, &test_ipv4_a, NET_ADDR_MANUAL, 0);
	net_if_ipv6_addr_add(if_simp_a, &test_ipv6_a, NET_ADDR_MANUAL, 0);
	zassert_equal(net_if_up(if_simp_a), 0, "net_if_up should succeed for if_simp_a.");

	/* Ensure no events */
	k_sleep(DAD_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if connecting iface is ignored.");
	zassert_equal(stats.event_count_ipv4, 0,
		"No events should be fired if connecting iface is ignored.");
	zassert_equal(stats.event_count_ipv6, 0,
		"No events should be fired if connecting iface is ignored.");

	/* Watch iface */
	conn_mgr_watch_iface(if_simp_a);

	/* Ensure connectivity gained */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.conn_count_gen, 1,
		"NET_EVENT_L4_CONNECTED should be fired when online iface is watched.");
	zassert_equal(stats.event_count_gen, 1,
		"Only NET_EVENT_L4_CONNECTED should be fired.");
	zassert_equal(stats.conn_count_ipv4, 1,
		"NET_EVENT_L4_IPV4_CONNECTED should be fired when online iface is watched.");
	zassert_equal(stats.event_count_ipv4, 1,
		"Only NET_EVENT_L4_IPV4_CONNECTED should be fired.");
	zassert_equal(stats.conn_count_ipv6, 1,
		"NET_EVENT_L4_IPV6_CONNECTED should be fired when online iface is watched.");
	zassert_equal(stats.event_count_ipv6, 1,
		"Only NET_EVENT_L4_IPV6_CONNECTED should be fired.");
	zassert_equal(stats.conn_iface_gen, if_simp_a, "if_simp_a should be blamed");
	zassert_equal(stats.conn_iface_ipv4, if_simp_a, "if_simp_a should be blamed");
	zassert_equal(stats.conn_iface_ipv6, if_simp_a, "if_simp_a should be blamed");


	/* Ignore iface */
	conn_mgr_ignore_iface(if_simp_a);

	/* Ensure connectivity lost */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.dconn_count_gen, 1,
		"NET_EVENT_L4_DISCONNECTED should be fired when online iface is ignored.");
	zassert_equal(stats.event_count_gen, 1,
		"Only NET_EVENT_L4_DISCONNECTED should be fired.");
	zassert_equal(stats.dconn_count_ipv4, 1,
		"NET_EVENT_L4_IPV4_DISCONNECTED should be fired when online iface is ignored.");
	zassert_equal(stats.event_count_ipv4, 1,
		"Only NET_EVENT_L4_IPV4_DISCONNECTED should be fired.");
	zassert_equal(stats.dconn_count_ipv6, 1,
		"NET_EVENT_L4_IPV6_DISCONNECTED should be fired when online iface is ignored.");
	zassert_equal(stats.event_count_ipv6, 1,
		"Only NET_EVENT_L4_IPV6_DISCONNECTED should be fired.");
	zassert_equal(stats.dconn_iface_gen, if_simp_a, "if_simp_a should be blamed");
	zassert_equal(stats.dconn_iface_ipv4, if_simp_a, "if_simp_a should be blamed");
	zassert_equal(stats.dconn_iface_ipv6, if_simp_a, "if_simp_a should be blamed");

	/* Take iface down*/
	zassert_equal(net_if_down(if_simp_a), 0, "net_if_down should succeed for if_simp_a.");

	/* Ensure no events */
	k_sleep(EVENT_WAIT_TIME);
	stats = get_reset_stats();
	zassert_equal(stats.event_count_gen, 0,
		"No events should be fired if disconnecting iface is ignored.");
	zassert_equal(stats.event_count_ipv4, 0,
		"No events should be fired if disconnecting iface is ignored.");
	zassert_equal(stats.event_count_ipv6, 0,
		"No events should be fired if disconnecting iface is ignored.");
}

/* Test L2 and iface ignore API */
ZTEST(conn_mgr_monitor, test_ignores)
{
	/* Ignore if_simp_a, ensuring if_simp_b is unaffected */
	conn_mgr_ignore_iface(if_simp_a);
	zassert_true(conn_mgr_is_iface_ignored(if_simp_a),
			"if_simp_a should be ignored.");
	zassert_false(conn_mgr_is_iface_ignored(if_simp_b),
			"if_simp_b should not be affected.");

	/* Ignore if_simp_b, ensuring if_simp_a is unaffected */
	conn_mgr_ignore_iface(if_simp_b);
	zassert_true(conn_mgr_is_iface_ignored(if_simp_b),
			"if_simp_b should be ignored.");
	zassert_true(conn_mgr_is_iface_ignored(if_simp_a),
			"if_simp_a should not be affected.");

	/* Watch if_simp_a, ensuring if_simp_b is unaffected */
	conn_mgr_watch_iface(if_simp_a);
	zassert_false(conn_mgr_is_iface_ignored(if_simp_a),
			"if_simp_a should be watched.");
	zassert_true(conn_mgr_is_iface_ignored(if_simp_b),
			"if_simp_b should not be affected.");

	/* Watch if_simp_b, ensuring if_simp_a is unaffected */
	conn_mgr_watch_iface(if_simp_b);
	zassert_false(conn_mgr_is_iface_ignored(if_simp_b),
			"if_simp_b should be watched.");
	zassert_false(conn_mgr_is_iface_ignored(if_simp_a),
			"if_simp_a should not be affected.");

	/* Ignore the entire DUMMY_L2, ensuring all ifaces except if_dummy_eth are affected */
	conn_mgr_ignore_l2(&NET_L2_GET_NAME(DUMMY));
	zassert_true(conn_mgr_is_iface_ignored(if_simp_a),
		"All DUMMY_L2 ifaces should be ignored.");
	zassert_true(conn_mgr_is_iface_ignored(if_simp_b),
		"All DUMMY_L2 ifaces should be ignored.");
	zassert_true(conn_mgr_is_iface_ignored(if_conn_a),
		"All DUMMY_L2 ifaces should be ignored.");
	zassert_true(conn_mgr_is_iface_ignored(if_conn_b),
		"All DUMMY_L2 ifaces should be ignored.");
	zassert_false(conn_mgr_is_iface_ignored(if_dummy_eth),
		"if_dummy_eth should not be affected.");

	/* Watch the entire DUMMY_L2, ensuring all ifaces except if_dummy_eth are affected */
	conn_mgr_watch_l2(&NET_L2_GET_NAME(DUMMY));
	zassert_false(conn_mgr_is_iface_ignored(if_simp_a),
		"All DUMMY_L2 ifaces should be watched.");
	zassert_false(conn_mgr_is_iface_ignored(if_simp_b),
		"All DUMMY_L2 ifaces should be watched.");
	zassert_false(conn_mgr_is_iface_ignored(if_conn_a),
		"All DUMMY_L2 ifaces should be watched.");
	zassert_false(conn_mgr_is_iface_ignored(if_conn_b),
		"All DUMMY_L2 ifaces should be watched.");
	zassert_false(conn_mgr_is_iface_ignored(if_dummy_eth),
		"if_dummy_eth should not be affected.");
}

/* Make sure all state transitions of a single connectivity-enabled iface result in all expected
 * events. Perform IPv4 changes before IPv6 changes.
 */
ZTEST(conn_mgr_monitor, test_cycle_states_connected_ipv46)
{
	cycle_iface_states(if_conn_a, IPV4_FIRST);
}

/* Make sure all state transitions of a single connectivity-enabled iface result in all expected
 * events. Perform IPv6 changes before IPv4 changes.
 */
ZTEST(conn_mgr_monitor, test_cycle_states_connected_ipv64)
{
	cycle_iface_states(if_conn_a, IPV6_FIRST);
}

/* Make sure all state transitions of a single simple iface result in all expected events.
 * Perform IPv4 changes before IPv6 changes.
 */
ZTEST(conn_mgr_monitor, test_cycle_states_simple_ipv46)
{
	cycle_iface_states(if_simp_a, IPV4_FIRST);
}

/* Make sure all state transitions of a single simple iface result in all expected events.
 * Perform IPv6 changes before IPv4 changes.
 */
ZTEST(conn_mgr_monitor, test_cycle_states_simple_ipv64)
{
	cycle_iface_states(if_simp_a, IPV6_FIRST);
}

ZTEST_SUITE(conn_mgr_monitor, NULL, conn_mgr_setup, conn_mgr_before, NULL, NULL);
