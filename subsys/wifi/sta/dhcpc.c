/*
 * @file
 * @brief DHCP client handling
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_WIFIMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(wifimgr);

#include <net/net_mgmt.h>

#include "wifimgr.h"

static struct net_mgmt_event_callback mgmt_cb;

static void wifimgr_dhcp_handler(struct net_mgmt_event_callback *cb,
				 unsigned int mgmt_event, struct net_if *iface)
{
	int i;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD)
		return;

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		char buf[NET_IPV4_ADDR_LEN];
		char *ipaddr, *netmask, *gateway;

		if (iface->config.ip.ipv4->unicast[i].addr_type !=
		    NET_ADDR_DHCP)
			continue;

		ipaddr =
		    iface->config.ip.ipv4->unicast[i].address.in_addr.s4_addr;
		netmask = iface->config.ip.ipv4->netmask.s4_addr;
		gateway = iface->config.ip.ipv4->gw.s4_addr;
		wifi_drv_notify_ip(iface, ipaddr, sizeof(struct in_addr));

		wifimgr_info("IP address: %s\n",
			     net_addr_ntop(AF_INET, ipaddr, buf, sizeof(buf)));
		wifimgr_info("Lease time: %us\n",
			     iface->config.dhcpv4.lease_time);
		wifimgr_info("Subnet: %s\n",
			     net_addr_ntop(AF_INET, netmask, buf, sizeof(buf)));
		wifimgr_info("Router: %s\n",
			     net_addr_ntop(AF_INET, gateway, buf, sizeof(buf)));
	}
}

void wifimgr_dhcp_start(void *handle)
{
	struct net_if *iface = (struct net_if *)handle;

	wifimgr_info("start DHCP client\n");

	net_mgmt_init_event_callback(&mgmt_cb, wifimgr_dhcp_handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);

	net_dhcpv4_start(iface);
}

void wifimgr_dhcp_stop(void *handle)
{
	struct net_if *iface = (struct net_if *)handle;

	wifimgr_info("stop DHCP client\n");

	net_mgmt_del_event_callback(&mgmt_cb);

	net_dhcpv4_stop(iface);
}
