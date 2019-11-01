/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(conn_mgr, CONFIG_NET_CONNECTION_MANAGER_LOG_LEVEL);

#include <init.h>
#include <kernel.h>
#include <errno.h>
#include <net/net_core.h>
#include <net/net_if.h>
#include <net/net_mgmt.h>

#include <conn_mgr.h>

u16_t iface_states[CONN_MGR_IFACE_MAX];

K_SEM_DEFINE(conn_mgr_lock, 1, UINT_MAX);

static enum net_conn_mgr_state conn_mgr_iface_status(int index)
{
	if (iface_states[index] & NET_STATE_IFACE_UP) {
		return NET_CONN_MGR_STATE_CONNECTED;
	}

	return NET_CONN_MGR_STATE_DISCONNECTED;
}

#if defined(CONFIG_NET_IPV6)
static enum net_conn_mgr_state conn_mgr_ipv6_status(int index)
{
	if ((iface_states[index] & CONN_MGR_IPV6_STATUS_MASK) ==
	    CONN_MGR_IPV6_STATUS_MASK) {
		NET_DBG("IPv6 connected on iface index %u", index + 1);
		return NET_CONN_MGR_STATE_CONNECTED;
	}

	return NET_CONN_MGR_STATE_DISCONNECTED;
}
#else
#define conn_mgr_ipv6_status(...) NET_CONN_MGR_STATE_CONNECTED
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
static enum net_conn_mgr_state conn_mgr_ipv4_status(int index)
{
	if ((iface_states[index] & CONN_MGR_IPV4_STATUS_MASK) ==
	    CONN_MGR_IPV4_STATUS_MASK) {
		NET_DBG("IPv4 connected on iface index %u", index + 1);
		return NET_CONN_MGR_STATE_CONNECTED;
	}

	return NET_CONN_MGR_STATE_DISCONNECTED;
}
#else
#define conn_mgr_ipv4_status(...) NET_CONN_MGR_STATE_CONNECTED
#endif /* CONFIG_NET_IPV4 */

static void conn_mgr_notify_status(int index)
{
	struct net_if *iface = net_if_get_by_index(index + 1);

	if (iface_states[index] & NET_STATE_CONNECTED) {
		NET_DBG("Iface %p connected", iface);
		net_mgmt_event_notify(NET_EVENT_L4_CONNECTED, iface);
	} else {
		NET_DBG("Iface %p disconnected", iface);
		net_mgmt_event_notify(NET_EVENT_L4_DISCONNECTED, iface);
	}
}

static void conn_mgr_act_on_changes(void)
{
	int idx;

	for (idx = 0; idx < ARRAY_SIZE(iface_states); idx++) {
		enum net_conn_mgr_state state;

		if (!(iface_states[idx] & NET_STATE_CHANGED)) {
			continue;
		}

		state = NET_CONN_MGR_STATE_CONNECTED;

		state &= conn_mgr_iface_status(idx);
		if (state) {
			enum net_conn_mgr_state ip_state =
				NET_CONN_MGR_STATE_DISCONNECTED;

			if (IS_ENABLED(CONFIG_NET_IPV6)) {
				ip_state |= conn_mgr_ipv6_status(idx);
			}

			if (IS_ENABLED(CONFIG_NET_IPV4)) {
				ip_state |= conn_mgr_ipv4_status(idx);
			}

			state &= ip_state;
		}

		iface_states[idx] &= ~NET_STATE_CHANGED;

		if (state == NET_CONN_MGR_STATE_CONNECTED &&
		    !(iface_states[idx] & NET_STATE_CONNECTED)) {
			iface_states[idx] |= NET_STATE_CONNECTED;

			conn_mgr_notify_status(idx);
		} else if (state != NET_CONN_MGR_STATE_CONNECTED &&
			   (iface_states[idx] & NET_STATE_CONNECTED)) {
			iface_states[idx] &= ~NET_STATE_CONNECTED;

			conn_mgr_notify_status(idx);
		}
	}
}

static void conn_mgr_initial_state(struct net_if *iface)
{
	int idx = net_if_get_by_iface(iface) - 1;

	if (net_if_is_up(iface)) {
		NET_DBG("Iface %p UP", iface);
		iface_states[idx] = NET_STATE_IFACE_UP;
	}

	if (IS_ENABLED(CONFIG_NET_NATIVE_IPV6)) {
		if (net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &iface)) {
			NET_DBG("IPv6 addr set");
			iface_states[idx] |= NET_STATE_IPV6_ADDR_SET |
						NET_STATE_IPV6_DAD_OK;
		} else if (net_if_ipv6_get_global_addr(NET_ADDR_TENTATIVE,
						       &iface)) {
			iface_states[idx] |= NET_STATE_IPV6_ADDR_SET;
		}
	}

	if (IS_ENABLED(CONFIG_NET_NATIVE_IPV4)) {
		if (net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED)) {
			NET_DBG("IPv4 addr set");
			iface_states[idx] |= NET_STATE_IPV4_ADDR_SET;
		}

	}

	iface_states[idx] |= NET_STATE_CHANGED;
}

static void conn_mgr_init_cb(struct net_if *iface, void *user_data)
{
	ARG_UNUSED(user_data);

	conn_mgr_initial_state(iface);
}

static void conn_mgr(void)
{
	conn_mgr_init_events_handler();

	net_if_foreach(conn_mgr_init_cb, NULL);

	NET_DBG("Connection Manager started");

	while (true) {
		k_sem_take(&conn_mgr_lock, K_FOREVER);

		conn_mgr_act_on_changes();
	}
}

K_THREAD_DEFINE(conn_mgr_thread, CONFIG_NET_CONNECTION_MANAGER_STACK_SIZE,
		(k_thread_entry_t)conn_mgr, NULL, NULL, NULL,
		K_PRIO_COOP(2), 0, K_NO_WAIT);

void net_conn_mgr_resend_status(void)
{
	int idx;

	for (idx = 0; idx < ARRAY_SIZE(iface_states); idx++) {
		conn_mgr_notify_status(idx);
	}
}

static int conn_mgr_init(struct device *dev)
{
	ARG_UNUSED(dev);

	k_thread_start(conn_mgr_thread);

	return 0;
}

SYS_INIT(conn_mgr_init, APPLICATION, CONFIG_NET_CONNECTION_MANAGER_PRIORITY);
