/* Networking DHCPv4 client */

/*
 * Copyright (c) 2017 ARM Ltd.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bmc_dhcp, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <errno.h>
#include <stdio.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>

#include "dhcp.h"
#include "config.h"

#define DHCP_OPTION_NTP (42)

static struct net_mgmt_event_callback mgmt_cb;

#if defined(CONFIG_NET_DHCPV4_OPTION_CALLBACKS)
static uint8_t ntp_server[4];
static struct net_dhcpv4_option_callback dhcp_cb;
#endif

static void start_dhcpv4_client(struct net_if *iface, void *user_data)
{
	ARG_UNUSED(user_data);

	LOG_INF("Start on %s[%d]", net_if_get_device(iface)->name,
		net_if_get_by_iface(iface));
	net_dhcpv4_start(iface);
}

static void restart_dhcpv4_client(struct net_if *iface, void *user_data)
{
	ARG_UNUSED(user_data);

	LOG_INF("Restart on %s[%d]", net_if_get_device(iface)->name,
		net_if_get_by_iface(iface));
	net_dhcpv4_restart(iface);
}

static void stop_dhcpv4_client(struct net_if *iface, void *user_data)
{
	ARG_UNUSED(user_data);

	LOG_INF("Stop on %s[%d]", net_if_get_device(iface)->name,
		net_if_get_by_iface(iface));
	net_dhcpv4_stop(iface);
}

static void handler(struct net_mgmt_event_callback *cb,
		    uint64_t mgmt_event,
		    struct net_if *iface)
{
	int i = 0;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		char buf[NET_IPV4_ADDR_LEN];

		if (iface->config.ip.ipv4->unicast[i].ipv4.addr_type !=
							NET_ADDR_DHCP) {
			continue;
		}

		LOG_INF("New lease on %s[%d]", net_if_get_device(iface)->name,
						net_if_get_by_iface(iface));
		LOG_INF("    Address: %s", net_addr_ntop(AF_INET,
				&iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr,
					buf, sizeof(buf)));
		LOG_DBG("    Subnet: %s", net_addr_ntop(AF_INET,
					&iface->config.ip.ipv4->unicast[i].netmask,
					buf, sizeof(buf)));
		LOG_DBG("    Router: %s", net_addr_ntop(AF_INET,
					&iface->config.ip.ipv4->gw,
					buf, sizeof(buf)));
		LOG_DBG("    Lease time: %u seconds",
					iface->config.dhcpv4.lease_time);
	}
}

#if defined(CONFIG_NET_DHCPV4_OPTION_CALLBACKS)
static void option_handler(struct net_dhcpv4_option_callback *cb,
			   size_t length,
			   enum net_dhcpv4_msg_type msg_type,
			   struct net_if *iface)
{
	char buf[NET_IPV4_ADDR_LEN];

	ARG_UNUSED(length);
	ARG_UNUSED(msg_type);
	ARG_UNUSED(iface);

	LOG_DBG("DHCP Option %d: %s", cb->option,
		net_addr_ntop(AF_INET, cb->data, buf, sizeof(buf)));
}
#endif

int dhcp4_init(void)
{
	net_mgmt_init_event_callback(&mgmt_cb, handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);

#if defined(CONFIG_NET_DHCPV4_OPTION_CALLBACKS)
	net_dhcpv4_init_option_callback(&dhcp_cb, option_handler,
					DHCP_OPTION_NTP, ntp_server,
					sizeof(ntp_server));

	net_dhcpv4_add_option_callback(&dhcp_cb);
#endif

	return 0;
}

int start_dhcp4(void)
{
	net_if_foreach(start_dhcpv4_client, NULL);

	return 0;
}

int restart_dhcp4(void)
{
	net_if_foreach(restart_dhcpv4_client, NULL);

	return 0;
}

int stop_dhcp4(void)
{
	net_if_foreach(stop_dhcpv4_client, NULL);

	return 0;
}
