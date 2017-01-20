/* Networking DHCPv4 client */

/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "dhcpv4"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <sections.h>
#include <errno.h>
#include <stdio.h>

#include <net/nbuf.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>

#include "net_private.h"

#define STACKSIZE 2000
char __noinit __stack thread_stack[STACKSIZE];

static struct net_mgmt_event_callback mgmt_cb;

static void handler(struct net_mgmt_event_callback *cb,
		    uint32_t mgmt_event,
		    struct net_if *iface)
{
	int i = 0;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (iface->ipv4.unicast[i].addr_type != NET_ADDR_DHCP) {
			continue;
		}

		NET_INFO("Your address: %s",
			 net_sprint_ipv4_addr(
				&iface->ipv4.unicast[i].address.in_addr));
		NET_INFO("Lease time: %u seconds", iface->dhcpv4.lease_time);
		NET_INFO("Subnet: %s",
			 net_sprint_ipv4_addr(&iface->ipv4.netmask));
		NET_INFO("Router: %s",
			 net_sprint_ipv4_addr(&iface->ipv4.gw));
	}
}

static void main_thread(void)
{
	struct net_if *iface;

	NET_INFO("Run dhcpv4 client");

	net_mgmt_init_event_callback(&mgmt_cb, handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);

	iface = net_if_get_default();

	net_dhcpv4_start(iface);
}

void main(void)
{
	NET_INFO("In main");

	k_thread_spawn(&thread_stack[0], STACKSIZE,
		       (k_thread_entry_t)main_thread,
		       NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}
