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

#include <zephyr/net/conn_mgr_connectivity.h>
#include "conn_mgr_private.h"

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(7)
#endif

/* Internal state array tracking readiness, flags, and other state information for all available
 * ifaces. Note that indexing starts at 0, whereas Zephyr iface indices start at 1.
 */
uint16_t iface_states[CONN_MGR_IFACE_MAX];

/* Used to signal when modifications have been made that need to be responded to */
K_SEM_DEFINE(conn_mgr_event_signal, 1, 1);

/* Used to protect conn_mgr state */
K_MUTEX_DEFINE(conn_mgr_lock);

#if defined(CONFIG_NET_IPV6)
static bool conn_mgr_is_if_ipv6_ready(int index)
{
	if ((iface_states[index] & CONN_MGR_IPV6_STATUS_MASK) == CONN_MGR_IPV6_STATUS_MASK) {
		NET_DBG("IPv6 connected on iface index %u", index + 1);
		return true;
	}

	return false;
}
#else
#define conn_mgr_is_if_ipv6_ready(...) false
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
static bool conn_mgr_is_if_ipv4_ready(int index)
{
	if ((iface_states[index] & CONN_MGR_IPV4_STATUS_MASK) == CONN_MGR_IPV4_STATUS_MASK) {
		NET_DBG("IPv4 connected on iface index %u", index + 1);
		return true;
	}

	return false;
}
#else
#define conn_mgr_is_if_ipv4_ready(...) false
#endif /* CONFIG_NET_IPV4 */

/**
 * @brief Retrieves pointer to an iface by the index that corresponds to it in iface_states
 *
 * @param index - The index in iface_states to find the corresponding iface for.
 * @return net_if* - The corresponding iface.
 */
static struct net_if *conn_mgr_get_if_by_index(int index)
{
	return net_if_get_by_index(index + 1);
}

/**
 * @brief Notifies listeners of the current readiness state of the iface at the given index
 *
 * @param index - Index of the iface (in iface_states)
 */
static void conn_mgr_notify_if_readiness(int index)
{
	struct net_if *iface = conn_mgr_get_if_by_index(index);
	bool readiness = iface_states[index] & CONN_MGR_IF_READY;

	if (iface == NULL) {
		return;
	}

	NET_DBG("Iface %d (%p) %s", net_if_get_by_iface(iface),
		iface, readiness ? "ready" : "unready");

	net_mgmt_event_notify(
		readiness ? NET_EVENT_L4_CONNECTED : NET_EVENT_L4_DISCONNECTED,
		iface
	);
}

/**
 * @brief Marks an iface as ready or unready and updates all associated state tracking.
 *
 * @param idx - index (in iface_states) of the iface to mark ready or unready
 * @param readiness - true if the iface should be considered ready, otherwise false
 */
static void conn_mgr_set_ready(int idx, bool readiness)
{
	/* Clear and then update the L4-readiness bit */
	iface_states[idx] &= ~CONN_MGR_IF_READY;
	if (readiness) {
		iface_states[idx] |= CONN_MGR_IF_READY;
	}
}

static void conn_mgr_act_on_changes(void)
{
	int idx;
	bool is_ip_ready;
	bool is_l4_ready;
	bool is_oper_up;
	bool was_l4_ready;

	k_mutex_lock(&conn_mgr_lock, K_FOREVER);

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
		is_ip_ready  =	conn_mgr_is_if_ipv6_ready(idx) || conn_mgr_is_if_ipv4_ready(idx);
		is_oper_up   =	iface_states[idx] & CONN_MGR_IF_UP;
		was_l4_ready =	iface_states[idx] & CONN_MGR_IF_READY;
		is_l4_ready  =	is_oper_up && is_ip_ready;

		/* Respond to changes to iface readiness */
		if (was_l4_ready != is_l4_ready) {
			/* Track the iface readiness change */
			conn_mgr_set_ready(idx, is_l4_ready);

			/* Notify listeners of the readiness change */
			conn_mgr_notify_if_readiness(idx);
		}
	}
	k_mutex_unlock(&conn_mgr_lock);
}

/**
 * @brief Initialize the internal state flags for the given iface using its current status
 *
 * @param iface - iface to initialize from.
 */
static void conn_mgr_initial_state(struct net_if *iface)
{
	int idx = net_if_get_by_iface(iface) - 1;

	k_mutex_lock(&conn_mgr_lock, K_FOREVER);

	if (net_if_is_up(iface)) {
		NET_DBG("Iface %p UP", iface);
		iface_states[idx] = CONN_MGR_IF_UP;
	}

	if (IS_ENABLED(CONFIG_NET_NATIVE_IPV6)) {
		if (net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &iface)) {
			NET_DBG("IPv6 addr set");
			iface_states[idx] |= CONN_MGR_IF_IPV6_SET | CONN_MGR_IF_IPV6_DAD_OK;
		} else if (net_if_ipv6_get_global_addr(NET_ADDR_TENTATIVE,
						       &iface)) {
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

	k_mutex_unlock(&conn_mgr_lock);
}

static void conn_mgr_init_cb(struct net_if *iface, void *user_data)
{
	ARG_UNUSED(user_data);

	conn_mgr_initial_state(iface);
}

static void conn_mgr_handler(void)
{
	k_mutex_lock(&conn_mgr_lock, K_FOREVER);

	conn_mgr_conn_init();

	conn_mgr_init_events_handler();

	net_if_foreach(conn_mgr_init_cb, NULL);

	k_mutex_unlock(&conn_mgr_lock);

	NET_DBG("Connection Manager started");

	while (true) {
		/* Wait for changes */
		k_sem_take(&conn_mgr_event_signal, K_FOREVER);

		/* Respond to changes */
		conn_mgr_act_on_changes();
	}
}

K_THREAD_DEFINE(conn_mgr, CONFIG_NET_CONNECTION_MANAGER_STACK_SIZE,
		(k_thread_entry_t)conn_mgr_handler, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, 0);

void conn_mgr_resend_status(void)
{
	int idx;

	k_mutex_lock(&conn_mgr_lock, K_FOREVER);

	for (idx = 0; idx < ARRAY_SIZE(iface_states); idx++) {
		conn_mgr_notify_if_readiness(idx);
	}

	k_mutex_unlock(&conn_mgr_lock);
}

static int conn_mgr_init(void)
{
	int i;


	for (i = 0; i < ARRAY_SIZE(iface_states); i++) {
		iface_states[i] = 0;
	}

	k_thread_start(conn_mgr);

	return 0;
}

SYS_INIT(conn_mgr_init, APPLICATION, CONFIG_NET_CONNECTION_MANAGER_PRIORITY);
