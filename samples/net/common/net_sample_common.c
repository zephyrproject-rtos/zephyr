/*
 * Copyright (c) 2024 Nordic Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_samples_common, LOG_LEVEL_DBG);

#include <zephyr/sys/atomic.h>

#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/dns_resolve.h>

#if defined(CONFIG_NET_CONNECTION_MANAGER)
#if defined(CONFIG_NET_SAMPLE_COMMON_WAIT_DNS_SERVER_ADDITION)
#define L4_EVENT_MASK                                                                              \
	(NET_EVENT_DNS_SERVER_ADD | NET_EVENT_DNS_SERVERS_RECONFIGURED | NET_EVENT_L4_CONNECTED |  \
	 NET_EVENT_L4_DISCONNECTED)

static atomic_t l4_up;
#else
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#endif

static struct net_mgmt_event_callback l4_cb;
static K_SEM_DEFINE(network_connected, 0, 1);

static void l4_event_handler(struct net_mgmt_event_callback *cb, uint64_t event,
			     struct net_if *iface)
{
	switch (event) {
#if defined(CONFIG_NET_SAMPLE_COMMON_WAIT_DNS_SERVER_ADDITION)
	/* Only a nudge to look again. The waiter decides. */
	case NET_EVENT_DNS_SERVER_ADD:
	case NET_EVENT_DNS_SERVERS_RECONFIGURED:
		k_sem_give(&network_connected);
		break;
	case NET_EVENT_L4_CONNECTED:
		atomic_set(&l4_up, 1);
		k_sem_give(&network_connected);
		break;
	case NET_EVENT_L4_DISCONNECTED:
		atomic_set(&l4_up, 0);
		break;
#else
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network connectivity established and IP address assigned");
		k_sem_give(&network_connected);
		break;
	case NET_EVENT_L4_DISCONNECTED:
		break;
#endif
	default:
		break;
	}
}

void wait_for_network(void)
{
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);
#if defined(CONFIG_NET_SAMPLE_COMMON_WAIT_DNS_SERVER_ADDITION)
	/* The replay below is queued, so drop what the last call left. */
	atomic_set(&l4_up, 0);
#endif
	conn_mgr_mon_resend_status();

	LOG_INF("Waiting for network...");

#if defined(CONFIG_NET_SAMPLE_COMMON_WAIT_DNS_SERVER_ADDITION)
	/* A server added before the callback existed is not resent, so read
	 * the state rather than trust the events alone. The resolver goes
	 * first because it can block, which would age a link read taken
	 * before it.
	 */
	while (!dns_resolve_is_active(dns_resolve_get_default()) || !atomic_get(&l4_up)) {
		k_sem_take(&network_connected, K_FOREVER);
	}

	LOG_INF("Network connectivity established and IP address assigned");
#else
	k_sem_take(&network_connected, K_FOREVER);
#endif
}
#endif /* CONFIG_NET_CONNECTION_MANAGER */
