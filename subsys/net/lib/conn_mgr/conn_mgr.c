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

#include <conn_mgr.h>

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(7)
#endif

uint16_t iface_states[CONN_MGR_IFACE_MAX];

/* Used to signal when modifications have been made that need to be responded to */
K_SEM_DEFINE(conn_mgr_event_signal, 1, 1);

/* Used to protect conn_mgr state */
K_MUTEX_DEFINE(conn_mgr_lock);

static bool conn_mgr_iface_is_oper_up(int index)
{
	return (iface_states[index] & CMGR_IF_ST_UP) != 0;
}

#if defined(CONFIG_NET_IPV6)
static bool conn_mgr_ipv6_status(int index)
{
	bool ready = (iface_states[index] & CONN_MGR_IPV6_STATUS_MASK) == CONN_MGR_IPV6_STATUS_MASK;
	if (ready) {
		NET_DBG("IPv6 connected on iface index %u", index + 1);
	}

	return ready;
}
#else
#define conn_mgr_ipv6_status(...) true
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
static bool conn_mgr_ipv4_status(int index)
{
	bool ready = (iface_states[index] & CONN_MGR_IPV4_STATUS_MASK) == CONN_MGR_IPV4_STATUS_MASK;
	if (ready) {
		NET_DBG("IPv4 connected on iface index %u", index + 1);
	}

	return ready;
}
#else
#define conn_mgr_ipv4_status(...) true
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
	bool readiness = iface_states[index] & CMGR_IF_ST_READY;

	if (iface == NULL) {
		return;
	}

	NET_DBG("Iface %d (%p) %s", net_if_get_by_iface(iface),
		iface, readiness ? "ready" : "unready");

	net_mgmt_event_notify(readiness ? NET_EVENT_L4_IF_READY : NET_EVENT_L4_IF_UNREADY, iface);
}

/**
 * @brief Takes the iface at the specified index admin-down
 *
 * @param idx - Index of the iface (in iface_states)
 */
static void conn_mgr_iface_down(int idx) {
	net_if_down(conn_mgr_get_if_by_index(idx));
}

static void conn_mgr_act_on_changes(void)
{
	int idx;

	k_mutex_lock(&conn_mgr_lock, K_FOREVER);

	for (idx = 0; idx < ARRAY_SIZE(iface_states); idx++) {
		bool is_ip_ready;
		bool is_l4_ready;
		bool was_l4_ready;

		if (iface_states[idx] == 0) {
			/* This interface is not used */
			continue;
		}

		if (!(iface_states[idx] & CMGR_IF_EVT_CHANGED)) {
			/* No changes on this iface */
			continue;
		}

		/* Clear the state-change flag */
		iface_states[idx] &= ~CMGR_IF_EVT_CHANGED;

		/* Trigger iface admin-down if it has been requested */
		if (iface_states[idx] & CMGR_IF_EVT_REQ_DOWN) {
			/* Clear the down-request flag */
			iface_states[idx] &= ~CMGR_IF_EVT_REQ_DOWN;

			/* Perform the request */
			conn_mgr_iface_down(idx);
		}

		/* Detect whether the iface is currently or was L4 ready */
		is_ip_ready =	(IS_ENABLED(CONFIG_NET_IPV6) && conn_mgr_ipv6_status(idx)) ||
				(IS_ENABLED(CONFIG_NET_IPV4) && conn_mgr_ipv4_status(idx));
		is_l4_ready =	conn_mgr_iface_is_oper_up(idx) && is_ip_ready;
		was_l4_ready =	iface_states[idx] & CMGR_IF_ST_READY;

		/* Respond to changes to iface readiness */
		if (was_l4_ready != is_l4_ready) {
			/* Clear and then update the L4-readiness bit */
			iface_states[idx] &= ~CMGR_IF_ST_READY;
			iface_states[idx] |= is_l4_ready * CMGR_IF_ST_READY;

			/* Notify listeners of the readiness change */
			conn_mgr_notify_if_readiness(idx);
		}

		/* Trigger iface admin-down if it has been requested */
		if (iface_states[idx] & CMGR_IF_EVT_REQ_DOWN) {
			/* Clear the down-request flag */
			iface_states[idx] &= ~CMGR_IF_EVT_REQ_DOWN;

			/* Perform the request */
			conn_mgr_iface_down(idx);
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
		iface_states[idx] = CMGR_IF_ST_UP;
	}

	if (IS_ENABLED(CONFIG_NET_NATIVE_IPV6)) {
		if (net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &iface)) {
			NET_DBG("IPv6 addr set");
			iface_states[idx] |= CMGR_IF_ST_IPV6_SET |
						CMGR_IF_ST_IPV6_DAD_OK;
		} else if (net_if_ipv6_get_global_addr(NET_ADDR_TENTATIVE,
						       &iface)) {
			iface_states[idx] |= CMGR_IF_ST_IPV6_SET;
		}
	}

	if (IS_ENABLED(CONFIG_NET_NATIVE_IPV4)) {
		if (net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED)) {
			NET_DBG("IPv4 addr set");
			iface_states[idx] |= CMGR_IF_ST_IPV4_SET;
		}

	}

	iface_states[idx] |= CMGR_IF_EVT_CHANGED;

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

static void conn_mgr_all_if_up_cb(struct net_if *iface, void *user_data)
{
	int *first_status = (int *)user_data;
	int status = net_if_up(iface);

	if (status == 0) {
		return;
	}

	if (*first_status == 0) {
		*first_status = status;
	}

	NET_ERR("net_if_up failed for Iface %d (%p). ERR: %d",
		net_if_get_by_iface(iface), iface, status);
}

static void conn_mgr_all_if_down_cb(struct net_if *iface, void *user_data)
{
	int *first_status = (int *)user_data;
	int status = net_if_down(iface);

	if (status == 0) {
		return;
	}

	if (*first_status == 0) {
		*first_status = status;
	}

	NET_ERR("net_if_down failed for Iface %d (%p). ERR: %d",
		net_if_get_by_iface(iface), iface, status);
}

static void conn_mgr_all_if_connect_cb(struct net_if *iface, void *user_data)
{
	int *first_status = (int *)user_data;
	int status = 0;

	/* First, take iface up if it isn't already */
	if (!net_if_is_admin_up(iface)) {
		conn_mgr_all_if_up_cb(iface, &status);
	}

	if (status != 0) {
		if (*first_status == 0) {
			*first_status = 0;
		}
		return;
	}

	/* We expect net_if_connect to be supported if the iface has a connectivity API at all.
	 * Otherwise, the API would be incomplete, since the connect function is not optional.
	 */
	if (!net_if_supports_connectivity(iface)) {
		return;
	}

	status = net_if_connect(iface);
	if (status == 0) {
		return;
	}

	if (*first_status == 0) {
		*first_status = status;
	}

	NET_ERR("net_if_connect failed for Iface %d (%p). ERR: %d",
		net_if_get_by_iface(iface), iface, status);
}

static void conn_mgr_all_if_disconnect_cb(struct net_if *iface, void *user_data)
{
	int *first_status = (int *)user_data;
	int status = net_if_disconnect(iface);

	if (status == 0) {
		return;
	}

	if (*first_status == 0) {
		*first_status = status;
	}

	NET_ERR("net_if_disconnect failed for Iface %d (%p). ERR: %d",
		net_if_get_by_iface(iface), iface, status);
}

int net_conn_mgr_all_if_up(void)
{
	int status = 0;

	net_if_foreach(conn_mgr_all_if_up_cb, &status);
	return status;
}

int net_conn_mgr_all_if_down(void)
{
	int status = 0;

	net_if_foreach(conn_mgr_all_if_down_cb, &status);
	return status;
}

int net_conn_mgr_all_if_connect(void)
{
	int status = 0;

	net_if_foreach(conn_mgr_all_if_connect_cb, &status);
	return status;
}

int net_conn_mgr_all_if_disconnect(void)
{
	int status = 0;

	net_if_foreach(conn_mgr_all_if_disconnect_cb, &status);
	return status;
}

void net_conn_mgr_resend_status(void)
{
	int idx;

	k_mutex_lock(&conn_mgr_lock, K_FOREVER);

	for (idx = 0; idx < ARRAY_SIZE(iface_states); idx++) {
		conn_mgr_notify_if_readiness(idx);
	}

	k_mutex_unlock(&conn_mgr_lock);
}

static int conn_mgr_init(const struct device *dev)
{
	int i;

	ARG_UNUSED(dev);

	for (i = 0; i < ARRAY_SIZE(iface_states); i++) {
		iface_states[i] = 0;
	}

	k_thread_start(conn_mgr);

	return 0;
}

SYS_INIT(conn_mgr_init, APPLICATION, CONFIG_NET_CONNECTION_MANAGER_PRIORITY);
