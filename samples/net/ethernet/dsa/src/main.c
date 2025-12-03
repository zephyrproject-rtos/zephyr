/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa_sample, CONFIG_NET_DSA_LOG_LEVEL);

#include "dsa.h"

#if defined(CONFIG_NET_SAMPLE_DSA_LLDP)
#include "dsa_lldp.h"
#endif

struct ud g_user_data = {0};

static void dsa_iface_find_cb(struct net_if *iface, void *user_data)
{

	struct ud *ifaces = user_data;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if ((net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_CONDUIT_PORT) &&
	    (ifaces->conduit == NULL)) {
		ifaces->conduit = iface;

		/* Get user interfaces */
		for (int i = 0; i < ARRAY_SIZE(ifaces->lan); i++) {
#if defined(CONFIG_NET_DSA_DEPRECATED)
			struct net_if *user = dsa_get_slave_port(iface, i);
#else
			struct net_if *user = dsa_user_get_iface(iface, i);
#endif
			if (user == NULL) {
				continue;
			}
			LOG_INF("[%d] User interface %d found.", i, net_if_get_by_iface(user));

			ifaces->lan[i] = user;
		}
		return;
	}
}

#if defined(CONFIG_NET_MGMT_EVENT)
#define EVENT_MASK (NET_EVENT_IF_UP)
static struct net_mgmt_event_callback mgmt_cb;

static void event_handler(struct net_mgmt_event_callback *cb,
			  uint64_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(cb);

	if (mgmt_event == NET_EVENT_IF_UP) {
		LOG_INF("Port %d is up", net_if_get_by_iface(iface));

#if defined(CONFIG_NET_DHCPV4)
		net_dhcpv4_start(iface);
#endif

		return;
	}
}
#endif /* CONFIG_NET_MGMT_EVENT */

int main(void)
{
	/* Initialize interfaces - read them to user_data */
	net_if_foreach(dsa_iface_find_cb, &g_user_data);

#if defined(CONFIG_NET_MGMT_EVENT)
	net_mgmt_init_event_callback(&mgmt_cb,
				     event_handler, EVENT_MASK);
	net_mgmt_add_event_callback(&mgmt_cb);
#endif /* CONFIG_NET_MGMT_EVENT */

#if defined(CONFIG_NET_SAMPLE_DSA_LLDP)
	dsa_lldp(&g_user_data);
#endif
	LOG_INF("DSA ports init - OK");
	return 0;
}
