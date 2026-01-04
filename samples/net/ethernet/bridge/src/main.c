/*
 * SPDX-FileCopyrightText: Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_bridge_sample, CONFIG_NET_L2_ETHERNET_LOG_LEVEL);

#include <zephyr/net/ethernet_bridge.h>

struct ud {
	struct net_if *bridge;
	struct net_if *iface[CONFIG_NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT];
#if defined(CONFIG_NET_MGMT_EVENT) && defined(CONFIG_NET_DHCPV4)
	struct net_mgmt_event_callback mgmt_cb;
#endif
};

struct ud g_user_data = {0};

static void bridge_find_cb(struct eth_bridge_iface_context *br, void *user_data)
{
	struct ud *u = user_data;

	if (u->bridge == NULL) {
		u->bridge = br->iface;
		LOG_INF("Find bridge iface %d.", net_if_get_by_iface(br->iface));
	}
}

static void bridge_add_iface_cb(struct net_if *iface, void *user_data)
{
#if defined(CONFIG_NET_DSA) && !defined(CONFIG_NET_DSA_DEPRECATED)
	struct ethernet_context *eth_ctx;
#endif
	struct ud *u = user_data;
	int i;

	for (i = 0; i < CONFIG_NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT; i++) {
		if (u->iface[i] == NULL) {
			break;
		}
	}

	if (i == CONFIG_NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT) {
		return;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

#if defined(CONFIG_NET_DSA) && !defined(CONFIG_NET_DSA_DEPRECATED)
	eth_ctx = net_if_l2_data(iface);

	if (eth_ctx->dsa_port == DSA_USER_PORT || eth_ctx->dsa_port == NON_DSA_PORT) {
		u->iface[i] = iface;
	}
#else
	u->iface[i] = iface;
#endif
	eth_bridge_iface_add(u->bridge, iface);
	LOG_INF("Find iface %d. Add into bridge.", net_if_get_by_iface(iface));
}

#if defined(CONFIG_NET_MGMT_EVENT) && defined(CONFIG_NET_DHCPV4)
static void event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			  struct net_if *iface)
{
	struct ethernet_context *eth_ctx;

	ARG_UNUSED(cb);

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	eth_ctx = net_if_l2_data(iface);

	if (net_eth_iface_is_bridged(eth_ctx) && mgmt_event == NET_EVENT_IF_UP) {
		net_dhcpv4_restart(net_eth_get_bridge(eth_ctx));
		return;
	}
}
#endif

int main(void)
{
	struct ud *u = &g_user_data;

	net_eth_bridge_foreach(bridge_find_cb, u);
	net_if_foreach(bridge_add_iface_cb, u);
	net_if_up(u->bridge);
#if defined(CONFIG_NET_MGMT_EVENT) && defined(CONFIG_NET_DHCPV4)
	net_mgmt_init_event_callback(&u->mgmt_cb, event_handler, NET_EVENT_IF_UP);
	net_mgmt_add_event_callback(&u->mgmt_cb);
#endif
	return 0;
}
