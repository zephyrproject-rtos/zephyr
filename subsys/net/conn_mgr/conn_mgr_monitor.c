/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(conn_mgr, CONFIG_NET_CONNECTION_MANAGER_LOG_LEVEL);

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include "conn_mgr_private.h"

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(7)
#endif

static K_THREAD_STACK_DEFINE(conn_mgr_mon_stack,
			     CONFIG_NET_CONNECTION_MANAGER_MONITOR_STACK_SIZE);
static struct k_thread conn_mgr_mon_thread;

/* Internal state array tracking readiness, flags, and other state information for all available
 * ifaces. Note that indexing starts at 0, whereas Zephyr iface indices start at 1.
 * conn_mgr_mon_get_if_by_index and conn_mgr_get_index_for_if are used to go back and forth between
 * iface_states indices and Zephyr iface pointers.
 */
uint16_t iface_states[CONN_MGR_IFACE_MAX];

/* Tracks the most recent total quantity of L4-ready ifaces (any, IPv4, IPv6) */
static uint16_t last_ready_count;
static uint16_t last_ready_count_ipv4;
static uint16_t last_ready_count_ipv6;

/* Tracks the last ifaces to cause a major state change (any, IPv4, IPv6) */
static struct net_if *last_blame;
static struct net_if *last_blame_ipv4;
static struct net_if *last_blame_ipv6;

/* Used to signal when modifications have been made that need to be responded to */
K_SEM_DEFINE(conn_mgr_mon_updated, 1, 1);

/* Used to protect conn_mgr_monitor state */
K_MUTEX_DEFINE(conn_mgr_mon_lock);

/**
 * @brief Retrieves pointer to an iface by the index that corresponds to it in iface_states
 *
 * @param index - The index in iface_states to find the corresponding iface for.
 * @return net_if* - The corresponding iface.
 */
static struct net_if *conn_mgr_mon_get_if_by_index(int index)
{
	return net_if_get_by_index(index + 1);
}

/**
 * @brief Gets the index in iface_states for the state corresponding to a provided iface.
 *
 * @param iface - iface to find the index of.
 * @return int - The index found.
 */
static int conn_mgr_get_index_for_if(struct net_if *iface)
{
	return net_if_get_by_iface(iface) - 1;
}

/**
 * @brief Conveniently update iface readiness state
 *
 * @param idx - index (in iface_states) of the iface to mark ready or unready
 * @param ready - true if the iface should be considered ready, otherwise false
 * @param ready_ipv4 - true if the iface is ready with IPv4, otherwise false
 * @param ready_ipv6 - true if the iface is ready with IPv6, otherwise false
 */
static void conn_mgr_mon_set_ready(int idx, bool ready, bool ready_ipv4, bool ready_ipv6)
{
	/* Clear and then update the L4-readiness bit */
	iface_states[idx] &= ~CONN_MGR_IF_READY;
	iface_states[idx] &= ~CONN_MGR_IF_READY_IPV4;
	iface_states[idx] &= ~CONN_MGR_IF_READY_IPV6;

	if (ready) {
		iface_states[idx] |= CONN_MGR_IF_READY;
	}

	if (ready_ipv4) {
		iface_states[idx] |= CONN_MGR_IF_READY_IPV4;
	}

	if (ready_ipv6) {
		iface_states[idx] |= CONN_MGR_IF_READY_IPV6;
	}
}

static void conn_mgr_mon_handle_update(void)
{
	int idx;
	bool has_ip;
	bool has_ipv6;
	bool has_ipv4;
	bool is_l4_ready;
	bool is_ipv6_ready;
	bool is_ipv4_ready;
	bool is_oper_up;
	bool was_l4_ready;
	bool was_ipv6_ready;
	bool was_ipv4_ready;
	bool is_ignored;
	int ready_count = 0;
	int ready_count_ipv4 = 0;
	int ready_count_ipv6 = 0;
	struct net_if *blame = NULL;
	struct net_if *blame_ipv4 = NULL;
	struct net_if *blame_ipv6 = NULL;

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	for (idx = 0; idx < ARRAY_SIZE(iface_states); idx++) {
		if (iface_states[idx] == 0) {
			/* This interface is not used */
			continue;
		}

		/* Detect whether iface was previously considered ready */
		was_l4_ready	= iface_states[idx] & CONN_MGR_IF_READY;
		was_ipv6_ready	= iface_states[idx] & CONN_MGR_IF_READY_IPV6;
		was_ipv4_ready	= iface_states[idx] & CONN_MGR_IF_READY_IPV4;

		/* Collect iface readiness requirements */
		has_ipv6	= iface_states[idx] & CONN_MGR_IF_IPV6_SET;
		has_ipv4	= iface_states[idx] & CONN_MGR_IF_IPV4_SET;
		has_ip		= has_ipv6 || has_ipv4;
		is_oper_up	= iface_states[idx] & CONN_MGR_IF_UP;
		is_ignored	= iface_states[idx] & CONN_MGR_IF_IGNORED;

		/* Determine whether iface is currently considered ready */
		is_l4_ready	= is_oper_up && has_ip   && !is_ignored;
		is_ipv6_ready	= is_oper_up && has_ipv6 && !is_ignored;
		is_ipv4_ready	= is_oper_up && has_ipv4 && !is_ignored;

		/* Track ready iface count */
		if (is_l4_ready) {
			ready_count += 1;
		}
		if (is_ipv6_ready) {
			ready_count_ipv6 += 1;
		}
		if (is_ipv4_ready) {
			ready_count_ipv4 += 1;
		}

		/* If any states changed, track blame for possibly triggered events */
		if (was_l4_ready != is_l4_ready) {
			blame = conn_mgr_mon_get_if_by_index(idx);
		}
		if (was_ipv6_ready != is_ipv6_ready) {
			blame_ipv6 = conn_mgr_mon_get_if_by_index(idx);
		}
		if (was_ipv4_ready != is_ipv4_ready) {
			blame_ipv4 = conn_mgr_mon_get_if_by_index(idx);
		}

		/* Update readiness state flags with the (possibly) new values */
		conn_mgr_mon_set_ready(idx, is_l4_ready, is_ipv4_ready, is_ipv6_ready);
	}

	/* If the total number of ready ifaces changed, possibly send an event */
	if (ready_count != last_ready_count) {
		if (ready_count == 0) {
			/* We just lost connectivity */
			net_mgmt_event_notify(NET_EVENT_L4_DISCONNECTED, blame);
		} else if (last_ready_count == 0) {
			/* We just gained connectivity */
			net_mgmt_event_notify(NET_EVENT_L4_CONNECTED, blame);
		}
		last_ready_count = ready_count;
		last_blame = blame;
	}

	/* Same, but specifically for IPv4 */
	if (ready_count_ipv4 != last_ready_count_ipv4) {
		if (ready_count_ipv4 == 0) {
			/* We just lost IPv4 connectivity */
			net_mgmt_event_notify(NET_EVENT_L4_IPV4_DISCONNECTED, blame_ipv4);
		} else if (last_ready_count_ipv4 == 0) {
			/* We just gained IPv4 connectivity */
			net_mgmt_event_notify(NET_EVENT_L4_IPV4_CONNECTED, blame_ipv4);
		}
		last_ready_count_ipv4 = ready_count_ipv4;
		last_blame_ipv4 = blame_ipv4;
	}

	/* Same, but specifically for IPv6 */
	if (ready_count_ipv6 != last_ready_count_ipv6) {
		if (ready_count_ipv6 == 0) {
			/* We just lost IPv6 connectivity */
			net_mgmt_event_notify(NET_EVENT_L4_IPV6_DISCONNECTED, blame_ipv6);
		} else if (last_ready_count_ipv6 == 0) {
			/* We just gained IPv6 connectivity */
			net_mgmt_event_notify(NET_EVENT_L4_IPV6_CONNECTED, blame_ipv6);
		}
		last_ready_count_ipv6 = ready_count_ipv6;
		last_blame_ipv6 = blame_ipv6;
	}

	k_mutex_unlock(&conn_mgr_mon_lock);
}

/**
 * @brief Initialize the internal state flags for the given iface using its current status
 *
 * @param iface - iface to initialize from.
 */
static void conn_mgr_mon_initial_state(struct net_if *iface)
{
	int idx = conn_mgr_get_index_for_if(iface);

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	if (net_if_is_up(iface)) {
		NET_DBG("Iface %p UP", iface);
		iface_states[idx] |= CONN_MGR_IF_UP;
	}

	if (IS_ENABLED(CONFIG_NET_NATIVE_IPV6)) {
		if (net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &iface)) {
			NET_DBG("IPv6 addr set");
			iface_states[idx] |= CONN_MGR_IF_IPV6_SET;
		}
	}

	if (IS_ENABLED(CONFIG_NET_NATIVE_IPV4)) {
		if (net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED)) {
			NET_DBG("IPv4 addr set");
			iface_states[idx] |= CONN_MGR_IF_IPV4_SET;
		}

	}

	k_mutex_unlock(&conn_mgr_mon_lock);
}

static void conn_mgr_mon_init_cb(struct net_if *iface, void *user_data)
{
	ARG_UNUSED(user_data);

	conn_mgr_mon_initial_state(iface);
}

static void conn_mgr_mon_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	conn_mgr_conn_init();

	conn_mgr_init_events_handler();

	net_if_foreach(conn_mgr_mon_init_cb, NULL);

	k_mutex_unlock(&conn_mgr_mon_lock);

	NET_DBG("Connection Manager started");

	while (true) {
		/* Wait for changes */
		k_sem_take(&conn_mgr_mon_updated, K_FOREVER);

		/* Respond to changes */
		conn_mgr_mon_handle_update();
	}
}

void conn_mgr_mon_resend_status(void)
{
	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	if (last_ready_count == 0) {
		net_mgmt_event_notify(NET_EVENT_L4_DISCONNECTED, last_blame);
	} else {
		net_mgmt_event_notify(NET_EVENT_L4_CONNECTED, last_blame);
	}

	if (last_ready_count_ipv6 == 0) {
		net_mgmt_event_notify(NET_EVENT_L4_IPV6_DISCONNECTED, last_blame_ipv6);
	} else {
		net_mgmt_event_notify(NET_EVENT_L4_IPV6_CONNECTED, last_blame_ipv6);
	}

	if (last_ready_count_ipv4 == 0) {
		net_mgmt_event_notify(NET_EVENT_L4_IPV4_DISCONNECTED, last_blame_ipv4);
	} else {
		net_mgmt_event_notify(NET_EVENT_L4_IPV4_CONNECTED, last_blame_ipv4);
	}

	k_mutex_unlock(&conn_mgr_mon_lock);
}

void conn_mgr_ignore_iface(struct net_if *iface)
{
	int idx = conn_mgr_get_index_for_if(iface);

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	if (!(iface_states[idx] & CONN_MGR_IF_IGNORED)) {
		/* Set ignored flag and mark state as changed */
		iface_states[idx] |= CONN_MGR_IF_IGNORED;
		k_sem_give(&conn_mgr_mon_updated);
	}

	k_mutex_unlock(&conn_mgr_mon_lock);
}

void conn_mgr_watch_iface(struct net_if *iface)
{
	int idx = conn_mgr_get_index_for_if(iface);

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	if (iface_states[idx] & CONN_MGR_IF_IGNORED) {
		/* Clear ignored flag and mark state as changed */
		iface_states[idx] &= ~CONN_MGR_IF_IGNORED;
		k_sem_give(&conn_mgr_mon_updated);
	}

	k_mutex_unlock(&conn_mgr_mon_lock);
}

bool conn_mgr_is_iface_ignored(struct net_if *iface)
{
	int idx = conn_mgr_get_index_for_if(iface);

	bool ret = false;

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	ret = iface_states[idx] & CONN_MGR_IF_IGNORED;

	k_mutex_unlock(&conn_mgr_mon_lock);

	return ret;
}

/**
 * @brief Check whether a provided iface uses the provided L2.
 *
 * @param iface - iface to check.
 * @param l2 - L2 to check. NULL will match offloaded ifaces.
 * @retval true if the iface uses the provided L2.
 * @retval false otherwise.
 */
static bool iface_uses_l2(struct net_if *iface, const struct net_l2 *l2)
{
	return	(!l2 && net_if_offload(iface)) ||
		(net_if_l2(iface) == l2);
}

void conn_mgr_ignore_l2(const struct net_l2 *l2)
{
	/* conn_mgr_ignore_iface already locks the mutex, but we lock it here too
	 * so that all matching ifaces are updated simultaneously.
	 */
	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (iface_uses_l2(iface, l2)) {
			conn_mgr_ignore_iface(iface);
		}
	}

	k_mutex_unlock(&conn_mgr_mon_lock);
}

void conn_mgr_watch_l2(const struct net_l2 *l2)
{
	/* conn_mgr_watch_iface already locks the mutex, but we lock it here too
	 * so that all matching ifaces are updated simultaneously.
	 */
	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (iface_uses_l2(iface, l2)) {
			conn_mgr_watch_iface(iface);
		}
	}

	k_mutex_unlock(&conn_mgr_mon_lock);
}

static int conn_mgr_mon_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(iface_states); i++) {
		iface_states[i] = 0;
	}

	k_thread_create(&conn_mgr_mon_thread, conn_mgr_mon_stack,
			CONFIG_NET_CONNECTION_MANAGER_MONITOR_STACK_SIZE,
			conn_mgr_mon_thread_fn,
			NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&conn_mgr_mon_thread, "conn_mgr_monitor");

	return 0;
}

uint16_t conn_mgr_if_state(struct net_if *iface)
{
	int idx = conn_mgr_get_index_for_if(iface);
	uint16_t state = CONN_MGR_IF_STATE_INVALID;

	if (idx < CONN_MGR_IFACE_MAX) {
		k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);
		state = iface_states[idx];
		k_mutex_unlock(&conn_mgr_mon_lock);
	}

	return state;
}

SYS_INIT(conn_mgr_mon_init, APPLICATION, CONFIG_NET_CONNECTION_MANAGER_MONITOR_PRIORITY);
