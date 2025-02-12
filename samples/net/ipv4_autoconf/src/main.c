/* Networking DHCPv4 client */

/*
 * Copyright (c) 2017 ARM Ltd.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ipv4_autoconf_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <errno.h>
#include <stdio.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>

static struct net_mgmt_event_callback mgmt_cb;

static void handler(struct net_mgmt_event_callback *cb,
		    uint32_t mgmt_event,
		    struct net_if *iface)
{
	int i = 0;
	struct net_if_config *cfg;

	cfg = net_if_get_config(iface);
	if (!cfg) {
		return;
	}

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		char buf[NET_IPV4_ADDR_LEN];

		if (cfg->ip.ipv4->unicast[i].ipv4.addr_type != NET_ADDR_AUTOCONF) {
			continue;
		}

		LOG_INF("Your address: %s",
			net_addr_ntop(AF_INET,
				    &cfg->ip.ipv4->unicast[i].ipv4.address.in_addr,
				    buf, sizeof(buf)));
		LOG_INF("Your netmask: %s",
			net_addr_ntop(AF_INET,
				    &cfg->ip.ipv4->unicast[i].netmask,
				    buf, sizeof(buf)));
	}
}

int main(void)
{
	LOG_INF("Run ipv4 autoconf client");

	net_mgmt_init_event_callback(&mgmt_cb, handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);
	return 0;
}
