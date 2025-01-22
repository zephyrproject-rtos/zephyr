/*
 * Copyright (c) 2024 Nordic Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_samples_common, LOG_LEVEL_DBG);

#include <zephyr/net/conn_mgr_connectivity.h>

#if defined(CONFIG_NET_CONNECTION_MANAGER)
#if defined(CONFIG_NET_SAMPLE_COMMON_WAIT_DNS_SERVER_ADDITION)
#define L4_EVENT_MASK (NET_EVENT_DNS_SERVER_ADD | NET_EVENT_L4_DISCONNECTED)
#else
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#endif

static struct net_mgmt_event_callback l4_cb;
static K_SEM_DEFINE(network_connected, 0, 1);

static void l4_event_handler(struct net_mgmt_event_callback *cb, uint32_t event,
			     struct net_if *iface)
{
	switch (event) {
#if defined(CONFIG_NET_SAMPLE_COMMON_WAIT_DNS_SERVER_ADDITION)
	case NET_EVENT_DNS_SERVER_ADD:
#else
	case NET_EVENT_L4_CONNECTED:
#endif
		LOG_INF("Network connectivity established and IP address assigned");
		k_sem_give(&network_connected);
		break;
	case NET_EVENT_L4_DISCONNECTED:
		break;
	default:
		break;
	}
}

void wait_for_network(void)
{
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	LOG_INF("Waiting for network...");

	k_sem_take(&network_connected, K_FOREVER);
}
#endif /* CONFIG_NET_CONNECTION_MANAGER */
