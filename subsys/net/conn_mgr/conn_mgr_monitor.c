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

/* Tracks the total number of L4-ready ifaces */
static uint16_t ready_count;

/* Tracks the last ifaces to change state in each respective direction */
static struct net_if *last_iface_down;
static struct net_if *last_iface_up;

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
 * @brief Marks an iface as ready or unready and updates all associated state tracking.
 *
 * @param idx - index (in iface_states) of the iface to mark ready or unready
 * @param readiness - true if the iface should be considered ready, otherwise false
 */
static void conn_mgr_mon_set_ready(int idx, bool readiness)
{
	/* Clear and then update the L4-readiness bit */
	iface_states[idx] &= ~CONN_MGR_IF_READY;

	if (readiness) {
		iface_states[idx] |= CONN_MGR_IF_READY;

		ready_count += 1;
		last_iface_up = conn_mgr_mon_get_if_by_index(idx);
	} else {
		ready_count -= 1;
		last_iface_down = conn_mgr_mon_get_if_by_index(idx);
	}
}

static void conn_mgr_mon_handle_update(void)
{
	int idx;
	int original_ready_count;
	bool is_ip_ready;
	bool is_ipv6_ready;
	bool is_ipv4_ready;
	bool is_l4_ready;
	bool is_oper_up;
	bool was_l4_ready;
	bool is_ignored;

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	original_ready_count = ready_count;
	for (idx = 0; idx < ARRAY_SIZE(iface_states); idx++) {
		if (iface_states[idx] == 0) {
			/* This interface is not used */
			continue;
		}

		if (!(iface_states[idx] & CONN_MGR_IF_CHANGED)) {
			/* No changes on this iface */
			continue;
		}

		/* Clear the state-change flag */
		iface_states[idx] &= ~CONN_MGR_IF_CHANGED;

		/* Detect whether the iface is currently or was L4 ready */
		was_l4_ready	= iface_states[idx] & CONN_MGR_IF_READY;
		is_ipv6_ready	= iface_states[idx] & CONN_MGR_IF_IPV6_SET;
		is_ipv4_ready	= iface_states[idx] & CONN_MGR_IF_IPV4_SET;
		is_oper_up	= iface_states[idx] & CONN_MGR_IF_UP;
		is_ignored	= iface_states[idx] & CONN_MGR_IF_IGNORED;
		is_ip_ready	= is_ipv6_ready || is_ipv4_ready;
		is_l4_ready	= is_oper_up && is_ip_ready && !is_ignored;

		/* Respond to changes to iface readiness */
		if (was_l4_ready != is_l4_ready) {
			/* Track the iface readiness change */
			conn_mgr_mon_set_ready(idx, is_l4_ready);
		}
	}

	/* If the total number of ready ifaces changed, possibly send an event */
	if (ready_count != original_ready_count) {
		if (ready_count == 0) {
			/* We just lost connectivity */
			net_mgmt_event_notify(NET_EVENT_L4_DISCONNECTED, last_iface_down);
		} else if (original_ready_count == 0) {
			/* We just gained connectivity */
			net_mgmt_event_notify(NET_EVENT_L4_CONNECTED, last_iface_up);
		}
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
	int idx = net_if_get_by_iface(iface) - 1;

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

	iface_states[idx] |= CONN_MGR_IF_CHANGED;

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

	if (ready_count == 0) {
		net_mgmt_event_notify(NET_EVENT_L4_DISCONNECTED, last_iface_down);
	} else {
		net_mgmt_event_notify(NET_EVENT_L4_CONNECTED, last_iface_up);
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
		iface_states[idx] |= CONN_MGR_IF_CHANGED;
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
		iface_states[idx] |= CONN_MGR_IF_CHANGED;
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

SYS_INIT(conn_mgr_mon_init, APPLICATION, CONFIG_NET_CONNECTION_MANAGER_MONITOR_PRIORITY);
