/** @file
 * @brief Network shell module
 *
 * Provide some networking shell commands that can be useful to applications.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_shell, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/pm/device.h>
#include <zephyr/random/random.h>
#include <stdlib.h>
#include <stdio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/ppp.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/sys/printk.h>
#if defined(CONFIG_NET_L2_ETHERNET) && defined(CONFIG_NET_L2_ETHERNET_MGMT)
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#endif /* CONFIG_NET_L2_ETHERNET */

#include <zephyr/net/icmp.h>

#include "route.h"
#include "icmpv6.h"
#include "icmpv4.h"
#include "connection.h"

#if defined(CONFIG_NET_TCP)
#include "tcp_internal.h"
#include <zephyr/sys/slist.h>
#endif

#include "ipv6.h"

#if defined(CONFIG_NET_ARP)
#include "ethernet/arp.h"
#endif

#if defined(CONFIG_NET_L2_ETHERNET)
#include <zephyr/net/ethernet.h>
#endif

#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
#include <zephyr/net/ethernet_mgmt.h>
#endif

#if defined(CONFIG_NET_L2_VIRTUAL)
#include <zephyr/net/virtual.h>
#endif

#if defined(CONFIG_NET_L2_VIRTUAL_MGMT)
#include <zephyr/net/virtual_mgmt.h>
#endif

#include <zephyr/net/capture.h>

#if defined(CONFIG_NET_GPTP)
#include <zephyr/net/gptp.h>
#include "ethernet/gptp/gptp_messages.h"
#include "ethernet/gptp/gptp_md.h"
#include "ethernet/gptp/gptp_state.h"
#include "ethernet/gptp/gptp_data_set.h"
#include "ethernet/gptp/gptp_private.h"
#endif

#if defined(CONFIG_NET_L2_PPP)
#include <zephyr/net/ppp.h>
#include "ppp/ppp_internal.h"
#endif

#include "net_shell.h"
#include "net_stats.h"

#include <zephyr/sys/fdtable.h>
#include "websocket/websocket_internal.h"

#define PR(fmt, ...)						\
	shell_fprintf(sh, SHELL_NORMAL, fmt, ##__VA_ARGS__)

#define PR_SHELL(sh, fmt, ...)				\
	shell_fprintf(sh, SHELL_NORMAL, fmt, ##__VA_ARGS__)

#define PR_ERROR(fmt, ...)					\
	shell_fprintf(sh, SHELL_ERROR, fmt, ##__VA_ARGS__)

#define PR_INFO(fmt, ...)					\
	shell_fprintf(sh, SHELL_INFO, fmt, ##__VA_ARGS__)

#define PR_WARNING(fmt, ...)					\
	shell_fprintf(sh, SHELL_WARNING, fmt, ##__VA_ARGS__)

#include "net_private.h"

struct net_shell_user_data {
	const struct shell *sh;
	void *user_data;
};

static inline const char *addrtype2str(enum net_addr_type addr_type)
{
	switch (addr_type) {
	case NET_ADDR_ANY:
		return "<unknown type>";
	case NET_ADDR_AUTOCONF:
		return "autoconf";
	case NET_ADDR_DHCP:
		return "DHCP";
	case NET_ADDR_MANUAL:
		return "manual";
	case NET_ADDR_OVERRIDABLE:
		return "overridable";
	}

	return "<invalid type>";
}

static inline const char *addrstate2str(enum net_addr_state addr_state)
{
	switch (addr_state) {
	case NET_ADDR_ANY_STATE:
		return "<unknown state>";
	case NET_ADDR_TENTATIVE:
		return "tentative";
	case NET_ADDR_PREFERRED:
		return "preferred";
	case NET_ADDR_DEPRECATED:
		return "deprecated";
	}

	return "<invalid state>";
}

static const char *iface2str(struct net_if *iface, const char **extra)
{
#ifdef CONFIG_NET_L2_IEEE802154
	if (net_if_l2(iface) == &NET_L2_GET_NAME(IEEE802154)) {
		if (extra) {
			*extra = "=============";
		}

		return "IEEE 802.15.4";
	}
#endif

#ifdef CONFIG_NET_L2_ETHERNET
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		if (extra) {
			*extra = "========";
		}

		return "Ethernet";
	}
#endif

#ifdef CONFIG_NET_L2_VIRTUAL
	if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		if (extra) {
			*extra = "=======";
		}

		return "Virtual";
	}
#endif

#ifdef CONFIG_NET_L2_PPP
	if (net_if_l2(iface) == &NET_L2_GET_NAME(PPP)) {
		if (extra) {
			*extra = "===";
		}

		return "PPP";
	}
#endif

#ifdef CONFIG_NET_L2_DUMMY
	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		if (extra) {
			*extra = "=====";
		}

		return "Dummy";
	}
#endif

#ifdef CONFIG_NET_L2_OPENTHREAD
	if (net_if_l2(iface) == &NET_L2_GET_NAME(OPENTHREAD)) {
		if (extra) {
			*extra = "==========";
		}

		return "OpenThread";
	}
#endif

#ifdef CONFIG_NET_L2_BT
	if (net_if_l2(iface) == &NET_L2_GET_NAME(BLUETOOTH)) {
		if (extra) {
			*extra = "=========";
		}

		return "Bluetooth";
	}
#endif

#ifdef CONFIG_NET_OFFLOAD
	if (net_if_is_ip_offloaded(iface)) {
		if (extra) {
			*extra = "==========";
		}

		return "IP Offload";
	}
#endif

#ifdef CONFIG_NET_L2_CANBUS_RAW
	if (net_if_l2(iface) == &NET_L2_GET_NAME(CANBUS_RAW)) {
		if (extra) {
			*extra = "==========";
		}

		return "CANBUS_RAW";
	}
#endif

	if (extra) {
		*extra = "==============";
	}

	return "<unknown type>";
}

#if defined(CONFIG_NET_L2_ETHERNET) && defined(CONFIG_NET_NATIVE)
struct ethernet_capabilities {
	enum ethernet_hw_caps capability;
	const char * const description;
};

#define EC(cap, desc) { .capability = cap, .description = desc }

static struct ethernet_capabilities eth_hw_caps[] = {
	EC(ETHERNET_HW_TX_CHKSUM_OFFLOAD, "TX checksum offload"),
	EC(ETHERNET_HW_RX_CHKSUM_OFFLOAD, "RX checksum offload"),
	EC(ETHERNET_HW_VLAN,              "Virtual LAN"),
	EC(ETHERNET_HW_VLAN_TAG_STRIP,    "VLAN Tag stripping"),
	EC(ETHERNET_AUTO_NEGOTIATION_SET, "Auto negotiation"),
	EC(ETHERNET_LINK_10BASE_T,        "10 Mbits"),
	EC(ETHERNET_LINK_100BASE_T,       "100 Mbits"),
	EC(ETHERNET_LINK_1000BASE_T,      "1 Gbits"),
	EC(ETHERNET_DUPLEX_SET,           "Half/full duplex"),
	EC(ETHERNET_PTP,                  "IEEE 802.1AS gPTP clock"),
	EC(ETHERNET_QAV,                  "IEEE 802.1Qav (credit shaping)"),
	EC(ETHERNET_QBV,                  "IEEE 802.1Qbv (scheduled traffic)"),
	EC(ETHERNET_QBU,                  "IEEE 802.1Qbu (frame preemption)"),
	EC(ETHERNET_TXTIME,               "TXTIME"),
	EC(ETHERNET_PROMISC_MODE,         "Promiscuous mode"),
	EC(ETHERNET_PRIORITY_QUEUES,      "Priority queues"),
	EC(ETHERNET_HW_FILTERING,         "MAC address filtering"),
	EC(ETHERNET_DSA_SLAVE_PORT,       "DSA slave port"),
	EC(ETHERNET_DSA_MASTER_PORT,      "DSA master port"),
};

static void print_supported_ethernet_capabilities(
	const struct shell *sh, struct net_if *iface)
{
	enum ethernet_hw_caps caps = net_eth_get_hw_capabilities(iface);
	int i;

	for (i = 0; i < ARRAY_SIZE(eth_hw_caps); i++) {
		if (caps & eth_hw_caps[i].capability) {
			PR("\t%s\n", eth_hw_caps[i].description);
		}
	}
}
#endif /* CONFIG_NET_L2_ETHERNET */

#if defined(CONFIG_NET_NATIVE)
static const char *iface_flags2str(struct net_if *iface)
{
	static char str[sizeof("POINTOPOINT") + sizeof("PROMISC") +
			sizeof("NO_AUTO_START") + sizeof("SUSPENDED") +
			sizeof("MCAST_FORWARD") + sizeof("IPv4") +
			sizeof("IPv6") + sizeof("NO_ND") + sizeof("NO_MLD")];
	int pos = 0;

	if (net_if_flag_is_set(iface, NET_IF_POINTOPOINT)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"POINTOPOINT,");
	}

	if (net_if_flag_is_set(iface, NET_IF_PROMISC)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"PROMISC,");
	}

	if (net_if_flag_is_set(iface, NET_IF_NO_AUTO_START)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"NO_AUTO_START,");
	} else {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"AUTO_START,");
	}

	if (net_if_flag_is_set(iface, NET_IF_FORWARD_MULTICASTS)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"MCAST_FORWARD,");
	}

	if (net_if_flag_is_set(iface, NET_IF_IPV4)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"IPv4,");
	}

	if (net_if_flag_is_set(iface, NET_IF_IPV6)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"IPv6,");
	}

	if (net_if_flag_is_set(iface, NET_IF_IPV6_NO_ND)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"NO_ND,");
	}

	if (net_if_flag_is_set(iface, NET_IF_IPV6_NO_MLD)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"NO_MLD,");
	}

	/* get rid of last ',' character */
	str[pos - 1] = '\0';

	return str;
}
#endif

static void iface_cb(struct net_if *iface, void *user_data)
{
#if defined(CONFIG_NET_NATIVE)
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;

#if defined(CONFIG_NET_IPV6)
	struct net_if_ipv6_prefix *prefix;
	struct net_if_router *router;
	struct net_if_ipv6 *ipv6;
#endif
#if defined(CONFIG_NET_IPV4)
	struct net_if_ipv4 *ipv4;
#endif
#if defined(CONFIG_NET_VLAN)
	struct ethernet_context *eth_ctx;
#endif
#if defined(CONFIG_NET_IP)
	struct net_if_addr *unicast;
	struct net_if_mcast_addr *mcast;
#endif
#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
	struct ethernet_req_params params;
	int ret;
#endif
	const char *extra;
#if defined(CONFIG_NET_IP)
	int i, count;
#endif

	if (data->user_data && data->user_data != iface) {
		return;
	}

#if defined(CONFIG_NET_INTERFACE_NAME)
	char ifname[CONFIG_NET_INTERFACE_NAME_LEN + 1] = { 0 };
	int ret_name;

	ret_name = net_if_get_name(iface, ifname, sizeof(ifname) - 1);
	if (ret_name < 1 || ifname[0] == '\0') {
		snprintk(ifname, sizeof(ifname), "?");
	}

	PR("\nInterface %s (%p) (%s) [%d]\n", ifname, iface, iface2str(iface, &extra),
	   net_if_get_by_iface(iface));
#else
	PR("\nInterface %p (%s) [%d]\n", iface, iface2str(iface, &extra),
	   net_if_get_by_iface(iface));
#endif
	PR("===========================%s\n", extra);

	if (!net_if_is_up(iface)) {
		PR_INFO("Interface is down.\n");

		/* Show detailed information only when user asks information
		 * about one specific network interface.
		 */
		if (data->user_data == NULL) {
			return;
		}
	}

#ifdef CONFIG_NET_POWER_MANAGEMENT
	if (net_if_is_suspended(iface)) {
		PR_INFO("Interface is suspended, thus not able to tx/rx.\n");
	}
#endif

#if defined(CONFIG_NET_L2_VIRTUAL)
	if (!sys_slist_is_empty(&iface->config.virtual_interfaces)) {
		struct virtual_interface_context *ctx, *tmp;

		PR("Virtual interfaces attached to this : ");
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(
					&iface->config.virtual_interfaces,
					ctx, tmp, node) {
			if (ctx->virtual_iface == iface) {
				continue;
			}

			PR("%d ", net_if_get_by_iface(ctx->virtual_iface));
		}

		PR("\n");
	}

	if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		struct net_if *orig_iface;
		char *name, buf[CONFIG_NET_L2_VIRTUAL_MAX_NAME_LEN];

		name = net_virtual_get_name(iface, buf, sizeof(buf));
		if (!(name && name[0])) {
			name = "<unknown>";
		}

		PR("Name      : %s\n", name);

		orig_iface = net_virtual_get_iface(iface);
		if (orig_iface == NULL) {
			PR("No attached network interface.\n");
		} else {
			PR("Attached  : %d (%s / %p)\n",
			   net_if_get_by_iface(orig_iface),
			   iface2str(orig_iface, NULL),
			   orig_iface);
		}
	}
#endif /* CONFIG_NET_L2_VIRTUAL */

	net_if_lock(iface);
	if (net_if_get_link_addr(iface) &&
	    net_if_get_link_addr(iface)->addr) {
		PR("Link addr : %s\n",
		   net_sprint_ll_addr(net_if_get_link_addr(iface)->addr,
				      net_if_get_link_addr(iface)->len));
	}
	net_if_unlock(iface);

	PR("MTU       : %d\n", net_if_get_mtu(iface));
	PR("Flags     : %s\n", iface_flags2str(iface));

#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		count = 0;
		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_PRIORITY_QUEUES_NUM,
				iface, &params,
				sizeof(struct ethernet_req_params));

		if (!ret && params.priority_queues_num) {
			count = params.priority_queues_num;
			PR("Priority queues:\n");
			for (i = 0; i < count; ++i) {
				params.qav_param.queue_id = i;
				params.qav_param.type =
					ETHERNET_QAV_PARAM_TYPE_STATUS;
				ret = net_mgmt(
					NET_REQUEST_ETHERNET_GET_QAV_PARAM,
					iface, &params,
					sizeof(struct ethernet_req_params));

				PR("\t%d: Qav ", i);
				if (ret) {
					PR("not supported\n");
				} else {
					PR("%s\n",
						params.qav_param.enabled ?
						"enabled" :
						"disabled");
				}
			}
		}
	}
#endif

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	PR("Promiscuous mode : %s\n",
	   net_if_is_promisc(iface) ? "enabled" : "disabled");
#endif

#if defined(CONFIG_NET_VLAN)
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		eth_ctx = net_if_l2_data(iface);

		if (eth_ctx->vlan_enabled) {
			for (i = 0; i < CONFIG_NET_VLAN_COUNT; i++) {
				if (eth_ctx->vlan[i].iface != iface ||
				    eth_ctx->vlan[i].tag ==
							NET_VLAN_TAG_UNSPEC) {
					continue;
				}

				PR("VLAN tag  : %d (0x%x)\n",
				   eth_ctx->vlan[i].tag,
				   eth_ctx->vlan[i].tag);
			}
		} else {
			PR("VLAN not enabled\n");
		}
	}
#endif

#ifdef CONFIG_NET_L2_ETHERNET
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		PR("Ethernet capabilities supported:\n");
		print_supported_ethernet_capabilities(sh, iface);
	}
#endif /* CONFIG_NET_L2_ETHERNET */

#if defined(CONFIG_NET_IPV6)
	count = 0;

	if (!net_if_flag_is_set(iface, NET_IF_IPV6)) {
		PR("%s not %s for this interface.\n", "IPv6", "enabled");
		ipv6 = NULL;
		goto skip_ipv6;
	}

	ipv6 = iface->config.ip.ipv6;

	PR("IPv6 unicast addresses (max %d):\n", NET_IF_MAX_IPV6_ADDR);
	for (i = 0; ipv6 && i < NET_IF_MAX_IPV6_ADDR; i++) {
		unicast = &ipv6->unicast[i];

		if (!unicast->is_used) {
			continue;
		}

		PR("\t%s %s %s%s%s\n",
		   net_sprint_ipv6_addr(&unicast->address.in6_addr),
		   addrtype2str(unicast->addr_type),
		   addrstate2str(unicast->addr_state),
		   unicast->is_infinite ? " infinite" : "",
		   unicast->is_mesh_local ? " meshlocal" : "");
		count++;
	}

	if (count == 0) {
		PR("\t<none>\n");
	}

	count = 0;

	PR("IPv6 multicast addresses (max %d):\n", NET_IF_MAX_IPV6_MADDR);
	for (i = 0; ipv6 && i < NET_IF_MAX_IPV6_MADDR; i++) {
		mcast = &ipv6->mcast[i];

		if (!mcast->is_used) {
			continue;
		}

		PR("\t%s\n", net_sprint_ipv6_addr(&mcast->address.in6_addr));

		count++;
	}

	if (count == 0) {
		PR("\t<none>\n");
	}

	count = 0;

	PR("IPv6 prefixes (max %d):\n", NET_IF_MAX_IPV6_PREFIX);
	for (i = 0; ipv6 && i < NET_IF_MAX_IPV6_PREFIX; i++) {
		prefix = &ipv6->prefix[i];

		if (!prefix->is_used) {
			continue;
		}

		PR("\t%s/%d%s\n",
		   net_sprint_ipv6_addr(&prefix->prefix),
		   prefix->len, prefix->is_infinite ? " infinite" : "");

		count++;
	}

	if (count == 0) {
		PR("\t<none>\n");
	}

	router = net_if_ipv6_router_find_default(iface, NULL);
	if (router) {
		PR("IPv6 default router :\n");
		PR("\t%s%s\n",
		   net_sprint_ipv6_addr(&router->address.in6_addr),
		   router->is_infinite ? " infinite" : "");
	}

skip_ipv6:

	if (ipv6) {
		PR("IPv6 hop limit           : %d\n",
		   ipv6->hop_limit);
		PR("IPv6 base reachable time : %d\n",
		   ipv6->base_reachable_time);
		PR("IPv6 reachable time      : %d\n",
		   ipv6->reachable_time);
		PR("IPv6 retransmit timer    : %d\n",
		   ipv6->retrans_timer);
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	/* No need to print IPv4 information for interface that does not
	 * support that protocol.
	 */
	if (
#if defined(CONFIG_NET_L2_IEEE802154)
		(net_if_l2(iface) == &NET_L2_GET_NAME(IEEE802154)) ||
#endif
#if defined(CONFIG_NET_L2_BT)
		 (net_if_l2(iface) == &NET_L2_GET_NAME(BLUETOOTH)) ||
#endif
		 0) {
		PR_WARNING("%s not %s for this interface.\n", "IPv4",
			   "supported");
		return;
	}

	count = 0;

	if (!net_if_flag_is_set(iface, NET_IF_IPV4)) {
		PR("%s not %s for this interface.\n", "IPv4", "enabled");
		ipv4 = NULL;
		goto skip_ipv4;
	}

	ipv4 = iface->config.ip.ipv4;

	PR("IPv4 unicast addresses (max %d):\n", NET_IF_MAX_IPV4_ADDR);
	for (i = 0; ipv4 && i < NET_IF_MAX_IPV4_ADDR; i++) {
		unicast = &ipv4->unicast[i];

		if (!unicast->is_used) {
			continue;
		}

		PR("\t%s %s %s%s\n",
		   net_sprint_ipv4_addr(&unicast->address.in_addr),
		   addrtype2str(unicast->addr_type),
		   addrstate2str(unicast->addr_state),
		   unicast->is_infinite ? " infinite" : "");

		count++;
	}

	if (count == 0) {
		PR("\t<none>\n");
	}

	count = 0;

	PR("IPv4 multicast addresses (max %d):\n", NET_IF_MAX_IPV4_MADDR);
	for (i = 0; ipv4 && i < NET_IF_MAX_IPV4_MADDR; i++) {
		mcast = &ipv4->mcast[i];

		if (!mcast->is_used) {
			continue;
		}

		PR("\t%s\n", net_sprint_ipv4_addr(&mcast->address.in_addr));

		count++;
	}

	if (count == 0) {
		PR("\t<none>\n");
	}

skip_ipv4:

	if (ipv4) {
		PR("IPv4 gateway : %s\n",
		   net_sprint_ipv4_addr(&ipv4->gw));
		PR("IPv4 netmask : %s\n",
		   net_sprint_ipv4_addr(&ipv4->netmask));
	}
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_DHCPV4)
	PR("DHCPv4 lease time : %u\n",
	   iface->config.dhcpv4.lease_time);
	PR("DHCPv4 renew time : %u\n",
	   iface->config.dhcpv4.renewal_time);
	PR("DHCPv4 server     : %s\n",
	   net_sprint_ipv4_addr(&iface->config.dhcpv4.server_id));
	PR("DHCPv4 requested  : %s\n",
	   net_sprint_ipv4_addr(&iface->config.dhcpv4.requested_ip));
	PR("DHCPv4 state      : %s\n",
	   net_dhcpv4_state_name(iface->config.dhcpv4.state));
	PR("DHCPv4 attempts   : %d\n",
	   iface->config.dhcpv4.attempts);
#endif /* CONFIG_NET_DHCPV4 */

#if defined(CONFIG_NET_DHCPV6)
	PR("DHCPv6 address requested : %s\n",
	   iface->config.dhcpv6.params.request_addr ?
	   net_sprint_ipv6_addr(&iface->config.dhcpv6.addr) : "none");
	PR("DHCPv6 prefix requested  : %s\n",
	   iface->config.dhcpv6.params.request_prefix ?
	   net_sprint_ipv6_addr(&iface->config.dhcpv6.prefix) : "none");
	PR("DHCPv6 state             : %s\n",
	   net_dhcpv6_state_name(iface->config.dhcpv6.state));
	PR("DHCPv6 attempts          : %d\n",
	   iface->config.dhcpv6.retransmissions + 1);
#endif /* CONFIG_NET_DHCPV6 */

#else
	ARG_UNUSED(iface);
	ARG_UNUSED(user_data);

#endif /* CONFIG_NET_NATIVE */
}

#if defined(CONFIG_NET_ROUTE) && defined(CONFIG_NET_NATIVE)
static void route_cb(struct net_route_entry *entry, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct net_if *iface = data->user_data;
	struct net_route_nexthop *nexthop_route;
	int count;
	uint32_t now = k_uptime_get_32();

	if (entry->iface != iface) {
		return;
	}

	PR("IPv6 prefix : %s/%d\n", net_sprint_ipv6_addr(&entry->addr),
	   entry->prefix_len);

	count = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&entry->nexthop, nexthop_route, node) {
		struct net_linkaddr_storage *lladdr;
		char remaining_str[sizeof("01234567890 sec")];
		uint32_t remaining;

		if (!nexthop_route->nbr) {
			continue;
		}

		PR("\tneighbor : %p\t", nexthop_route->nbr);

		if (nexthop_route->nbr->idx == NET_NBR_LLADDR_UNKNOWN) {
			PR("addr : <unknown>\t");
		} else {
			lladdr = net_nbr_get_lladdr(nexthop_route->nbr->idx);

			PR("addr : %s\t", net_sprint_ll_addr(lladdr->addr,
							     lladdr->len));
		}

		if (entry->is_infinite) {
			snprintk(remaining_str, sizeof(remaining_str) - 1,
				 "infinite");
		} else {
			remaining = net_timeout_remaining(&entry->lifetime, now);
			snprintk(remaining_str, sizeof(remaining_str) - 1,
				 "%u sec", remaining);
		}

		PR("lifetime : %s\n", remaining_str);

		count++;
	}

	if (count == 0) {
		PR("\t<none>\n");
	}
}

static void iface_per_route_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	const char *extra;

	PR("\nIPv6 routes for interface %d (%p) (%s)\n",
	   net_if_get_by_iface(iface), iface,
	   iface2str(iface, &extra));
	PR("=========================================%s\n", extra);

	data->user_data = iface;

	net_route_foreach(route_cb, data);
}
#endif /* CONFIG_NET_ROUTE */

#if defined(CONFIG_NET_ROUTE_MCAST) && defined(CONFIG_NET_NATIVE)
static void route_mcast_cb(struct net_route_entry_mcast *entry,
			   void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct net_if *iface = data->user_data;
	const char *extra;

	if (entry->iface != iface) {
		return;
	}

	PR("IPv6 multicast route %p for interface %d (%p) (%s)\n", entry,
	   net_if_get_by_iface(iface), iface, iface2str(iface, &extra));
	PR("==========================================================="
	   "%s\n", extra);

	PR("IPv6 group     : %s\n", net_sprint_ipv6_addr(&entry->group));
	PR("IPv6 group len : %d\n", entry->prefix_len);
	PR("Lifetime       : %u\n", entry->lifetime);
}

static void iface_per_mcast_route_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;

	data->user_data = iface;

	net_route_mcast_foreach(route_mcast_cb, NULL, data);
}
#endif /* CONFIG_NET_ROUTE_MCAST */

#if defined(CONFIG_NET_STATISTICS)

#if NET_TC_COUNT > 1
static const char *priority2str(enum net_priority priority)
{
	switch (priority) {
	case NET_PRIORITY_BK:
		return "BK"; /* Background */
	case NET_PRIORITY_BE:
		return "BE"; /* Best effort */
	case NET_PRIORITY_EE:
		return "EE"; /* Excellent effort */
	case NET_PRIORITY_CA:
		return "CA"; /* Critical applications */
	case NET_PRIORITY_VI:
		return "VI"; /* Video, < 100 ms latency and jitter */
	case NET_PRIORITY_VO:
		return "VO"; /* Voice, < 10 ms latency and jitter  */
	case NET_PRIORITY_IC:
		return "IC"; /* Internetwork control */
	case NET_PRIORITY_NC:
		return "NC"; /* Network control */
	}

	return "??";
}
#endif

#if defined(CONFIG_NET_STATISTICS_ETHERNET) && \
					defined(CONFIG_NET_STATISTICS_USER_API)
static void print_eth_stats(struct net_if *iface, struct net_stats_eth *data,
			    const struct shell *sh)
{
	PR("Statistics for Ethernet interface %p [%d]\n", iface,
	       net_if_get_by_iface(iface));

	PR("Bytes received   : %u\n", data->bytes.received);
	PR("Bytes sent       : %u\n", data->bytes.sent);
	PR("Packets received : %u\n", data->pkts.rx);
	PR("Packets sent     : %u\n", data->pkts.tx);
	PR("Bcast received   : %u\n", data->broadcast.rx);
	PR("Bcast sent       : %u\n", data->broadcast.tx);
	PR("Mcast received   : %u\n", data->multicast.rx);
	PR("Mcast sent       : %u\n", data->multicast.tx);

	PR("Send errors      : %u\n", data->errors.tx);
	PR("Receive errors   : %u\n", data->errors.rx);
	PR("Collisions       : %u\n", data->collisions);
	PR("Send Drops       : %u\n", data->tx_dropped);
	PR("Send timeouts    : %u\n", data->tx_timeout_count);
	PR("Send restarts    : %u\n", data->tx_restart_queue);
	PR("Unknown protocol : %u\n", data->unknown_protocol);

#if defined(CONFIG_NET_STATISTICS_ETHERNET_VENDOR)
	if (data->vendor) {
		PR("Vendor specific statistics for Ethernet "
		   "interface %p [%d]:\n",
			iface, net_if_get_by_iface(iface));
		size_t i = 0;

		do {
			PR("%s : %u\n", data->vendor[i].key,
			   data->vendor[i].value);
			i++;
		} while (data->vendor[i].key);
	}
#endif /* CONFIG_NET_STATISTICS_ETHERNET_VENDOR */
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET && CONFIG_NET_STATISTICS_USER_API */

#if defined(CONFIG_NET_STATISTICS_PPP) && \
					defined(CONFIG_NET_STATISTICS_USER_API)
static void print_ppp_stats(struct net_if *iface, struct net_stats_ppp *data,
			    const struct shell *sh)
{
	PR("Frames recv    %u\n", data->pkts.rx);
	PR("Frames sent    %u\n", data->pkts.tx);
	PR("Frames dropped %u\n", data->drop);
	PR("Bad FCS        %u\n", data->chkerr);
}
#endif /* CONFIG_NET_STATISTICS_PPP && CONFIG_NET_STATISTICS_USER_API */

#if !defined(CONFIG_NET_NATIVE)
#define GET_STAT(a, b) 0
#endif

#if defined(CONFIG_NET_PKT_TXTIME_STATS_DETAIL) || \
	defined(CONFIG_NET_PKT_RXTIME_STATS_DETAIL)
#if (NET_TC_TX_COUNT > 1) || (NET_TC_RX_COUNT > 1)
static char *get_net_pkt_tc_stats_detail(struct net_if *iface, int i,
					  bool is_tx)
{
	static char extra_stats[sizeof("\t[0=xxxx us]") +
				sizeof("->xxxx") *
				NET_PKT_DETAIL_STATS_COUNT];
	int j, total = 0, pos = 0;

	pos += snprintk(extra_stats, sizeof(extra_stats), "\t[0");

	for (j = 0; j < NET_PKT_DETAIL_STATS_COUNT; j++) {
		net_stats_t count = 0;
		uint32_t avg;

		if (is_tx) {
#if defined(CONFIG_NET_PKT_TXTIME_STATS_DETAIL) && (NET_TC_TX_COUNT > 1)
			count = GET_STAT(iface,
					 tc.sent[i].tx_time_detail[j].count);
#endif
		} else {
#if defined(CONFIG_NET_PKT_RXTIME_STATS_DETAIL) && (NET_TC_RX_COUNT > 1)
			count = GET_STAT(iface,
					 tc.recv[i].rx_time_detail[j].count);
#endif
		}

		if (count == 0) {
			break;
		}

		if (is_tx) {
#if defined(CONFIG_NET_PKT_TXTIME_STATS_DETAIL) && (NET_TC_TX_COUNT > 1)
			avg = (uint32_t)(GET_STAT(iface,
					   tc.sent[i].tx_time_detail[j].sum) /
				      (uint64_t)count);
#endif
		} else {
#if defined(CONFIG_NET_PKT_RXTIME_STATS_DETAIL) && (NET_TC_RX_COUNT > 1)
			avg = (uint32_t)(GET_STAT(iface,
					   tc.recv[i].rx_time_detail[j].sum) /
				      (uint64_t)count);
#endif
		}

		if (avg == 0) {
			continue;
		}

		total += avg;

		pos += snprintk(extra_stats + pos, sizeof(extra_stats) - pos,
				"->%u", avg);
	}

	if (total == 0U) {
		return "\0";
	}

	pos += snprintk(extra_stats + pos, sizeof(extra_stats) - pos,
				"=%u us]", total);

	return extra_stats;
}
#endif /* (NET_TC_TX_COUNT > 1) || (NET_TC_RX_COUNT > 1) */

#if (NET_TC_TX_COUNT <= 1) || (NET_TC_RX_COUNT <= 1)
static char *get_net_pkt_stats_detail(struct net_if *iface, bool is_tx)
{
	static char extra_stats[sizeof("\t[0=xxxx us]") + sizeof("->xxxx") *
				NET_PKT_DETAIL_STATS_COUNT];
	int j, total = 0, pos = 0;

	pos += snprintk(extra_stats, sizeof(extra_stats), "\t[0");

	for (j = 0; j < NET_PKT_DETAIL_STATS_COUNT; j++) {
		net_stats_t count;
		uint32_t avg;

		if (is_tx) {
#if defined(CONFIG_NET_PKT_TXTIME_STATS_DETAIL)
			count = GET_STAT(iface, tx_time_detail[j].count);
#endif
		} else {
#if defined(CONFIG_NET_PKT_RXTIME_STATS_DETAIL)
			count = GET_STAT(iface, rx_time_detail[j].count);
#endif
		}

		if (count == 0) {
			break;
		}

		if (is_tx) {
#if defined(CONFIG_NET_PKT_TXTIME_STATS_DETAIL)
			avg = (uint32_t)(GET_STAT(iface,
					       tx_time_detail[j].sum) /
				      (uint64_t)count);
#endif
		} else {
#if defined(CONFIG_NET_PKT_RXTIME_STATS_DETAIL)
			avg = (uint32_t)(GET_STAT(iface,
					       rx_time_detail[j].sum) /
				      (uint64_t)count);
#endif
		}

		if (avg == 0) {
			continue;
		}

		total += avg;

		pos += snprintk(extra_stats + pos,
				sizeof(extra_stats) - pos,
				"->%u", avg);
	}

	if (total == 0U) {
		return "\0";
	}

	pos += snprintk(extra_stats + pos, sizeof(extra_stats) - pos,
			"=%u us]", total);

	return extra_stats;
}
#endif /* (NET_TC_TX_COUNT == 1) || (NET_TC_RX_COUNT == 1) */

#else /* CONFIG_NET_PKT_TXTIME_STATS_DETAIL ||
	 CONFIG_NET_PKT_RXTIME_STATS_DETAIL */

#if defined(CONFIG_NET_PKT_TXTIME_STATS) || \
	defined(CONFIG_NET_PKT_RXTIME_STATS)

#if (NET_TC_TX_COUNT > 1) || (NET_TC_RX_COUNT > 1)
static char *get_net_pkt_tc_stats_detail(struct net_if *iface, int i,
					 bool is_tx)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(i);
	ARG_UNUSED(is_tx);

	return "\0";
}
#endif

#if (NET_TC_TX_COUNT == 1) || (NET_TC_RX_COUNT == 1)
static char *get_net_pkt_stats_detail(struct net_if *iface, bool is_tx)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(is_tx);

	return "\0";
}
#endif
#endif /* CONFIG_NET_PKT_TXTIME_STATS) || CONFIG_NET_PKT_RXTIME_STATS */
#endif /* CONFIG_NET_PKT_TXTIME_STATS_DETAIL ||
	  CONFIG_NET_PKT_RXTIME_STATS_DETAIL */

static void print_tc_tx_stats(const struct shell *sh, struct net_if *iface)
{
#if NET_TC_TX_COUNT > 1
	int i;

	PR("TX traffic class statistics:\n");

#if defined(CONFIG_NET_PKT_TXTIME_STATS)
	PR("TC  Priority\tSent pkts\tbytes\ttime\n");

	for (i = 0; i < NET_TC_TX_COUNT; i++) {
		net_stats_t count = GET_STAT(iface,
					     tc.sent[i].tx_time.count);
		if (count == 0) {
			PR("[%d] %s (%d)\t%d\t\t%d\t-\n", i,
			   priority2str(GET_STAT(iface, tc.sent[i].priority)),
			   GET_STAT(iface, tc.sent[i].priority),
			   GET_STAT(iface, tc.sent[i].pkts),
			   GET_STAT(iface, tc.sent[i].bytes));
		} else {
			PR("[%d] %s (%d)\t%d\t\t%d\t%u us%s\n", i,
			   priority2str(GET_STAT(iface, tc.sent[i].priority)),
			   GET_STAT(iface, tc.sent[i].priority),
			   GET_STAT(iface, tc.sent[i].pkts),
			   GET_STAT(iface, tc.sent[i].bytes),
			   (uint32_t)(GET_STAT(iface,
					    tc.sent[i].tx_time.sum) /
				   (uint64_t)count),
			   get_net_pkt_tc_stats_detail(iface, i, true));
		}
	}
#else
	PR("TC  Priority\tSent pkts\tbytes\n");

	for (i = 0; i < NET_TC_TX_COUNT; i++) {
		PR("[%d] %s (%d)\t%d\t\t%d\n", i,
		   priority2str(GET_STAT(iface, tc.sent[i].priority)),
		   GET_STAT(iface, tc.sent[i].priority),
		   GET_STAT(iface, tc.sent[i].pkts),
		   GET_STAT(iface, tc.sent[i].bytes));
	}
#endif /* CONFIG_NET_PKT_TXTIME_STATS */
#else
	ARG_UNUSED(sh);

#if defined(CONFIG_NET_PKT_TXTIME_STATS)
	net_stats_t count = GET_STAT(iface, tx_time.count);

	if (count != 0) {
		PR("Avg %s net_pkt (%u) time %lu us%s\n", "TX", count,
		   (uint32_t)(GET_STAT(iface, tx_time.sum) / (uint64_t)count),
		   get_net_pkt_stats_detail(iface, true));
	}
#else
	ARG_UNUSED(iface);
#endif /* CONFIG_NET_PKT_TXTIME_STATS */
#endif /* NET_TC_TX_COUNT > 1 */
}

static void print_tc_rx_stats(const struct shell *sh, struct net_if *iface)
{
#if NET_TC_RX_COUNT > 1
	int i;

	PR("RX traffic class statistics:\n");

#if defined(CONFIG_NET_PKT_RXTIME_STATS)
	PR("TC  Priority\tRecv pkts\tbytes\ttime\n");

	for (i = 0; i < NET_TC_RX_COUNT; i++) {
		net_stats_t count = GET_STAT(iface,
					     tc.recv[i].rx_time.count);
		if (count == 0) {
			PR("[%d] %s (%d)\t%d\t\t%d\t-\n", i,
			   priority2str(GET_STAT(iface, tc.recv[i].priority)),
			   GET_STAT(iface, tc.recv[i].priority),
			   GET_STAT(iface, tc.recv[i].pkts),
			   GET_STAT(iface, tc.recv[i].bytes));
		} else {
			PR("[%d] %s (%d)\t%d\t\t%d\t%u us%s\n", i,
			   priority2str(GET_STAT(iface, tc.recv[i].priority)),
			   GET_STAT(iface, tc.recv[i].priority),
			   GET_STAT(iface, tc.recv[i].pkts),
			   GET_STAT(iface, tc.recv[i].bytes),
			   (uint32_t)(GET_STAT(iface,
					    tc.recv[i].rx_time.sum) /
				   (uint64_t)count),
			   get_net_pkt_tc_stats_detail(iface, i, false));
		}
	}
#else
	PR("TC  Priority\tRecv pkts\tbytes\n");

	for (i = 0; i < NET_TC_RX_COUNT; i++) {
		PR("[%d] %s (%d)\t%d\t\t%d\n", i,
		   priority2str(GET_STAT(iface, tc.recv[i].priority)),
		   GET_STAT(iface, tc.recv[i].priority),
		   GET_STAT(iface, tc.recv[i].pkts),
		   GET_STAT(iface, tc.recv[i].bytes));
	}
#endif /* CONFIG_NET_PKT_RXTIME_STATS */
#else
	ARG_UNUSED(sh);

#if defined(CONFIG_NET_PKT_RXTIME_STATS)
	net_stats_t count = GET_STAT(iface, rx_time.count);

	if (count != 0) {
		PR("Avg %s net_pkt (%u) time %lu us%s\n", "RX", count,
		   (uint32_t)(GET_STAT(iface, rx_time.sum) / (uint64_t)count),
		   get_net_pkt_stats_detail(iface, false));
	}
#else
	ARG_UNUSED(iface);
#endif /* CONFIG_NET_PKT_RXTIME_STATS */

#endif /* NET_TC_RX_COUNT > 1 */
}

static void print_net_pm_stats(const struct shell *sh, struct net_if *iface)
{
#if defined(CONFIG_NET_STATISTICS_POWER_MANAGEMENT)
	PR("PM suspend stats:\n");
	PR("\tLast time     : %u ms\n",
	   GET_STAT(iface, pm.last_suspend_time));
	PR("\tAverage time  : %u ms\n",
	   (uint32_t)(GET_STAT(iface, pm.overall_suspend_time) /
		   GET_STAT(iface, pm.suspend_count)));
	PR("\tTotal time    : %" PRIu64 " ms\n",
	   GET_STAT(iface, pm.overall_suspend_time));
	PR("\tHow many times: %u\n",
	   GET_STAT(iface, pm.suspend_count));
#else
	ARG_UNUSED(sh);
	ARG_UNUSED(iface);
#endif
}

static void net_shell_print_statistics(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;

	if (iface) {
		const char *extra;

		PR("\nInterface %p (%s) [%d]\n", iface,
		   iface2str(iface, &extra), net_if_get_by_iface(iface));
		PR("===========================%s\n", extra);
	} else {
		PR("\nGlobal statistics\n");
		PR("=================\n");
	}

#if defined(CONFIG_NET_STATISTICS_IPV6) && defined(CONFIG_NET_NATIVE_IPV6)
	PR("IPv6 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
	   GET_STAT(iface, ipv6.recv),
	   GET_STAT(iface, ipv6.sent),
	   GET_STAT(iface, ipv6.drop),
	   GET_STAT(iface, ipv6.forwarded));
#if defined(CONFIG_NET_STATISTICS_IPV6_ND)
	PR("IPv6 ND recv   %d\tsent\t%d\tdrop\t%d\n",
	   GET_STAT(iface, ipv6_nd.recv),
	   GET_STAT(iface, ipv6_nd.sent),
	   GET_STAT(iface, ipv6_nd.drop));
#endif /* CONFIG_NET_STATISTICS_IPV6_ND */
#if defined(CONFIG_NET_STATISTICS_MLD)
	PR("IPv6 MLD recv  %d\tsent\t%d\tdrop\t%d\n",
	   GET_STAT(iface, ipv6_mld.recv),
	   GET_STAT(iface, ipv6_mld.sent),
	   GET_STAT(iface, ipv6_mld.drop));
#endif /* CONFIG_NET_STATISTICS_MLD */
#endif /* CONFIG_NET_STATISTICS_IPV6 */

#if defined(CONFIG_NET_STATISTICS_IPV4) && defined(CONFIG_NET_NATIVE_IPV4)
	PR("IPv4 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
	   GET_STAT(iface, ipv4.recv),
	   GET_STAT(iface, ipv4.sent),
	   GET_STAT(iface, ipv4.drop),
	   GET_STAT(iface, ipv4.forwarded));
#endif /* CONFIG_NET_STATISTICS_IPV4 */

	PR("IP vhlerr      %d\thblener\t%d\tlblener\t%d\n",
	   GET_STAT(iface, ip_errors.vhlerr),
	   GET_STAT(iface, ip_errors.hblenerr),
	   GET_STAT(iface, ip_errors.lblenerr));
	PR("IP fragerr     %d\tchkerr\t%d\tprotoer\t%d\n",
	   GET_STAT(iface, ip_errors.fragerr),
	   GET_STAT(iface, ip_errors.chkerr),
	   GET_STAT(iface, ip_errors.protoerr));

#if defined(CONFIG_NET_STATISTICS_ICMP) && defined(CONFIG_NET_NATIVE_IPV4)
	PR("ICMP recv      %d\tsent\t%d\tdrop\t%d\n",
	   GET_STAT(iface, icmp.recv),
	   GET_STAT(iface, icmp.sent),
	   GET_STAT(iface, icmp.drop));
	PR("ICMP typeer    %d\tchkerr\t%d\n",
	   GET_STAT(iface, icmp.typeerr),
	   GET_STAT(iface, icmp.chkerr));
#endif
#if defined(CONFIG_NET_STATISTICS_IGMP)
	PR("IGMP recv      %d\tsent\t%d\tdrop\t%d\n",
	   GET_STAT(iface, ipv4_igmp.recv),
	   GET_STAT(iface, ipv4_igmp.sent),
	   GET_STAT(iface, ipv4_igmp.drop));
#endif /* CONFIG_NET_STATISTICS_IGMP */
#if defined(CONFIG_NET_STATISTICS_UDP) && defined(CONFIG_NET_NATIVE_UDP)
	PR("UDP recv       %d\tsent\t%d\tdrop\t%d\n",
	   GET_STAT(iface, udp.recv),
	   GET_STAT(iface, udp.sent),
	   GET_STAT(iface, udp.drop));
	PR("UDP chkerr     %d\n",
	   GET_STAT(iface, udp.chkerr));
#endif

#if defined(CONFIG_NET_STATISTICS_TCP) && defined(CONFIG_NET_NATIVE_TCP)
	PR("TCP bytes recv %u\tsent\t%d\tresent\t%d\n",
	   GET_STAT(iface, tcp.bytes.received),
	   GET_STAT(iface, tcp.bytes.sent),
	   GET_STAT(iface, tcp.resent));
	PR("TCP seg recv   %d\tsent\t%d\tdrop\t%d\n",
	   GET_STAT(iface, tcp.recv),
	   GET_STAT(iface, tcp.sent),
	   GET_STAT(iface, tcp.seg_drop));
	PR("TCP seg resent %d\tchkerr\t%d\tackerr\t%d\n",
	   GET_STAT(iface, tcp.rexmit),
	   GET_STAT(iface, tcp.chkerr),
	   GET_STAT(iface, tcp.ackerr));
	PR("TCP seg rsterr %d\trst\t%d\n",
	   GET_STAT(iface, tcp.rsterr),
	   GET_STAT(iface, tcp.rst));
	PR("TCP conn drop  %d\tconnrst\t%d\n",
	   GET_STAT(iface, tcp.conndrop),
	   GET_STAT(iface, tcp.connrst));
	PR("TCP pkt drop   %d\n", GET_STAT(iface, tcp.drop));
#endif

	PR("Bytes received %u\n", GET_STAT(iface, bytes.received));
	PR("Bytes sent     %u\n", GET_STAT(iface, bytes.sent));
	PR("Processing err %d\n", GET_STAT(iface, processing_error));

	print_tc_tx_stats(sh, iface);
	print_tc_rx_stats(sh, iface);

#if defined(CONFIG_NET_STATISTICS_ETHERNET) && \
					defined(CONFIG_NET_STATISTICS_USER_API)
	if (iface && net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		struct net_stats_eth eth_data;
		int ret;

		ret = net_mgmt(NET_REQUEST_STATS_GET_ETHERNET, iface,
			       &eth_data, sizeof(eth_data));
		if (!ret) {
			print_eth_stats(iface, &eth_data, sh);
		}
	}
#endif /* CONFIG_NET_STATISTICS_ETHERNET && CONFIG_NET_STATISTICS_USER_API */

#if defined(CONFIG_NET_STATISTICS_PPP) && \
					defined(CONFIG_NET_STATISTICS_USER_API)
	if (iface && net_if_l2(iface) == &NET_L2_GET_NAME(PPP)) {
		struct net_stats_ppp ppp_data;
		int ret;

		ret = net_mgmt(NET_REQUEST_STATS_GET_PPP, iface,
			       &ppp_data, sizeof(ppp_data));
		if (!ret) {
			print_ppp_stats(iface, &ppp_data, sh);
		}
	}
#endif /* CONFIG_NET_STATISTICS_PPP && CONFIG_NET_STATISTICS_USER_API */

	print_net_pm_stats(sh, iface);
}
#endif /* CONFIG_NET_STATISTICS */

#if defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_NATIVE)
static void get_addresses(struct net_context *context,
			  char addr_local[], int local_len,
			  char addr_remote[], int remote_len)
{
#if defined(CONFIG_NET_IPV6)
	if (context->local.family == AF_INET6) {
		snprintk(addr_local, local_len, "[%s]:%u",
			 net_sprint_ipv6_addr(
				 net_sin6_ptr(&context->local)->sin6_addr),
			 ntohs(net_sin6_ptr(&context->local)->sin6_port));
		snprintk(addr_remote, remote_len, "[%s]:%u",
			 net_sprint_ipv6_addr(
				 &net_sin6(&context->remote)->sin6_addr),
			 ntohs(net_sin6(&context->remote)->sin6_port));
	} else
#endif
#if defined(CONFIG_NET_IPV4)
	if (context->local.family == AF_INET) {
		snprintk(addr_local, local_len, "%s:%d",
			 net_sprint_ipv4_addr(
				 net_sin_ptr(&context->local)->sin_addr),
			 ntohs(net_sin_ptr(&context->local)->sin_port));
		snprintk(addr_remote, remote_len, "%s:%d",
			 net_sprint_ipv4_addr(
				 &net_sin(&context->remote)->sin_addr),
			 ntohs(net_sin(&context->remote)->sin_port));
	} else
#endif
	if (context->local.family == AF_UNSPEC) {
		snprintk(addr_local, local_len, "AF_UNSPEC");
	} else if (context->local.family == AF_PACKET) {
		snprintk(addr_local, local_len, "AF_PACKET");
	} else if (context->local.family == AF_CAN) {
		snprintk(addr_local, local_len, "AF_CAN");
	} else {
		snprintk(addr_local, local_len, "AF_UNK(%d)",
			 context->local.family);
	}
}

static void context_cb(struct net_context *context, void *user_data)
{
#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
#define ADDR_LEN NET_IPV6_ADDR_LEN
#elif defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
#define ADDR_LEN NET_IPV4_ADDR_LEN
#else
#define ADDR_LEN NET_IPV6_ADDR_LEN
#endif
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	/* +7 for []:port */
	char addr_local[ADDR_LEN + 7];
	char addr_remote[ADDR_LEN + 7] = "";

	get_addresses(context, addr_local, sizeof(addr_local),
		      addr_remote, sizeof(addr_remote));

	PR("[%2d] %p\t%d      %c%c%c   %16s\t%16s\n",
	   (*count) + 1, context,
	   net_if_get_by_iface(net_context_get_iface(context)),
	   net_context_get_family(context) == AF_INET6 ? '6' :
	   (net_context_get_family(context) == AF_INET ? '4' : ' '),
	   net_context_get_type(context) == SOCK_DGRAM ? 'D' :
	   (net_context_get_type(context) == SOCK_STREAM ? 'S' :
	    (net_context_get_type(context) == SOCK_RAW ? 'R' : ' ')),
	   net_context_get_proto(context) == IPPROTO_UDP ? 'U' :
	   (net_context_get_proto(context) == IPPROTO_TCP ? 'T' : ' '),
	   addr_local, addr_remote);

	(*count)++;
}
#endif /* CONFIG_NET_OFFLOAD || CONFIG_NET_NATIVE */

#if CONFIG_NET_CONN_LOG_LEVEL >= LOG_LEVEL_DBG
static void conn_handler_cb(struct net_conn *conn, void *user_data)
{
#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
#define ADDR_LEN NET_IPV6_ADDR_LEN
#elif defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
#define ADDR_LEN NET_IPV4_ADDR_LEN
#else
#define ADDR_LEN NET_IPV6_ADDR_LEN
#endif
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	/* +7 for []:port */
	char addr_local[ADDR_LEN + 7];
	char addr_remote[ADDR_LEN + 7] = "";

#if defined(CONFIG_NET_IPV6)
	if (conn->local_addr.sa_family == AF_INET6) {
		snprintk(addr_local, sizeof(addr_local), "[%s]:%u",
			 net_sprint_ipv6_addr(
				 &net_sin6(&conn->local_addr)->sin6_addr),
			 ntohs(net_sin6(&conn->local_addr)->sin6_port));
		snprintk(addr_remote, sizeof(addr_remote), "[%s]:%u",
			 net_sprint_ipv6_addr(
				 &net_sin6(&conn->remote_addr)->sin6_addr),
			 ntohs(net_sin6(&conn->remote_addr)->sin6_port));
	} else
#endif
#if defined(CONFIG_NET_IPV4)
	if (conn->local_addr.sa_family == AF_INET) {
		snprintk(addr_local, sizeof(addr_local), "%s:%d",
			 net_sprint_ipv4_addr(
				 &net_sin(&conn->local_addr)->sin_addr),
			 ntohs(net_sin(&conn->local_addr)->sin_port));
		snprintk(addr_remote, sizeof(addr_remote), "%s:%d",
			 net_sprint_ipv4_addr(
				 &net_sin(&conn->remote_addr)->sin_addr),
			 ntohs(net_sin(&conn->remote_addr)->sin_port));
	} else
#endif
	if (conn->local_addr.sa_family == AF_UNSPEC) {
		snprintk(addr_local, sizeof(addr_local), "AF_UNSPEC");
	} else {
		snprintk(addr_local, sizeof(addr_local), "AF_UNK(%d)",
			 conn->local_addr.sa_family);
	}

	PR("[%2d] %p %p\t%s\t%16s\t%16s\n",
	   (*count) + 1, conn, conn->cb,
	   net_proto2str(conn->local_addr.sa_family, conn->proto),
	   addr_local, addr_remote);

	(*count)++;
}
#endif /* CONFIG_NET_CONN_LOG_LEVEL >= LOG_LEVEL_DBG */

#if CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG
struct tcp_detail_info {
	int printed_send_queue_header;
	int printed_details;
	int count;
};
#endif

#if defined(CONFIG_NET_TCP) && \
	(defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_NATIVE))
static void tcp_cb(struct tcp *conn, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	uint16_t recv_mss = net_tcp_get_supported_mss(conn);

	PR("%p %p   %5u    %5u %10u %10u %5u   %s\n",
	   conn, conn->context,
	   ntohs(net_sin6_ptr(&conn->context->local)->sin6_port),
	   ntohs(net_sin6(&conn->context->remote)->sin6_port),
	   conn->seq, conn->ack, recv_mss,
	   net_tcp_state_str(net_tcp_get_state(conn)));

	(*count)++;
}

#if CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG
static void tcp_sent_list_cb(struct tcp *conn, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct tcp_detail_info *details = data->user_data;
	struct net_pkt *pkt;
	sys_snode_t *node;

	if (conn->state != TCP_LISTEN) {
		if (!details->printed_details) {
			PR("\nTCP        Ref  Recv_win Send_win Pending "
			   "Unacked Flags Queue\n");
			details->printed_details = true;
		}

		PR("%p   %ld    %u\t %u\t  %zd\t  %d\t  %d/%d/%d %s\n",
		   conn, atomic_get(&conn->ref_count), conn->recv_win,
		   conn->send_win, conn->send_data_total, conn->unacked_len,
		   conn->in_retransmission, conn->in_connect, conn->in_close,
		   sys_slist_is_empty(&conn->send_queue) ? "empty" : "data");

		details->count++;
	}

	if (sys_slist_is_empty(&conn->send_queue)) {
		return;
	}

	if (!details->printed_send_queue_header) {
		PR("\nTCP packets waiting ACK:\n");
		PR("TCP             net_pkt[ref/totlen]->net_buf[ref/len]..."
		   "\n");
	}

	PR("%p      ", conn);

	node = sys_slist_peek_head(&conn->send_queue);
	if (node) {
		pkt = CONTAINER_OF(node, struct net_pkt, next);
		if (pkt) {
			struct net_buf *frag = pkt->frags;

			if (!details->printed_send_queue_header) {
				PR("%p[%ld/%zd]", pkt,
				   atomic_get(&pkt->atomic_ref),
				   net_pkt_get_len(pkt));
				details->printed_send_queue_header = true;
			} else {
				PR("                %p[%ld/%zd]",
				   pkt, atomic_get(&pkt->atomic_ref),
				   net_pkt_get_len(pkt));
			}

			if (frag) {
				PR("->");
			}

			while (frag) {
				PR("%p[%d/%d]", frag, frag->ref, frag->len);

				frag = frag->frags;
				if (frag) {
					PR("->");
				}
			}

			PR("\n");
		}
	}

	details->printed_send_queue_header = true;
}
#endif /* CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG */
#endif /* TCP */

#if defined(CONFIG_NET_IPV6_FRAGMENT)
static void ipv6_frag_cb(struct net_ipv6_reassembly *reass,
			 void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char src[ADDR_LEN];
	int i;

	if (!*count) {
		PR("\nIPv6 reassembly Id         Remain "
		   "Src             \tDst\n");
	}

	snprintk(src, ADDR_LEN, "%s", net_sprint_ipv6_addr(&reass->src));

	PR("%p      0x%08x  %5d %16s\t%16s\n", reass, reass->id,
	   k_ticks_to_ms_ceil32(k_work_delayable_remaining_get(&reass->timer)),
	   src, net_sprint_ipv6_addr(&reass->dst));

	for (i = 0; i < CONFIG_NET_IPV6_FRAGMENT_MAX_PKT; i++) {
		if (reass->pkt[i]) {
			struct net_buf *frag = reass->pkt[i]->frags;

			PR("[%d] pkt %p->", i, reass->pkt[i]);

			while (frag) {
				PR("%p", frag);

				frag = frag->frags;
				if (frag) {
					PR("->");
				}
			}

			PR("\n");
		}
	}

	(*count)++;
}
#endif /* CONFIG_NET_IPV6_FRAGMENT */

#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC)
static void allocs_cb(struct net_pkt *pkt,
		      struct net_buf *buf,
		      const char *func_alloc,
		      int line_alloc,
		      const char *func_free,
		      int line_free,
		      bool in_use,
		      void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	const char *str;

	if (in_use) {
		str = "used";
	} else {
		if (func_alloc) {
			str = "free";
		} else {
			str = "avail";
		}
	}

	if (buf) {
		goto buf;
	}

	if (func_alloc) {
		if (in_use) {
			PR("%p/%ld\t%5s\t%5s\t%s():%d\n",
			   pkt, atomic_get(&pkt->atomic_ref), str,
			   net_pkt_slab2str(pkt->slab),
			   func_alloc, line_alloc);
		} else {
			PR("%p\t%5s\t%5s\t%s():%d -> %s():%d\n",
			   pkt, str, net_pkt_slab2str(pkt->slab),
			   func_alloc, line_alloc, func_free,
			   line_free);
		}
	}

	return;
buf:
	if (func_alloc) {
		struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

		if (in_use) {
			PR("%p/%d\t%5s\t%5s\t%s():%d\n",
			   buf, buf->ref,
			   str, net_pkt_pool2str(pool), func_alloc,
			   line_alloc);
		} else {
			PR("%p\t%5s\t%5s\t%s():%d -> %s():%d\n",
			   buf, str, net_pkt_pool2str(pool),
			   func_alloc, line_alloc, func_free,
			   line_free);
		}
	}
}
#endif /* CONFIG_NET_DEBUG_NET_PKT_ALLOC */

/* Put the actual shell commands after this */

static int cmd_net_allocs(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC)
	struct net_shell_user_data user_data;
#endif

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC)
	user_data.sh = sh;

	PR("Network memory allocations\n\n");
	PR("memory\t\tStatus\tPool\tFunction alloc -> freed\n");
	net_pkt_allocs_foreach(allocs_cb, &user_data);
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_DEBUG_NET_PKT_ALLOC", "net_pkt allocation");
#endif /* CONFIG_NET_DEBUG_NET_PKT_ALLOC */

	return 0;
}

#if defined(CONFIG_NET_ARP) && defined(CONFIG_NET_NATIVE)
static void arp_cb(struct arp_entry *entry, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;

	if (*count == 0) {
		PR("     Interface  Link              Address\n");
	}

	PR("[%2d] %d          %s %s\n", *count,
	   net_if_get_by_iface(entry->iface),
	   net_sprint_ll_addr(entry->eth.addr, sizeof(struct net_eth_addr)),
	   net_sprint_ipv4_addr(&entry->ip));

	(*count)++;
}
#endif /* CONFIG_NET_ARP */

#if !defined(CONFIG_NET_ARP)
static void print_arp_error(const struct shell *sh)
{
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_NATIVE, CONFIG_NET_ARP, CONFIG_NET_IPV4 and"
		" CONFIG_NET_L2_ETHERNET", "ARP");
}
#endif

static int cmd_net_arp(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_ARP)
	struct net_shell_user_data user_data;
	int arg = 1;
#endif

	ARG_UNUSED(argc);

#if defined(CONFIG_NET_ARP)
	if (!argv[arg]) {
		/* ARP cache content */
		int count = 0;

		user_data.sh = sh;
		user_data.user_data = &count;

		if (net_arp_foreach(arp_cb, &user_data) == 0) {
			PR("ARP cache is empty.\n");
		}
	}
#else
	print_arp_error(sh);
#endif

	return 0;
}

static int cmd_net_arp_flush(const struct shell *sh, size_t argc,
			     char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_ARP)
	PR("Flushing ARP cache.\n");
	net_arp_clear_cache(NULL);
#else
	print_arp_error(sh);
#endif

	return 0;
}

#if defined(CONFIG_NET_CAPTURE)
static const struct device *capture_dev;

static void get_address_str(const struct sockaddr *addr,
			    char *str, int str_len)
{
	if (IS_ENABLED(CONFIG_NET_IPV6) && addr->sa_family == AF_INET6) {
		snprintk(str, str_len, "[%s]:%u",
			 net_sprint_ipv6_addr(&net_sin6(addr)->sin6_addr),
			 ntohs(net_sin6(addr)->sin6_port));

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && addr->sa_family == AF_INET) {
		snprintk(str, str_len, "%s:%d",
			 net_sprint_ipv4_addr(&net_sin(addr)->sin_addr),
			 ntohs(net_sin(addr)->sin_port));

	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) &&
		   addr->sa_family == AF_PACKET) {
		snprintk(str, str_len, "AF_PACKET");
	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_CAN) &&
		   addr->sa_family == AF_CAN) {
		snprintk(str, str_len, "AF_CAN");
	} else if (addr->sa_family == AF_UNSPEC) {
		snprintk(str, str_len, "AF_UNSPEC");
	} else {
		snprintk(str, str_len, "AF_UNK(%d)", addr->sa_family);
	}
}

static void capture_cb(struct net_capture_info *info, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char addr_local[ADDR_LEN + 7];
	char addr_peer[ADDR_LEN + 7];

	if (*count == 0) {
		PR("      \t\tCapture  Tunnel\n");
		PR("Device\t\tiface    iface   Local\t\t\tPeer\n");
	}

	get_address_str(info->local, addr_local, sizeof(addr_local));
	get_address_str(info->peer, addr_peer, sizeof(addr_peer));

	PR("%s\t%c        %d      %s\t%s\n", info->capture_dev->name,
	   info->is_enabled ?
	   (net_if_get_by_iface(info->capture_iface) + '0') : '-',
	   net_if_get_by_iface(info->tunnel_iface),
	   addr_local, addr_peer);

	(*count)++;
}
#endif

static int cmd_net_capture(const struct shell *sh, size_t argc,
			   char *argv[])
{
#if defined(CONFIG_NET_CAPTURE)
	bool ret;

	if (capture_dev == NULL) {
		PR_INFO("Network packet capture %s\n", "not configured");
	} else {
		struct net_shell_user_data user_data;
		int count = 0;

		ret = net_capture_is_enabled(capture_dev);
		PR_INFO("Network packet capture %s\n",
			ret ? "enabled" : "disabled");

		user_data.sh = sh;
		user_data.user_data = &count;

		net_capture_foreach(capture_cb, &user_data);
	}
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_CAPTURE", "network packet capture");
#endif
	return 0;
}

static int cmd_net_capture_setup(const struct shell *sh, size_t argc,
				 char *argv[])
{
#if defined(CONFIG_NET_CAPTURE)
	int ret, arg = 1;
	const char *remote, *local, *peer;

	remote = argv[arg++];
	if (!remote) {
		PR_WARNING("Remote IP address not specified.\n");
		return -ENOEXEC;
	}

	local = argv[arg++];
	if (!local) {
		PR_WARNING("Local IP address not specified.\n");
		return -ENOEXEC;
	}

	peer = argv[arg];
	if (!peer) {
		PR_WARNING("Peer IP address not specified.\n");
		return -ENOEXEC;
	}

	if (capture_dev != NULL) {
		PR_INFO("Capture already setup, cleaning up settings.\n");
		net_capture_cleanup(capture_dev);
		capture_dev = NULL;
	}

	ret = net_capture_setup(remote, local, peer, &capture_dev);
	if (ret < 0) {
		PR_WARNING("Capture cannot be setup (%d)\n", ret);
		return -ENOEXEC;
	}

	PR_INFO("Capture setup done, next enable it by "
		"\"net capture enable <idx>\"\n");
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_CAPTURE", "network packet capture");
#endif

	return 0;
}

static int cmd_net_capture_cleanup(const struct shell *sh, size_t argc,
				   char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_CAPTURE)
	int ret;

	if (capture_dev == NULL) {
		return 0;
	}

	ret = net_capture_cleanup(capture_dev);
	if (ret < 0) {
		PR_WARNING("Capture %s failed (%d)\n", "cleanup", ret);
		return -ENOEXEC;
	}

	capture_dev = NULL;
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_CAPTURE", "network packet capture");
#endif

	return 0;
}

static int cmd_net_capture_enable(const struct shell *sh, size_t argc,
				  char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_CAPTURE)
	int ret, arg = 1, if_index;
	struct net_if *iface;

	if (capture_dev == NULL) {
		return 0;
	}

	if (argv[arg] == NULL) {
		PR_WARNING("Interface index is missing. Please give interface "
			   "what you want to monitor\n");
		return -ENOEXEC;
	}

	if_index = atoi(argv[arg++]);
	if (if_index == 0) {
		PR_WARNING("Interface index %d is invalid.\n", if_index);
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(if_index);
	if (iface == NULL) {
		PR_WARNING("No such interface with index %d\n", if_index);
		return -ENOEXEC;
	}

	ret = net_capture_enable(capture_dev, iface);
	if (ret < 0) {
		PR_WARNING("Capture %s failed (%d)\n", "enable", ret);
		return -ENOEXEC;
	}
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_CAPTURE", "network packet capture");
#endif

	return 0;
}

static int cmd_net_capture_disable(const struct shell *sh, size_t argc,
				   char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_CAPTURE)
	int ret;

	if (capture_dev == NULL) {
		return 0;
	}

	ret = net_capture_disable(capture_dev);
	if (ret < 0) {
		PR_WARNING("Capture %s failed (%d)\n", "disable", ret);
		return -ENOEXEC;
	}
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_CAPTURE", "network packet capture");
#endif

	return 0;
}

static int cmd_net_conn(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_NATIVE)
	struct net_shell_user_data user_data;
	int count = 0;

	PR("     Context   \tIface  Flags            Local             Remote\n");

	user_data.sh = sh;
	user_data.user_data = &count;

	net_context_foreach(context_cb, &user_data);

	if (count == 0) {
		PR("No connections\n");
	}

#if CONFIG_NET_CONN_LOG_LEVEL >= LOG_LEVEL_DBG
	PR("\n     Handler    Callback  \tProto\tLocal           \tRemote\n");

	count = 0;

	net_conn_foreach(conn_handler_cb, &user_data);

	if (count == 0) {
		PR("No connection handlers found.\n");
	}
#endif

#if defined(CONFIG_NET_TCP)
	PR("\nTCP        Context   Src port Dst port   "
	   "Send-Seq   Send-Ack  MSS    State\n");

	count = 0;

	net_tcp_foreach(tcp_cb, &user_data);

	if (count == 0) {
		PR("No TCP connections\n");
	} else {
#if CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG
		/* Print information about pending packets */
		struct tcp_detail_info details;

		count = 0;

		if (IS_ENABLED(CONFIG_NET_TCP)) {
			memset(&details, 0, sizeof(details));
			user_data.user_data = &details;
		}

		net_tcp_foreach(tcp_sent_list_cb, &user_data);

		if (IS_ENABLED(CONFIG_NET_TCP)) {
			if (details.count == 0) {
				PR("No active connections.\n");
			}
		}
#endif /* CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG */
	}

#if CONFIG_NET_TCP_LOG_LEVEL < LOG_LEVEL_DBG
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_TCP_LOG_LEVEL_DBG", "TCP debugging");
#endif /* CONFIG_NET_TCP_LOG_LEVEL < LOG_LEVEL_DBG */

#endif

#if defined(CONFIG_NET_IPV6_FRAGMENT)
	count = 0;

	net_ipv6_frag_foreach(ipv6_frag_cb, &user_data);

	/* Do not print anything if no fragments are pending atm */
#endif

#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_OFFLOAD or CONFIG_NET_NATIVE",
		"connection information");

#endif /* CONFIG_NET_OFFLOAD || CONFIG_NET_NATIVE */

	return 0;
}

#if defined(CONFIG_DNS_RESOLVER)
static void dns_result_cb(enum dns_resolve_status status,
			  struct dns_addrinfo *info,
			  void *user_data)
{
	const struct shell *sh = user_data;

	if (status == DNS_EAI_CANCELED) {
		PR_WARNING("dns: Timeout while resolving name.\n");
		return;
	}

	if (status == DNS_EAI_INPROGRESS && info) {
		char addr[NET_IPV6_ADDR_LEN];

		if (info->ai_family == AF_INET) {
			net_addr_ntop(AF_INET,
				      &net_sin(&info->ai_addr)->sin_addr,
				      addr, NET_IPV4_ADDR_LEN);
		} else if (info->ai_family == AF_INET6) {
			net_addr_ntop(AF_INET6,
				      &net_sin6(&info->ai_addr)->sin6_addr,
				      addr, NET_IPV6_ADDR_LEN);
		} else {
			strncpy(addr, "Invalid protocol family",
				sizeof(addr));
			/* strncpy() doesn't guarantee NUL byte at the end. */
			addr[sizeof(addr) - 1] = 0;
		}

		PR("dns: %s\n", addr);
		return;
	}

	if (status == DNS_EAI_ALLDONE) {
		PR("dns: All results received\n");
		return;
	}

	if (status == DNS_EAI_FAIL) {
		PR_WARNING("dns: No such name found.\n");
		return;
	}

	PR_WARNING("dns: Unhandled status %d received\n", status);
}

static void print_dns_info(const struct shell *sh,
			   struct dns_resolve_context *ctx)
{
	int i;

	PR("DNS servers:\n");

	for (i = 0; i < CONFIG_DNS_RESOLVER_MAX_SERVERS +
		     DNS_MAX_MCAST_SERVERS; i++) {
		if (ctx->servers[i].dns_server.sa_family == AF_INET) {
			PR("\t%s:%u\n",
			   net_sprint_ipv4_addr(
				   &net_sin(&ctx->servers[i].dns_server)->
				   sin_addr),
			   ntohs(net_sin(
				 &ctx->servers[i].dns_server)->sin_port));
		} else if (ctx->servers[i].dns_server.sa_family == AF_INET6) {
			PR("\t[%s]:%u\n",
			   net_sprint_ipv6_addr(
				   &net_sin6(&ctx->servers[i].dns_server)->
				   sin6_addr),
			   ntohs(net_sin6(
				 &ctx->servers[i].dns_server)->sin6_port));
		}
	}

	PR("Pending queries:\n");

	for (i = 0; i < CONFIG_DNS_NUM_CONCUR_QUERIES; i++) {
		int32_t remaining;

		if (!ctx->queries[i].cb || !ctx->queries[i].query) {
			continue;
		}

		remaining = k_ticks_to_ms_ceil32(
			k_work_delayable_remaining_get(&ctx->queries[i].timer));

		if (ctx->queries[i].query_type == DNS_QUERY_TYPE_A) {
			PR("\tIPv4[%u]: %s remaining %d\n",
			   ctx->queries[i].id,
			   ctx->queries[i].query,
			   remaining);
		} else if (ctx->queries[i].query_type == DNS_QUERY_TYPE_AAAA) {
			PR("\tIPv6[%u]: %s remaining %d\n",
			   ctx->queries[i].id,
			   ctx->queries[i].query,
			   remaining);
		}
	}
}
#endif

static int cmd_net_dns_cancel(const struct shell *sh, size_t argc,
			      char *argv[])
{
#if defined(CONFIG_DNS_RESOLVER)
	struct dns_resolve_context *ctx;
	int ret, i;
#endif

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_DNS_RESOLVER)
	ctx = dns_resolve_get_default();
	if (!ctx) {
		PR_WARNING("No default DNS context found.\n");
		return -ENOEXEC;
	}

	for (ret = 0, i = 0; i < CONFIG_DNS_NUM_CONCUR_QUERIES; i++) {
		if (!ctx->queries[i].cb) {
			continue;
		}

		if (!dns_resolve_cancel(ctx, ctx->queries[i].id)) {
			ret++;
		}
	}

	if (ret) {
		PR("Cancelled %d pending requests.\n", ret);
	} else {
		PR("No pending DNS requests.\n");
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_DNS_RESOLVER",
		"DNS resolver");
#endif

	return 0;
}

static int cmd_net_dns_query(const struct shell *sh, size_t argc,
			     char *argv[])
{

#if defined(CONFIG_DNS_RESOLVER)
#define DNS_TIMEOUT (MSEC_PER_SEC * 2) /* ms */
	enum dns_query_type qtype = DNS_QUERY_TYPE_A;
	char *host, *type = NULL;
	int ret, arg = 1;

	host = argv[arg++];
	if (!host) {
		PR_WARNING("Hostname not specified.\n");
		return -ENOEXEC;
	}

	if (argv[arg]) {
		type = argv[arg];
	}

	if (type) {
		if (strcmp(type, "A") == 0) {
			qtype = DNS_QUERY_TYPE_A;
			PR("IPv4 address type\n");
		} else if (strcmp(type, "AAAA") == 0) {
			qtype = DNS_QUERY_TYPE_AAAA;
			PR("IPv6 address type\n");
		} else {
			PR_WARNING("Unknown query type, specify either "
				   "A or AAAA\n");
			return -ENOEXEC;
		}
	}

	ret = dns_get_addr_info(host, qtype, NULL, dns_result_cb,
				(void *)sh, DNS_TIMEOUT);
	if (ret < 0) {
		PR_WARNING("Cannot resolve '%s' (%d)\n", host, ret);
	} else {
		PR("Query for '%s' sent.\n", host);
	}
#else
	PR_INFO("DNS resolver not supported. Set CONFIG_DNS_RESOLVER to "
		"enable it.\n");
#endif

	return 0;
}

static int cmd_net_dns(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_DNS_RESOLVER)
	struct dns_resolve_context *ctx;
#endif

#if defined(CONFIG_DNS_RESOLVER)
	if (argv[1]) {
		/* So this is a query then */
		cmd_net_dns_query(sh, argc, argv);
		return 0;
	}

	/* DNS status */
	ctx = dns_resolve_get_default();
	if (!ctx) {
		PR_WARNING("No default DNS context found.\n");
		return -ENOEXEC;
	}

	print_dns_info(sh, ctx);
#else
	PR_INFO("DNS resolver not supported. Set CONFIG_DNS_RESOLVER to "
		"enable it.\n");
#endif

	return 0;
}

#if defined(CONFIG_NET_MGMT_EVENT_MONITOR)
#define EVENT_MON_STACK_SIZE 1024
#define THREAD_PRIORITY K_PRIO_COOP(2)
#define MAX_EVENT_INFO_SIZE NET_EVENT_INFO_MAX_SIZE
#define MONITOR_L2_MASK (_NET_EVENT_IF_BASE)
#define MONITOR_L3_IPV4_MASK (_NET_EVENT_IPV4_BASE)
#define MONITOR_L3_IPV6_MASK (_NET_EVENT_IPV6_BASE)
#define MONITOR_L4_MASK (_NET_EVENT_L4_BASE)

static bool net_event_monitoring;
static bool net_event_shutting_down;
static struct net_mgmt_event_callback l2_cb;
static struct net_mgmt_event_callback l3_ipv4_cb;
static struct net_mgmt_event_callback l3_ipv6_cb;
static struct net_mgmt_event_callback l4_cb;
static struct k_thread event_mon;
static K_THREAD_STACK_DEFINE(event_mon_stack, EVENT_MON_STACK_SIZE);

struct event_msg {
	struct net_if *iface;
	size_t len;
	uint32_t event;
	uint8_t data[MAX_EVENT_INFO_SIZE];
};

K_MSGQ_DEFINE(event_mon_msgq, sizeof(struct event_msg),
	      CONFIG_NET_MGMT_EVENT_QUEUE_SIZE, sizeof(intptr_t));

static void event_handler(struct net_mgmt_event_callback *cb,
			  uint32_t mgmt_event, struct net_if *iface)
{
	struct event_msg msg;
	int ret;

	memset(&msg, 0, sizeof(msg));

	msg.len = MIN(sizeof(msg.data), cb->info_length);
	msg.event = mgmt_event;
	msg.iface = iface;

	if (cb->info_length > 0) {
		memcpy(msg.data, cb->info, msg.len);
	}

	ret = k_msgq_put(&event_mon_msgq, (void *)&msg, K_MSEC(10));
	if (ret < 0) {
		NET_ERR("Cannot write to msgq (%d)\n", ret);
	}
}

static const char *get_l2_desc(uint32_t event)
{
	static const char *desc = "<unknown event>";

	switch (event) {
	case NET_EVENT_IF_DOWN:
		desc = "down";
		break;
	case NET_EVENT_IF_UP:
		desc = "up";
		break;
	}

	return desc;
}

static char *get_l3_desc(struct event_msg *msg,
			 const char **desc, const char **desc2,
			 char *extra_info, size_t extra_info_len)
{
	static const char *desc_unknown = "<unknown event>";
	char *info = NULL;

	*desc = desc_unknown;

	switch (msg->event) {
	case NET_EVENT_IPV6_ADDR_ADD:
		*desc = "IPv6 address";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_ADDR_DEL:
		*desc = "IPv6 address";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_MADDR_ADD:
		*desc = "IPv6 mcast address";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_MADDR_DEL:
		*desc = "IPv6 mcast address";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_PREFIX_ADD:
		*desc = "IPv6 prefix";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_PREFIX_DEL:
		*desc = "IPv6 prefix";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_MCAST_JOIN:
		*desc = "IPv6 mcast";
		*desc2 = "join";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_MCAST_LEAVE:
		*desc = "IPv6 mcast";
		*desc2 = "leave";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_ROUTER_ADD:
		*desc = "IPv6 router";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_ROUTER_DEL:
		*desc = "IPv6 router";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_ROUTE_ADD:
		*desc = "IPv6 route";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_ROUTE_DEL:
		*desc = "IPv6 route";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_DAD_SUCCEED:
		*desc = "IPv6 DAD";
		*desc2 = "ok";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_DAD_FAILED:
		*desc = "IPv6 DAD";
		*desc2 = "fail";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_NBR_ADD:
		*desc = "IPv6 neighbor";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_NBR_DEL:
		*desc = "IPv6 neighbor";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET6, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV6_DHCP_START:
		*desc = "DHCPv6";
		*desc2 = "start";
		break;
	case NET_EVENT_IPV6_DHCP_BOUND:
		*desc = "DHCPv6";
		*desc2 = "bound";
#if defined(CONFIG_NET_DHCPV6)
		struct net_if_dhcpv6 *data = (struct net_if_dhcpv6 *)msg->data;

		if (data->params.request_addr) {
			info = net_addr_ntop(AF_INET6, &data->addr, extra_info,
					     extra_info_len);
		} else if (data->params.request_prefix) {
			info = net_addr_ntop(AF_INET6, &data->prefix, extra_info,
					     extra_info_len);
		}
#endif
		break;
	case NET_EVENT_IPV6_DHCP_STOP:
		*desc = "DHCPv6";
		*desc2 = "stop";
		break;
	case NET_EVENT_IPV4_ADDR_ADD:
		*desc = "IPv4 address";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV4_ADDR_DEL:
		*desc = "IPv4 address";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV4_ROUTER_ADD:
		*desc = "IPv4 router";
		*desc2 = "add";
		info = net_addr_ntop(AF_INET, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV4_ROUTER_DEL:
		*desc = "IPv4 router";
		*desc2 = "del";
		info = net_addr_ntop(AF_INET, msg->data, extra_info,
				     extra_info_len);
		break;
	case NET_EVENT_IPV4_DHCP_START:
		*desc = "DHCPv4";
		*desc2 = "start";
		break;
	case NET_EVENT_IPV4_DHCP_BOUND:
		*desc = "DHCPv4";
		*desc2 = "bound";
#if defined(CONFIG_NET_DHCPV4)
		struct net_if_dhcpv4 *data = (struct net_if_dhcpv4 *)msg->data;

		info = net_addr_ntop(AF_INET, &data->requested_ip, extra_info,
				     extra_info_len);
#endif
		break;
	case NET_EVENT_IPV4_DHCP_STOP:
		*desc = "DHCPv4";
		*desc2 = "stop";
		break;
	}

	return info;
}

static const char *get_l4_desc(uint32_t event)
{
	static const char *desc = "<unknown event>";

	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		desc = "connected";
		break;
	case NET_EVENT_L4_DISCONNECTED:
		desc = "disconnected";
		break;
	case NET_EVENT_DNS_SERVER_ADD:
		desc = "DNS server add";
		break;
	case NET_EVENT_DNS_SERVER_DEL:
		desc = "DNS server del";
		break;
	}

	return desc;
}

/* We use a separate thread in order not to do any shell printing from
 * event handler callback (to avoid stack size issues).
 */
static void event_mon_handler(const struct shell *sh)
{
	char extra_info[NET_IPV6_ADDR_LEN];
	struct event_msg msg;

	net_mgmt_init_event_callback(&l2_cb, event_handler,
				     MONITOR_L2_MASK);
	net_mgmt_add_event_callback(&l2_cb);

	net_mgmt_init_event_callback(&l3_ipv4_cb, event_handler,
				     MONITOR_L3_IPV4_MASK);
	net_mgmt_add_event_callback(&l3_ipv4_cb);

	net_mgmt_init_event_callback(&l3_ipv6_cb, event_handler,
				     MONITOR_L3_IPV6_MASK);
	net_mgmt_add_event_callback(&l3_ipv6_cb);

	net_mgmt_init_event_callback(&l4_cb, event_handler,
				     MONITOR_L4_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	while (net_event_shutting_down == false) {
		const char *layer_str = "<unknown layer>";
		const char *desc = "", *desc2 = "";
		char *info = NULL;
		uint32_t layer;

		(void)k_msgq_get(&event_mon_msgq, &msg, K_FOREVER);

		if (msg.iface == NULL && msg.event == 0 && msg.len == 0) {
			/* This is the stop message */
			continue;
		}

		layer = NET_MGMT_GET_LAYER(msg.event);
		if (layer == NET_MGMT_LAYER_L2) {
			layer_str = "L2";
			desc = get_l2_desc(msg.event);
		} else if (layer == NET_MGMT_LAYER_L3) {
			layer_str = "L3";
			info = get_l3_desc(&msg, &desc, &desc2,
					   extra_info, NET_IPV6_ADDR_LEN);
		} else if (layer == NET_MGMT_LAYER_L4) {
			layer_str = "L4";
			desc = get_l4_desc(msg.event);
		}

		PR_INFO("EVENT: %s [%d] %s%s%s%s%s\n", layer_str,
			net_if_get_by_iface(msg.iface), desc,
			desc2 ? " " : "", desc2 ? desc2 : "",
			info ? " " : "", info ? info : "");
	}

	net_mgmt_del_event_callback(&l2_cb);
	net_mgmt_del_event_callback(&l3_ipv4_cb);
	net_mgmt_del_event_callback(&l3_ipv6_cb);
	net_mgmt_del_event_callback(&l4_cb);

	k_msgq_purge(&event_mon_msgq);

	net_event_monitoring = false;
	net_event_shutting_down = false;

	PR_INFO("Network event monitoring %s.\n", "disabled");
}
#endif

static int cmd_net_events_on(const struct shell *sh, size_t argc,
			     char *argv[])
{
#if defined(CONFIG_NET_MGMT_EVENT_MONITOR)
	k_tid_t tid;

	if (net_event_monitoring) {
		PR_INFO("Network event monitoring is already %s.\n",
			"enabled");
		return -ENOEXEC;
	}

	tid = k_thread_create(&event_mon, event_mon_stack,
			      K_THREAD_STACK_SIZEOF(event_mon_stack),
			      (k_thread_entry_t)event_mon_handler,
			      (void *)sh, NULL, NULL, THREAD_PRIORITY, 0,
			      K_FOREVER);
	if (!tid) {
		PR_ERROR("Cannot create network event monitor thread!");
		return -ENOEXEC;
	}

	k_thread_name_set(tid, "event_mon");

	PR_INFO("Network event monitoring %s.\n", "enabled");

	net_event_monitoring = true;
	net_event_shutting_down = false;

	k_thread_start(tid);
#else
	PR_INFO("Network management events are not supported. "
		"Set CONFIG_NET_MGMT_EVENT_MONITOR to enable it.\n");
#endif

	return 0;
}

static int cmd_net_events_off(const struct shell *sh, size_t argc,
			      char *argv[])
{
#if defined(CONFIG_NET_MGMT_EVENT_MONITOR)
	static const struct event_msg msg;
	int ret;

	if (!net_event_monitoring) {
		PR_INFO("Network event monitoring is already %s.\n",
			"disabled");
		return -ENOEXEC;
	}

	net_event_shutting_down = true;

	ret = k_msgq_put(&event_mon_msgq, (void *)&msg, K_MSEC(100));
	if (ret < 0) {
		PR_ERROR("Cannot write to msgq (%d)\n", ret);
		return -ENOEXEC;
	}
#else
	PR_INFO("Network management events are not supported. "
		"Set CONFIG_NET_MGMT_EVENT_MONITOR to enable it.\n");
#endif

	return 0;
}

static int cmd_net_events(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_MGMT_EVENT_MONITOR)
	PR("Network event monitoring is %s.\n",
	   net_event_monitoring ? "enabled" : "disabled");

	if (!argv[1]) {
		PR_INFO("Give 'on' to enable event monitoring and "
			"'off' to disable it.\n");
	}
#else
	PR_INFO("Network management events are not supported. "
		"Set CONFIG_NET_MGMT_EVENT_MONITOR to enable it.\n");
#endif

	return 0;
}

#if defined(CONFIG_NET_GPTP)
static const char *selected_role_str(int port);

static void gptp_port_cb(int port, struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;

	if (*count == 0) {
		PR("Port Interface  \tRole\n");
	}

	(*count)++;

	PR("%2d   %p [%d]  \t%s\n", port, iface, net_if_get_by_iface(iface),
	   selected_role_str(port));
}

static const char *pdelay_req2str(enum gptp_pdelay_req_states state)
{
	switch (state) {
	case GPTP_PDELAY_REQ_NOT_ENABLED:
		return "REQ_NOT_ENABLED";
	case GPTP_PDELAY_REQ_INITIAL_SEND_REQ:
		return "INITIAL_SEND_REQ";
	case GPTP_PDELAY_REQ_RESET:
		return "REQ_RESET";
	case GPTP_PDELAY_REQ_SEND_REQ:
		return "SEND_REQ";
	case GPTP_PDELAY_REQ_WAIT_RESP:
		return "WAIT_RESP";
	case GPTP_PDELAY_REQ_WAIT_FOLLOW_UP:
		return "WAIT_FOLLOW_UP";
	case GPTP_PDELAY_REQ_WAIT_ITV_TIMER:
		return "WAIT_ITV_TIMER";
	}

	return "<unknown>";
};

static const char *pdelay_resp2str(enum gptp_pdelay_resp_states state)
{
	switch (state) {
	case GPTP_PDELAY_RESP_NOT_ENABLED:
		return "RESP_NOT_ENABLED";
	case GPTP_PDELAY_RESP_INITIAL_WAIT_REQ:
		return "INITIAL_WAIT_REQ";
	case GPTP_PDELAY_RESP_WAIT_REQ:
		return "WAIT_REQ";
	case GPTP_PDELAY_RESP_WAIT_TSTAMP:
		return "WAIT_TSTAMP";
	}

	return "<unknown>";
}

static const char *sync_rcv2str(enum gptp_sync_rcv_states state)
{
	switch (state) {
	case GPTP_SYNC_RCV_DISCARD:
		return "DISCARD";
	case GPTP_SYNC_RCV_WAIT_SYNC:
		return "WAIT_SYNC";
	case GPTP_SYNC_RCV_WAIT_FOLLOW_UP:
		return "WAIT_FOLLOW_UP";
	}

	return "<unknown>";
}

static const char *sync_send2str(enum gptp_sync_send_states state)
{
	switch (state) {
	case GPTP_SYNC_SEND_INITIALIZING:
		return "INITIALIZING";
	case GPTP_SYNC_SEND_SEND_SYNC:
		return "SEND_SYNC";
	case GPTP_SYNC_SEND_SEND_FUP:
		return "SEND_FUP";
	}

	return "<unknown>";
}

static const char *pss_rcv2str(enum gptp_pss_rcv_states state)
{
	switch (state) {
	case GPTP_PSS_RCV_DISCARD:
		return "DISCARD";
	case GPTP_PSS_RCV_RECEIVED_SYNC:
		return "RECEIVED_SYNC";
	}

	return "<unknown>";
}

static const char *pss_send2str(enum gptp_pss_send_states state)
{
	switch (state) {
	case GPTP_PSS_SEND_TRANSMIT_INIT:
		return "TRANSMIT_INIT";
	case GPTP_PSS_SEND_SYNC_RECEIPT_TIMEOUT:
		return "SYNC_RECEIPT_TIMEOUT";
	case GPTP_PSS_SEND_SEND_MD_SYNC:
		return "SEND_MD_SYNC";
	case GPTP_PSS_SEND_SET_SYNC_RECEIPT_TIMEOUT:
		return "SET_SYNC_RECEIPT_TIMEOUT";
	}

	return "<unknown>";
}

static const char *pa_rcv2str(enum gptp_pa_rcv_states state)
{
	switch (state) {
	case GPTP_PA_RCV_DISCARD:
		return "DISCARD";
	case GPTP_PA_RCV_RECEIVE:
		return "RECEIVE";
	}

	return "<unknown>";
};

static const char *pa_info2str(enum gptp_pa_info_states state)
{
	switch (state) {
	case GPTP_PA_INFO_DISABLED:
		return "DISABLED";
	case GPTP_PA_INFO_POST_DISABLED:
		return "POST_DISABLED";
	case GPTP_PA_INFO_AGED:
		return "AGED";
	case GPTP_PA_INFO_UPDATE:
		return "UPDATE";
	case GPTP_PA_INFO_CURRENT:
		return "CURRENT";
	case GPTP_PA_INFO_RECEIVE:
		return "RECEIVE";
	case GPTP_PA_INFO_SUPERIOR_MASTER_PORT:
		return "SUPERIOR_MASTER_PORT";
	case GPTP_PA_INFO_REPEATED_MASTER_PORT:
		return "REPEATED_MASTER_PORT";
	case GPTP_PA_INFO_INFERIOR_MASTER_OR_OTHER_PORT:
		return "INFERIOR_MASTER_OR_OTHER_PORT";
	}

	return "<unknown>";
};

static const char *pa_transmit2str(enum gptp_pa_transmit_states state)
{
	switch (state) {
	case GPTP_PA_TRANSMIT_INIT:
		return "INIT";
	case GPTP_PA_TRANSMIT_PERIODIC:
		return "PERIODIC";
	case GPTP_PA_TRANSMIT_IDLE:
		return "IDLE";
	case GPTP_PA_TRANSMIT_POST_IDLE:
		return "POST_IDLE";
	}

	return "<unknown>";
};

static const char *site_sync2str(enum gptp_site_sync_sync_states state)
{
	switch (state) {
	case GPTP_SSS_INITIALIZING:
		return "INITIALIZING";
	case GPTP_SSS_RECEIVING_SYNC:
		return "RECEIVING_SYNC";
	}

	return "<unknown>";
}

static const char *clk_slave2str(enum gptp_clk_slave_sync_states state)
{
	switch (state) {
	case GPTP_CLK_SLAVE_SYNC_INITIALIZING:
		return "INITIALIZING";
	case GPTP_CLK_SLAVE_SYNC_SEND_SYNC_IND:
		return "SEND_SYNC_IND";
	}

	return "<unknown>";
};

static const char *pr_selection2str(enum gptp_pr_selection_states state)
{
	switch (state) {
	case GPTP_PR_SELECTION_INIT_BRIDGE:
		return "INIT_BRIDGE";
	case GPTP_PR_SELECTION_ROLE_SELECTION:
		return "ROLE_SELECTION";
	}

	return "<unknown>";
};

static const char *cms_rcv2str(enum gptp_cms_rcv_states state)
{
	switch (state) {
	case GPTP_CMS_RCV_INITIALIZING:
		return "INITIALIZING";
	case GPTP_CMS_RCV_WAITING:
		return "WAITING";
	case GPTP_CMS_RCV_SOURCE_TIME:
		return "SOURCE_TIME";
	}

	return "<unknown>";
};

#if !defined(USCALED_NS_TO_NS)
#define USCALED_NS_TO_NS(val) (val >> 16)
#endif

static const char *selected_role_str(int port)
{
	switch (GPTP_GLOBAL_DS()->selected_role[port]) {
	case GPTP_PORT_INITIALIZING:
		return "INITIALIZING";
	case GPTP_PORT_FAULTY:
		return "FAULTY";
	case GPTP_PORT_DISABLED:
		return "DISABLED";
	case GPTP_PORT_LISTENING:
		return "LISTENING";
	case GPTP_PORT_PRE_MASTER:
		return "PRE-MASTER";
	case GPTP_PORT_MASTER:
		return "MASTER";
	case GPTP_PORT_PASSIVE:
		return "PASSIVE";
	case GPTP_PORT_UNCALIBRATED:
		return "UNCALIBRATED";
	case GPTP_PORT_SLAVE:
		return "SLAVE";
	}

	return "<unknown>";
}

static void gptp_print_port_info(const struct shell *sh, int port)
{
	struct gptp_port_bmca_data *port_bmca_data;
	struct gptp_port_param_ds *port_param_ds;
	struct gptp_port_states *port_state;
	struct gptp_domain *domain;
	struct gptp_port_ds *port_ds;
	struct net_if *iface;
	int ret, i;

	domain = gptp_get_domain();

	ret = gptp_get_port_data(domain,
				 port,
				 &port_ds,
				 &port_param_ds,
				 &port_state,
				 &port_bmca_data,
				 &iface);
	if (ret < 0) {
		PR_WARNING("Cannot get gPTP information for port %d (%d)\n",
			   port, ret);
		return;
	}

	NET_ASSERT(port == port_ds->port_id.port_number,
		   "Port number mismatch! (%d vs %d)", port,
		   port_ds->port_id.port_number);

	PR("Port id    : %d (%s)\n", port_ds->port_id.port_number,
	   selected_role_str(port_ds->port_id.port_number));
	PR("Interface  : %p [%d]\n", iface, net_if_get_by_iface(iface));
	PR("Clock id   : ");
	for (i = 0; i < sizeof(port_ds->port_id.clk_id); i++) {
		PR("%02x", port_ds->port_id.clk_id[i]);

		if (i != (sizeof(port_ds->port_id.clk_id) - 1)) {
			PR(":");
		}
	}
	PR("\n");

	PR("Version    : %d\n", port_ds->version);
	PR("AS capable : %s\n", port_ds->as_capable ? "yes" : "no");

	PR("\nConfiguration:\n");
	PR("Time synchronization and Best Master Selection enabled        "
	   ": %s\n", port_ds->ptt_port_enabled ? "yes" : "no");
	PR("The port is measuring the path delay                          "
	   ": %s\n", port_ds->is_measuring_delay ? "yes" : "no");
	PR("One way propagation time on %s    : %u ns\n",
	   "the link attached to this port",
	   (uint32_t)port_ds->neighbor_prop_delay);
	PR("Propagation time threshold for %s : %u ns\n",
	   "the link attached to this port",
	   (uint32_t)port_ds->neighbor_prop_delay_thresh);
	PR("Estimate of the ratio of the frequency with the peer          "
	   ": %u\n", (uint32_t)port_ds->neighbor_rate_ratio);
	PR("Asymmetry on the link relative to the grand master time base  "
	   ": %" PRId64 "\n", port_ds->delay_asymmetry);
	PR("Maximum interval between sync %s                        "
	   ": %" PRIu64 "\n", "messages",
	   port_ds->sync_receipt_timeout_time_itv);
	PR("Maximum number of Path Delay Requests without a response      "
	   ": %d\n", port_ds->allowed_lost_responses);
	PR("Current Sync %s                        : %d\n",
	   "sequence id for this port", port_ds->sync_seq_id);
	PR("Current Path Delay Request %s          : %d\n",
	   "sequence id for this port", port_ds->pdelay_req_seq_id);
	PR("Current Announce %s                    : %d\n",
	   "sequence id for this port", port_ds->announce_seq_id);
	PR("Current Signaling %s                   : %d\n",
	   "sequence id for this port", port_ds->signaling_seq_id);
	PR("Whether neighborRateRatio %s  : %s\n",
	   "needs to be computed for this port",
	   port_ds->compute_neighbor_rate_ratio ? "yes" : "no");
	PR("Whether neighborPropDelay %s  : %s\n",
	   "needs to be computed for this port",
	   port_ds->compute_neighbor_prop_delay ? "yes" : "no");
	PR("Initial Announce Interval %s            : %d\n",
	   "as a Logarithm to base 2", port_ds->ini_log_announce_itv);
	PR("Current Announce Interval %s            : %d\n",
	   "as a Logarithm to base 2", port_ds->cur_log_announce_itv);
	PR("Initial Sync Interval %s                : %d\n",
	   "as a Logarithm to base 2", port_ds->ini_log_half_sync_itv);
	PR("Current Sync Interval %s                : %d\n",
	   "as a Logarithm to base 2", port_ds->cur_log_half_sync_itv);
	PR("Initial Path Delay Request Interval %s  : %d\n",
	   "as a Logarithm to base 2", port_ds->ini_log_pdelay_req_itv);
	PR("Current Path Delay Request Interval %s  : %d\n",
	   "as a Logarithm to base 2", port_ds->cur_log_pdelay_req_itv);
	PR("Time without receiving announce %s %s  : %d ms (%d)\n",
	   "messages", "before running BMCA",
	   gptp_uscaled_ns_to_timer_ms(
		   &port_bmca_data->ann_rcpt_timeout_time_interval),
	   port_ds->announce_receipt_timeout);
	PR("Time without receiving sync %s %s      : %" PRIu64 " ms (%d)\n",
	   "messages", "before running BMCA",
	   (port_ds->sync_receipt_timeout_time_itv >> 16) /
					(NSEC_PER_SEC / MSEC_PER_SEC),
	   port_ds->sync_receipt_timeout);
	PR("Sync event %s                 : %" PRIu64 " ms\n",
	   "transmission interval for the port",
	   USCALED_NS_TO_NS(port_ds->half_sync_itv.low) /
					(NSEC_PER_USEC * USEC_PER_MSEC));
	PR("Path Delay Request %s         : %" PRIu64 " ms\n",
	   "transmission interval for the port",
	   USCALED_NS_TO_NS(port_ds->pdelay_req_itv.low) /
					(NSEC_PER_USEC * USEC_PER_MSEC));
	PR("BMCA %s %s%d%s: %d\n", "default", "priority", 1,
	   "                                        ",
	   domain->default_ds.priority1);
	PR("BMCA %s %s%d%s: %d\n", "default", "priority", 2,
	   "                                        ",
	   domain->default_ds.priority2);

	PR("\nRuntime status:\n");
	PR("Current global port state                          "
	   "      : %s\n", selected_role_str(port));
	PR("Path Delay Request state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", pdelay_req2str(port_state->pdelay_req.state));
	PR("\tInitial Path Delay Response Peer Timestamp       "
	   ": %" PRIu64 "\n", port_state->pdelay_req.ini_resp_evt_tstamp);
	PR("\tInitial Path Delay Response Ingress Timestamp    "
	   ": %" PRIu64 "\n", port_state->pdelay_req.ini_resp_ingress_tstamp);
	PR("\tPath Delay Response %s %s            : %u\n",
	   "messages", "received",
	   port_state->pdelay_req.rcvd_pdelay_resp);
	PR("\tPath Delay Follow Up %s %s           : %u\n",
	   "messages", "received",
	   port_state->pdelay_req.rcvd_pdelay_follow_up);
	PR("\tNumber of lost Path Delay Responses              "
	   ": %u\n", port_state->pdelay_req.lost_responses);
	PR("\tTimer expired send a new Path Delay Request      "
	   ": %u\n", port_state->pdelay_req.pdelay_timer_expired);
	PR("\tNeighborRateRatio has been computed successfully "
	   ": %u\n", port_state->pdelay_req.neighbor_rate_ratio_valid);
	PR("\tPath Delay has already been computed after init  "
	   ": %u\n", port_state->pdelay_req.init_pdelay_compute);
	PR("\tCount consecutive reqs with multiple responses   "
	   ": %u\n", port_state->pdelay_req.multiple_resp_count);

	PR("Path Delay Response state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", pdelay_resp2str(port_state->pdelay_resp.state));

	PR("SyncReceive state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", sync_rcv2str(port_state->sync_rcv.state));
	PR("\tA Sync %s %s                 : %s\n",
	   "Message", "has been received",
	   port_state->sync_rcv.rcvd_sync ? "yes" : "no");
	PR("\tA Follow Up %s %s            : %s\n",
	   "Message", "has been received",
	   port_state->sync_rcv.rcvd_follow_up ? "yes" : "no");
	PR("\tA Follow Up %s %s                      : %s\n",
	   "Message", "timeout",
	   port_state->sync_rcv.follow_up_timeout_expired ? "yes" : "no");
	PR("\tTime at which a Sync %s without Follow Up\n"
	   "\t                             will be discarded   "
	   ": %" PRIu64 "\n", "Message",
	   port_state->sync_rcv.follow_up_receipt_timeout);

	PR("SyncSend state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", sync_send2str(port_state->sync_send.state));
	PR("\tA MDSyncSend structure %s         : %s\n",
	   "has been received",
	   port_state->sync_send.rcvd_md_sync ? "yes" : "no");
	PR("\tThe timestamp for the sync msg %s : %s\n",
	   "has been received",
	   port_state->sync_send.md_sync_timestamp_avail ? "yes" : "no");

	PR("PortSyncSyncReceive state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", pss_rcv2str(port_state->pss_rcv.state));
	PR("\tGrand Master / Local Clock frequency ratio       "
	   ": %f\n", port_state->pss_rcv.rate_ratio);
	PR("\tA MDSyncReceive struct is ready to be processed  "
	   ": %s\n", port_state->pss_rcv.rcvd_md_sync ? "yes" : "no");
	PR("\tExpiry of SyncReceiptTimeoutTimer                : %s\n",
	   port_state->pss_rcv.rcv_sync_receipt_timeout_timer_expired ?
	   "yes" : "no");

	PR("PortSyncSyncSend state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", pss_send2str(port_state->pss_send.state));
	PR("\tFollow Up Correction Field of last recv PSS      "
	   ": %" PRId64 "\n",
	   port_state->pss_send.last_follow_up_correction_field);
	PR("\tUpstream Tx Time of the last recv PortSyncSync   "
	   ": %" PRIu64 "\n", port_state->pss_send.last_upstream_tx_time);
	PR("\tRate Ratio of the last received PortSyncSync     "
	   ": %f\n",
	   port_state->pss_send.last_rate_ratio);
	PR("\tGM Freq Change of the last received PortSyncSync "
	   ": %f\n", port_state->pss_send.last_gm_freq_change);
	PR("\tGM Time Base Indicator of last recv PortSyncSync "
	   ": %d\n", port_state->pss_send.last_gm_time_base_indicator);
	PR("\tReceived Port Number of last recv PortSyncSync   "
	   ": %d\n",
	   port_state->pss_send.last_rcvd_port_num);
	PR("\tPortSyncSync structure is ready to be processed  "
	   ": %s\n", port_state->pss_send.rcvd_pss_sync ? "yes" : "no");
	PR("\tFlag when the %s has expired    : %s\n",
	   "half_sync_itv_timer",
	   port_state->pss_send.half_sync_itv_timer_expired ? "yes" : "no");
	PR("\tHas %s expired twice            : %s\n",
	   "half_sync_itv_timer",
	   port_state->pss_send.sync_itv_timer_expired ? "yes" : "no");
	PR("\tHas syncReceiptTimeoutTime expired               "
	   ": %s\n",
	   port_state->pss_send.send_sync_receipt_timeout_timer_expired ?
	   "yes" : "no");

	PR("PortAnnounceReceive state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", pa_rcv2str(port_state->pa_rcv.state));
	PR("\tAn announce message is ready to be processed     "
	   ": %s\n",
	   port_state->pa_rcv.rcvd_announce ? "yes" : "no");

	PR("PortAnnounceInformation state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", pa_info2str(port_state->pa_info.state));
	PR("\tExpired announce information                     "
	   ": %s\n", port_state->pa_info.ann_expired ? "yes" : "no");

	PR("PortAnnounceTransmit state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", pa_transmit2str(port_state->pa_transmit.state));
	PR("\tTrigger announce information                     "
	   ": %s\n", port_state->pa_transmit.ann_trigger ? "yes" : "no");

#if defined(CONFIG_NET_GPTP_STATISTICS)
	PR("\nStatistics:\n");
	PR("Sync %s %s                 : %u\n",
	   "messages", "received", port_param_ds->rx_sync_count);
	PR("Follow Up %s %s            : %u\n",
	   "messages", "received", port_param_ds->rx_fup_count);
	PR("Path Delay Request %s %s   : %u\n",
	   "messages", "received", port_param_ds->rx_pdelay_req_count);
	PR("Path Delay Response %s %s  : %u\n",
	   "messages", "received", port_param_ds->rx_pdelay_resp_count);
	PR("Path Delay %s threshold %s : %u\n",
	   "messages", "exceeded",
	   port_param_ds->neighbor_prop_delay_exceeded);
	PR("Path Delay Follow Up %s %s : %u\n",
	   "messages", "received", port_param_ds->rx_pdelay_resp_fup_count);
	PR("Announce %s %s             : %u\n",
	   "messages", "received", port_param_ds->rx_announce_count);
	PR("ptp %s discarded                 : %u\n",
	   "messages", port_param_ds->rx_ptp_packet_discard_count);
	PR("Sync %s %s                 : %u\n",
	   "reception", "timeout",
	   port_param_ds->sync_receipt_timeout_count);
	PR("Announce %s %s             : %u\n",
	   "reception", "timeout",
	   port_param_ds->announce_receipt_timeout_count);
	PR("Path Delay Requests without a response "
	   ": %u\n",
	   port_param_ds->pdelay_allowed_lost_resp_exceed_count);
	PR("Sync %s %s                     : %u\n",
	   "messages", "sent", port_param_ds->tx_sync_count);
	PR("Follow Up %s %s                : %u\n",
	   "messages", "sent", port_param_ds->tx_fup_count);
	PR("Path Delay Request %s %s       : %u\n",
	   "messages", "sent", port_param_ds->tx_pdelay_req_count);
	PR("Path Delay Response %s %s      : %u\n",
	   "messages", "sent", port_param_ds->tx_pdelay_resp_count);
	PR("Path Delay Response FUP %s %s  : %u\n",
	   "messages", "sent", port_param_ds->tx_pdelay_resp_fup_count);
	PR("Announce %s %s                 : %u\n",
	   "messages", "sent", port_param_ds->tx_announce_count);
#endif /* CONFIG_NET_GPTP_STATISTICS */
}
#endif /* CONFIG_NET_GPTP */

static int cmd_net_gptp_port(const struct shell *sh, size_t argc,
			     char *argv[])
{
#if defined(CONFIG_NET_GPTP)
	int arg = 1;
	char *endptr;
	int port;
#endif

#if defined(CONFIG_NET_GPTP)
	if (!argv[arg]) {
		PR_WARNING("Port number must be given.\n");
		return -ENOEXEC;
	}

	port = strtol(argv[arg], &endptr, 10);

	if (*endptr == '\0') {
		gptp_print_port_info(sh, port);
	} else {
		PR_WARNING("Not a valid gPTP port number: %s\n", argv[arg]);
	}
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_GPTP", "gPTP");
#endif

	return 0;
}

static int cmd_net_gptp(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_GPTP)
	/* gPTP status */
	struct gptp_domain *domain = gptp_get_domain();
	int count = 0;
	int arg = 1;
#endif

#if defined(CONFIG_NET_GPTP)
	if (argv[arg]) {
		cmd_net_gptp_port(sh, argc, argv);
	} else {
		struct net_shell_user_data user_data;

		user_data.sh = sh;
		user_data.user_data = &count;

		gptp_foreach_port(gptp_port_cb, &user_data);

		PR("\n");

		PR("SiteSyncSync state machine variables:\n");
		PR("\tCurrent state                  : %s\n",
		   site_sync2str(domain->state.site_ss.state));
		PR("\tA PortSyncSync struct is ready : %s\n",
		   domain->state.site_ss.rcvd_pss ? "yes" : "no");

		PR("ClockSlaveSync state machine variables:\n");
		PR("\tCurrent state                  : %s\n",
		   clk_slave2str(domain->state.clk_slave_sync.state));
		PR("\tA PortSyncSync struct is ready : %s\n",
		   domain->state.clk_slave_sync.rcvd_pss ? "yes" : "no");
		PR("\tThe local clock has expired    : %s\n",
		   domain->state.clk_slave_sync.rcvd_local_clk_tick ?
							   "yes" : "no");

		PR("PortRoleSelection state machine variables:\n");
		PR("\tCurrent state                  : %s\n",
		   pr_selection2str(domain->state.pr_sel.state));

		PR("ClockMasterSyncReceive state machine variables:\n");
		PR("\tCurrent state                  : %s\n",
		   cms_rcv2str(domain->state.clk_master_sync_receive.state));
		PR("\tA ClockSourceTime              : %s\n",
		   domain->state.clk_master_sync_receive.rcvd_clock_source_req
							       ? "yes" : "no");
		PR("\tThe local clock has expired    : %s\n",
		   domain->state.clk_master_sync_receive.rcvd_local_clock_tick
							       ? "yes" : "no");
	}
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_GPTP", "gPTP");
#endif

	return 0;
}

static int get_iface_idx(const struct shell *sh, char *index_str)
{
	char *endptr;
	int idx;

	if (!index_str) {
		PR_WARNING("Interface index is missing.\n");
		return -EINVAL;
	}

	idx = strtol(index_str, &endptr, 10);
	if (*endptr != '\0') {
		PR_WARNING("Invalid index %s\n", index_str);
		return -ENOENT;
	}

	if (idx < 0 || idx > 255) {
		PR_WARNING("Invalid index %d\n", idx);
		return -ERANGE;
	}

	return idx;
}

static int cmd_net_iface_up(const struct shell *sh, size_t argc,
			    char *argv[])
{
	struct net_if *iface;
	int idx, ret;

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOEXEC;
	}

	if (net_if_is_up(iface)) {
		PR_WARNING("Interface %d is already up.\n", idx);
		return -ENOEXEC;
	}

	ret = net_if_up(iface);
	if (ret) {
		PR_WARNING("Cannot take interface %d up (%d)\n", idx, ret);
		return -ENOEXEC;
	} else {
		PR("Interface %d is up\n", idx);
	}

	return 0;
}

static int cmd_net_iface_down(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct net_if *iface;
	int idx, ret;

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOEXEC;
	}

	ret = net_if_down(iface);
	if (ret) {
		PR_WARNING("Cannot take interface %d down (%d)\n", idx, ret);
		return -ENOEXEC;
	} else {
		PR("Interface %d is down\n", idx);
	}

	return 0;
}

#if defined(CONFIG_NET_NATIVE_IPV6)

static void address_lifetime_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	const char *extra;
	int i;

	ARG_UNUSED(user_data);

	PR("\nIPv6 addresses for interface %d (%p) (%s)\n",
	   net_if_get_by_iface(iface), iface, iface2str(iface, &extra));
	PR("============================================%s\n", extra);

	if (!ipv6) {
		PR("No IPv6 config found for this interface.\n");
		return;
	}

	PR("Type      \tState    \tLifetime (sec)\tAddress\n");

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		struct net_if_ipv6_prefix *prefix;
		char remaining_str[sizeof("01234567890")];
		uint64_t remaining;
		uint8_t prefix_len;

		if (!ipv6->unicast[i].is_used ||
		    ipv6->unicast[i].address.family != AF_INET6) {
			continue;
		}

		remaining = net_timeout_remaining(&ipv6->unicast[i].lifetime,
						  k_uptime_get_32());

		prefix = net_if_ipv6_prefix_get(iface,
					   &ipv6->unicast[i].address.in6_addr);
		if (prefix) {
			prefix_len = prefix->len;
		} else {
			prefix_len = 128U;
		}

		if (ipv6->unicast[i].is_infinite) {
			snprintk(remaining_str, sizeof(remaining_str) - 1,
				 "infinite");
		} else {
			snprintk(remaining_str, sizeof(remaining_str) - 1,
				 "%u", (uint32_t)(remaining / 1000U));
		}

		PR("%s  \t%s\t%s    \t%s/%d\n",
		       addrtype2str(ipv6->unicast[i].addr_type),
		       addrstate2str(ipv6->unicast[i].addr_state),
		       remaining_str,
		       net_sprint_ipv6_addr(
			       &ipv6->unicast[i].address.in6_addr),
		       prefix_len);
	}
}
#endif /* CONFIG_NET_NATIVE_IPV6 */

static int cmd_net_ipv6(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_NATIVE_IPV6)
	struct net_shell_user_data user_data;
#endif

	PR("IPv6 support                              : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6) ?
	   "enabled" : "disabled");
	if (!IS_ENABLED(CONFIG_NET_IPV6)) {
		return -ENOEXEC;
	}

#if defined(CONFIG_NET_NATIVE_IPV6)
	PR("IPv6 fragmentation support                : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6_FRAGMENT) ? "enabled" :
	   "disabled");
	PR("Multicast Listener Discovery support      : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6_MLD) ? "enabled" :
	   "disabled");
	PR("Neighbor cache support                    : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6_NBR_CACHE) ? "enabled" :
	   "disabled");
	PR("Neighbor discovery support                : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6_ND) ? "enabled" :
	   "disabled");
	PR("Duplicate address detection (DAD) support : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6_DAD) ? "enabled" :
	   "disabled");
	PR("Router advertisement RDNSS option support : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6_RA_RDNSS) ? "enabled" :
	   "disabled");
	PR("6lo header compression support            : %s\n",
	   IS_ENABLED(CONFIG_NET_6LO) ? "enabled" :
	   "disabled");

	if (IS_ENABLED(CONFIG_NET_6LO_CONTEXT)) {
		PR("6lo context based compression "
		   "support     : %s\n",
		   IS_ENABLED(CONFIG_NET_6LO_CONTEXT) ? "enabled" :
		   "disabled");
	}

	PR("Max number of IPv6 network interfaces "
	   "in the system          : %d\n",
	   CONFIG_NET_IF_MAX_IPV6_COUNT);
	PR("Max number of unicast IPv6 addresses "
	   "per network interface   : %d\n",
	   CONFIG_NET_IF_UNICAST_IPV6_ADDR_COUNT);
	PR("Max number of multicast IPv6 addresses "
	   "per network interface : %d\n",
	   CONFIG_NET_IF_MCAST_IPV6_ADDR_COUNT);
	PR("Max number of IPv6 prefixes per network "
	   "interface            : %d\n",
	   CONFIG_NET_IF_IPV6_PREFIX_COUNT);

	user_data.sh = sh;
	user_data.user_data = NULL;

	/* Print information about address lifetime */
	net_if_foreach(address_lifetime_cb, &user_data);
#endif

	return 0;
}

static int cmd_net_ip6_add(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_NATIVE_IPV6)
	struct net_if *iface = NULL;
	int idx;
	struct in6_addr addr;

	if (argc != 3) {
		PR_ERROR("Correct usage: net ipv6 add <index> <address>\n");
		return -EINVAL;
	}

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOENT;
	}

	if (net_addr_pton(AF_INET6, argv[2], &addr)) {
		PR_ERROR("Invalid address: %s\n", argv[2]);
		return -EINVAL;
	}

	if (!net_if_ipv6_addr_add(iface, &addr, NET_ADDR_MANUAL, 0)) {
		PR_ERROR("Failed to add %s address to interface %p\n", argv[2], iface);
	}

#else /* CONFIG_NET_NATIVE_IPV6 */
	PR_INFO("Set %s and %s to enable native %s support.\n",
			"CONFIG_NET_NATIVE", "CONFIG_NET_IPV6", "IPv6");
#endif /* CONFIG_NET_NATIVE_IPV6 */
	return 0;
}

static int cmd_net_ip6_del(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_NATIVE_IPV6)
	struct net_if *iface = NULL;
	int idx;
	struct in6_addr addr;

	if (argc != 3) {
		PR_ERROR("Correct usage: net ipv6 del <index> <address>\n");
		return -EINVAL;
	}

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOEXEC;
	}

	if (net_addr_pton(AF_INET6, argv[2], &addr)) {
		PR_ERROR("Invalid address: %s\n", argv[2]);
		return -EINVAL;
	}

	if (!net_if_ipv6_addr_rm(iface, &addr)) {
		PR_ERROR("Failed to delete %s\n", argv[2]);
		return -1;
	}

#else /* CONFIG_NET_NATIVE_IPV6 */
	PR_INFO("Set %s and %s to enable native %s support.\n",
			"CONFIG_NET_NATIVE", "CONFIG_NET_IPV6", "IPv6");
#endif /* CONFIG_NET_NATIVE_IPV6 */
	return 0;
}

static int cmd_net_ip6_route_add(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_NATIVE_IPV6) && (CONFIG_NET_ROUTE)
	struct net_if *iface = NULL;
	int idx;
	struct net_route_entry *route;
	struct in6_addr gw = {0};
	struct in6_addr prefix = {0};

	if (argc != 4) {
		PR_ERROR("Correct usage: net route add <index> "
				 "<destination> <gateway>\n");
		return -EINVAL;
	}

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOEXEC;
	}

	if (net_addr_pton(AF_INET6, argv[2], &prefix)) {
		PR_ERROR("Invalid address: %s\n", argv[2]);
		return -EINVAL;
	}

	if (net_addr_pton(AF_INET6, argv[3], &gw)) {
		PR_ERROR("Invalid gateway: %s\n", argv[3]);
		return -EINVAL;
	}

	route = net_route_add(iface, &prefix, NET_IPV6_DEFAULT_PREFIX_LEN,
				&gw, NET_IPV6_ND_INFINITE_LIFETIME,
				NET_ROUTE_PREFERENCE_MEDIUM);
	if (route == NULL) {
		PR_ERROR("Failed to add route\n");
		return -ENOEXEC;
	}

#else /* CONFIG_NET_NATIVE_IPV6 && CONFIG_NET_ROUTE */
	PR_INFO("Set %s and %s to enable native %s support."
			" And enable CONFIG_NET_ROUTE.\n",
			"CONFIG_NET_NATIVE", "CONFIG_NET_IPV6", "IPv6");
#endif /* CONFIG_NET_NATIVE_IPV6 && CONFIG_NET_ROUTE */
	return 0;
}

static int cmd_net_ip6_route_del(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_NATIVE_IPV6) && (CONFIG_NET_ROUTE)
	struct net_if *iface = NULL;
	int idx;
	struct net_route_entry *route;
	struct in6_addr prefix = { 0 };

	if (argc != 3) {
		PR_ERROR("Correct usage: net route del <index> <destination>\n");
		return -EINVAL;
	}
	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOEXEC;
	}

	if (net_addr_pton(AF_INET6, argv[2], &prefix)) {
		PR_ERROR("Invalid address: %s\n", argv[2]);
		return -EINVAL;
	}

	route = net_route_lookup(iface, &prefix);
	if (route) {
		net_route_del(route);
	}
#else /* CONFIG_NET_NATIVE_IPV6 && CONFIG_NET_ROUTE */
	PR_INFO("Set %s and %s to enable native %s support."
			" And enable CONFIG_NET_ROUTE\n",
			"CONFIG_NET_NATIVE", "CONFIG_NET_IPV6", "IPv6");
#endif /* CONFIG_NET_NATIVE_IPV6 && CONFIG_NET_ROUTE */
	return 0;
}

#if defined(CONFIG_NET_NATIVE_IPV4)
static void ip_address_lifetime_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	const char *extra;
	int i;

	ARG_UNUSED(user_data);

	PR("\nIPv4 addresses for interface %d (%p) (%s)\n",
	   net_if_get_by_iface(iface), iface, iface2str(iface, &extra));
	PR("============================================%s\n", extra);

	if (!ipv4) {
		PR("No IPv4 config found for this interface.\n");
		return;
	}

	PR("Type      \tState    \tLifetime (sec)\tAddress\n");

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (!ipv4->unicast[i].is_used ||
		    ipv4->unicast[i].address.family != AF_INET) {
			continue;
		}

		PR("%s  \t%s    \t%12s/%12s\n",
		       addrtype2str(ipv4->unicast[i].addr_type),
		       addrstate2str(ipv4->unicast[i].addr_state),
		       net_sprint_ipv4_addr(
			       &ipv4->unicast[i].address.in_addr),
		       net_sprint_ipv4_addr(
			       &ipv4->netmask));
	}
}
#endif /* CONFIG_NET_NATIVE_IPV4 */

static int cmd_net_ipv4(const struct shell *sh, size_t argc, char *argv[])
{
	PR("IPv4 support                              : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV4) ?
	   "enabled" : "disabled");
	if (!IS_ENABLED(CONFIG_NET_IPV4)) {
		return -ENOEXEC;
	}

#if defined(CONFIG_NET_NATIVE_IPV4)
	struct net_shell_user_data user_data;

	PR("IPv4 fragmentation support                : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV4_FRAGMENT) ? "enabled" :
	   "disabled");
	PR("Max number of IPv4 network interfaces "
	   "in the system          : %d\n",
	   CONFIG_NET_IF_MAX_IPV4_COUNT);
	PR("Max number of unicast IPv4 addresses "
	   "per network interface   : %d\n",
	   CONFIG_NET_IF_UNICAST_IPV4_ADDR_COUNT);
	PR("Max number of multicast IPv4 addresses "
	   "per network interface : %d\n",
	   CONFIG_NET_IF_MCAST_IPV4_ADDR_COUNT);

	user_data.sh = sh;
	user_data.user_data = NULL;

	/* Print information about address lifetime */
	net_if_foreach(ip_address_lifetime_cb, &user_data);
#endif

	return 0;
}

static int cmd_net_ip_add(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_NATIVE_IPV4)
	struct net_if *iface = NULL;
	int idx;
	struct in_addr addr;

	if (argc != 4) {
		PR_ERROR("Correct usage: net ipv4 add <index> <address> <netmask>\n");
		return -EINVAL;
	}

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOEXEC;
	}

	if (net_addr_pton(AF_INET, argv[2], &addr)) {
		PR_ERROR("Invalid address: %s\n", argv[2]);
		return -EINVAL;
	}

	net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);

	if (net_addr_pton(AF_INET, argv[3], &addr)) {
		PR_ERROR("Invalid netmask: %s", argv[3]);
		return -EINVAL;
	}

	net_if_ipv4_set_netmask(iface, &addr);

#else /* CONFIG_NET_NATIVE_IPV4 */
	PR_INFO("Set %s and %s to enable native %s support.\n",
			"CONFIG_NET_NATIVE", "CONFIG_NET_IPV4", "IPv4");
#endif /* CONFIG_NET_NATIVE_IPV4 */
	return 0;
}

static int cmd_net_ip_del(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_NATIVE_IPV4)
	struct net_if *iface = NULL;
	int idx;
	struct in_addr addr;

	if (argc != 3) {
		PR_ERROR("Correct usage: net ipv4 del <index> <address>");
		return -EINVAL;
	}

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOEXEC;
	}

	if (net_addr_pton(AF_INET, argv[2], &addr)) {
		PR_ERROR("Invalid address: %s\n", argv[2]);
		return -EINVAL;
	}

	if (!net_if_ipv4_addr_rm(iface, &addr)) {
		PR_ERROR("Failed to delete %s\n", argv[2]);
		return -ENOEXEC;
	}
#else /* CONFIG_NET_NATIVE_IPV4 */
	PR_INFO("Set %s and %s to enable native %s support.\n",
			"CONFIG_NET_NATIVE", "CONFIG_NET_IPV4", "IPv4");
#endif /* CONFIG_NET_NATIVE_IPV4 */
	return 0;
}

static int cmd_net_iface(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = NULL;
	struct net_shell_user_data user_data;
	int idx;

	if (argv[1]) {
		idx = get_iface_idx(sh, argv[1]);
		if (idx < 0) {
			return -ENOEXEC;
		}

		iface = net_if_get_by_index(idx);
		if (!iface) {
			PR_WARNING("No such interface in index %d\n", idx);
			return -ENOEXEC;
		}
	}

#if defined(CONFIG_NET_HOSTNAME_ENABLE)
	PR("Hostname: %s\n\n", net_hostname_get());
#endif

	user_data.sh = sh;
	user_data.user_data = iface;

	net_if_foreach(iface_cb, &user_data);

	return 0;
}

static int cmd_net_set_mac(const struct shell *sh, size_t argc, char *argv[])
{
#if !defined(CONFIG_NET_L2_ETHERNET) || !defined(CONFIG_NET_L2_ETHERNET_MGMT)
	PR_WARNING("Unsupported command, please enable CONFIG_NET_L2_ETHERNET "
		"and CONFIG_NET_L2_ETHERNET_MGMT\n");
	return -ENOEXEC;
#else
	struct net_if *iface;
	struct ethernet_req_params params;
	char *mac_addr = params.mac_address.addr;
	int idx;
	int ret;

	if (argc < 3) {
		PR_WARNING("Missing interface index and/or MAC address\n");
		goto err;
	}

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		goto err;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		goto err;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		PR_WARNING("MAC address can be set only for Ethernet\n");
		goto err;
	}

	if ((net_bytes_from_str(mac_addr, sizeof(params.mac_address), argv[2]) < 0) ||
	    !net_eth_is_addr_valid(&params.mac_address)) {
		PR_WARNING("Invalid MAC address: %s\n", argv[2]);
		goto err;
	}

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_MAC_ADDRESS, iface, &params, sizeof(params));
	if (ret < 0) {
		if (ret == -EACCES) {
			PR_WARNING("MAC address cannot be set when interface is operational\n");
			goto err;
		}
		PR_WARNING("Failed to set MAC address (%d)\n", ret);
		goto err;
	}

	PR_INFO("MAC address set to %s\n",
		net_sprint_ll_addr(net_if_get_link_addr(iface)->addr,
		net_if_get_link_addr(iface)->len));

	return 0;
err:
	return -ENOEXEC;
#endif /* CONFIG_NET_L2_ETHERNET */
}

struct ctx_info {
	int pos;
	bool are_external_pools;
	struct k_mem_slab *tx_slabs[CONFIG_NET_MAX_CONTEXTS];
	struct net_buf_pool *data_pools[CONFIG_NET_MAX_CONTEXTS];
};

#if defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_NATIVE)
#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
static bool slab_pool_found_already(struct ctx_info *info,
				    struct k_mem_slab *slab,
				    struct net_buf_pool *pool)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_CONTEXTS; i++) {
		if (slab) {
			if (info->tx_slabs[i] == slab) {
				return true;
			}
		} else {
			if (info->data_pools[i] == pool) {
				return true;
			}
		}
	}

	return false;
}
#endif

static void context_info(struct net_context *context, void *user_data)
{
#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct ctx_info *info = data->user_data;
	struct k_mem_slab *slab;
	struct net_buf_pool *pool;

	if (!net_context_is_used(context)) {
		return;
	}

	if (context->tx_slab) {
		slab = context->tx_slab();

		if (slab_pool_found_already(info, slab, NULL)) {
			return;
		}

#if defined(CONFIG_NET_BUF_POOL_USAGE)
		PR("%p\t%u\t%u\tETX\n",
		   slab, slab->info.num_blocks, k_mem_slab_num_free_get(slab));
#else
		PR("%p\t%d\tETX\n", slab, slab->info.num_blocks);
#endif
		info->are_external_pools = true;
		info->tx_slabs[info->pos] = slab;
	}

	if (context->data_pool) {
		pool = context->data_pool();

		if (slab_pool_found_already(info, NULL, pool)) {
			return;
		}

#if defined(CONFIG_NET_BUF_POOL_USAGE)
		PR("%p\t%d\t%ld\tEDATA (%s)\n", pool, pool->buf_count,
		   atomic_get(&pool->avail_count), pool->name);
#else
		PR("%p\t%d\tEDATA\n", pool, pool->buf_count);
#endif
		info->are_external_pools = true;
		info->data_pools[info->pos] = pool;
	}

	info->pos++;
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */
}
#endif /* CONFIG_NET_OFFLOAD || CONFIG_NET_NATIVE */

static int cmd_net_mem(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_NATIVE)
	struct k_mem_slab *rx, *tx;
	struct net_buf_pool *rx_data, *tx_data;

	net_pkt_get_info(&rx, &tx, &rx_data, &tx_data);

#if defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)
	PR("Fragment length %d bytes\n", CONFIG_NET_BUF_DATA_SIZE);
#else
	PR("Fragment data pool size %d bytes\n", CONFIG_NET_BUF_DATA_POOL_SIZE);
#endif /* CONFIG_NET_BUF_FIXED_DATA_SIZE */

	PR("Network buffer pools:\n");

#if defined(CONFIG_NET_BUF_POOL_USAGE)
	PR("Address\t\tTotal\tAvail\tName\n");

	PR("%p\t%d\t%u\tRX\n",
	       rx, rx->info.num_blocks, k_mem_slab_num_free_get(rx));

	PR("%p\t%d\t%u\tTX\n",
	       tx, tx->info.num_blocks, k_mem_slab_num_free_get(tx));

	PR("%p\t%d\t%ld\tRX DATA (%s)\n", rx_data, rx_data->buf_count,
	   atomic_get(&rx_data->avail_count), rx_data->name);

	PR("%p\t%d\t%ld\tTX DATA (%s)\n", tx_data, tx_data->buf_count,
	   atomic_get(&tx_data->avail_count), tx_data->name);
#else
	PR("Address\t\tTotal\tName\n");

	PR("%p\t%d\tRX\n", rx, rx->info.num_blocks);
	PR("%p\t%d\tTX\n", tx, tx->info.num_blocks);
	PR("%p\t%d\tRX DATA\n", rx_data, rx_data->buf_count);
	PR("%p\t%d\tTX DATA\n", tx_data, tx_data->buf_count);
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_BUF_POOL_USAGE", "net_buf allocation");
#endif /* CONFIG_NET_BUF_POOL_USAGE */

	if (IS_ENABLED(CONFIG_NET_CONTEXT_NET_PKT_POOL)) {
		struct net_shell_user_data user_data;
		struct ctx_info info;

		(void)memset(&info, 0, sizeof(info));

		user_data.sh = sh;
		user_data.user_data = &info;

		net_context_foreach(context_info, &user_data);

		if (!info.are_external_pools) {
			PR("No external memory pools found.\n");
		}
	}
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_OFFLOAD or CONFIG_NET_NATIVE", "memory usage");
#endif /* CONFIG_NET_OFFLOAD || CONFIG_NET_NATIVE */

	return 0;
}

static int cmd_net_nbr_rm(const struct shell *sh, size_t argc,
			  char *argv[])
{
#if defined(CONFIG_NET_IPV6)
	struct in6_addr addr;
	int ret;
#endif

#if defined(CONFIG_NET_IPV6)
	if (!argv[1]) {
		PR_WARNING("Neighbor IPv6 address missing.\n");
		return -ENOEXEC;
	}

	ret = net_addr_pton(AF_INET6, argv[1], &addr);
	if (ret < 0) {
		PR_WARNING("Cannot parse '%s'\n", argv[1]);
		return -ENOEXEC;
	}

	if (!net_ipv6_nbr_rm(NULL, &addr)) {
		PR_WARNING("Cannot remove neighbor %s\n",
			   net_sprint_ipv6_addr(&addr));
		return -ENOEXEC;
	} else {
		PR("Neighbor %s removed.\n", net_sprint_ipv6_addr(&addr));
	}
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("IPv6 not enabled.\n");
#endif

	return 0;
}

#if defined(CONFIG_NET_IPV6)
static void nbr_cb(struct net_nbr *nbr, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char *padding = "";
	char *state_pad = "";
	const char *state_str;
#if defined(CONFIG_NET_IPV6_ND)
	int64_t remaining;
#endif

#if defined(CONFIG_NET_L2_IEEE802154)
	padding = "      ";
#endif

	if (*count == 0) {
		PR("     Neighbor  Interface  Flags    State     "
		   "Remain  Link              %sAddress\n", padding);
	}

	(*count)++;

	state_str = net_ipv6_nbr_state2str(net_ipv6_nbr_data(nbr)->state);

	/* This is not a proper way but the minimal libc does not honor
	 * string lengths in %s modifier so in order the output to look
	 * nice, do it like this.
	 */
	if (strlen(state_str) == 5) {
		state_pad = "    ";
	}

#if defined(CONFIG_NET_IPV6_ND)
	remaining = net_ipv6_nbr_data(nbr)->reachable +
		    net_ipv6_nbr_data(nbr)->reachable_timeout -
		    k_uptime_get();
#endif

	PR("[%2d] %p  %d      %5d/%d/%d/%d  %s%s %6d  %17s%s %s\n",
	   *count, nbr, net_if_get_by_iface(nbr->iface),
	   net_ipv6_nbr_data(nbr)->link_metric,
	   nbr->ref,
	   net_ipv6_nbr_data(nbr)->ns_count,
	   net_ipv6_nbr_data(nbr)->is_router,
	   state_str,
	   state_pad,
#if defined(CONFIG_NET_IPV6_ND)
	   (int)(remaining > 0 ? remaining : 0),
#else
	   0,
#endif
	   nbr->idx == NET_NBR_LLADDR_UNKNOWN ? "?" :
	   net_sprint_ll_addr(
		   net_nbr_get_lladdr(nbr->idx)->addr,
		   net_nbr_get_lladdr(nbr->idx)->len),
	   nbr->idx == NET_NBR_LLADDR_UNKNOWN ? "" :
		(net_nbr_get_lladdr(nbr->idx)->len == 8U ? "" : padding),
	   net_sprint_ipv6_addr(&net_ipv6_nbr_data(nbr)->addr));
}
#endif

static int cmd_net_nbr(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_IPV6)
	int count = 0;
	struct net_shell_user_data user_data;
#endif

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_IPV6)
	user_data.sh = sh;
	user_data.user_data = &count;

	net_ipv6_nbr_foreach(nbr_cb, &user_data);

	if (count == 0) {
		PR("No neighbors.\n");
	}
#else
	PR_INFO("IPv6 not enabled.\n");
#endif /* CONFIG_NET_IPV6 */

	return 0;
}

#if defined(CONFIG_NET_IP)

static struct ping_context {
	struct k_work_delayable work;
	struct net_icmp_ctx icmp;
	union {
		struct sockaddr_in addr4;
		struct sockaddr_in6 addr6;
		struct sockaddr addr;
	};
	struct net_if *iface;
	const struct shell *sh;

	/* Ping parameters */
	uint32_t count;
	uint32_t interval;
	uint32_t sequence;
	uint16_t payload_size;
	uint8_t tos;
	int priority;
} ping_ctx;

static void ping_done(struct ping_context *ctx);

#if defined(CONFIG_NET_NATIVE_IPV6)

static int handle_ipv6_echo_reply(struct net_icmp_ctx *ctx,
				  struct net_pkt *pkt,
				  struct net_icmp_ip_hdr *hdr,
				  struct net_icmp_hdr *icmp_hdr,
				  void *user_data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access,
					      struct net_icmpv6_echo_req);
	struct net_ipv6_hdr *ip_hdr = hdr->ipv6;
	struct net_icmpv6_echo_req *icmp_echo;
	uint32_t cycles;
	char time_buf[16] = { 0 };

	icmp_echo = (struct net_icmpv6_echo_req *)net_pkt_get_data(pkt,
								&icmp_access);
	if (icmp_echo == NULL) {
		return -EIO;
	}

	net_pkt_skip(pkt, sizeof(*icmp_echo));

	if (net_pkt_remaining_data(pkt) >= sizeof(uint32_t)) {
		if (net_pkt_read_be32(pkt, &cycles)) {
			return -EIO;
		}

		cycles = k_cycle_get_32() - cycles;

		snprintf(time_buf, sizeof(time_buf),
#ifdef CONFIG_FPU
			 "time=%.2f ms",
			 (double)((uint32_t)k_cyc_to_ns_floor64(cycles) / 1000000.f)
#else
			 "time=%d ms",
			 ((uint32_t)k_cyc_to_ns_floor64(cycles) / 1000000)
#endif
			);
	}

	PR_SHELL(ping_ctx.sh, "%d bytes from %s to %s: icmp_seq=%d ttl=%d "
#ifdef CONFIG_IEEE802154
		 "rssi=%d "
#endif
		 "%s\n",
		 ntohs(ip_hdr->len) - net_pkt_ipv6_ext_len(pkt) -
								NET_ICMPH_LEN,
		 net_sprint_ipv6_addr(&ip_hdr->src),
		 net_sprint_ipv6_addr(&ip_hdr->dst),
		 ntohs(icmp_echo->sequence),
		 ip_hdr->hop_limit,
#ifdef CONFIG_IEEE802154
		 net_pkt_ieee802154_rssi_dbm(pkt),
#endif
		 time_buf);

	if (ntohs(icmp_echo->sequence) == ping_ctx.count) {
		ping_done(&ping_ctx);
	}

	return 0;
}
#else
static int handle_ipv6_echo_reply(struct net_icmp_ctx *ctx,
				  struct net_pkt *pkt,
				  struct net_icmp_ip_hdr *hdr,
				  struct net_icmp_hdr *icmp_hdr,
				  void *user_data)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(pkt);
	ARG_UNUSED(hdr);
	ARG_UNUSED(icmp_hdr);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_NATIVE_IPV4)

static int handle_ipv4_echo_reply(struct net_icmp_ctx *ctx,
				  struct net_pkt *pkt,
				  struct net_icmp_ip_hdr *hdr,
				  struct net_icmp_hdr *icmp_hdr,
				  void *user_data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access,
					      struct net_icmpv4_echo_req);
	struct net_ipv4_hdr *ip_hdr = hdr->ipv4;
	uint32_t cycles;
	struct net_icmpv4_echo_req *icmp_echo;
	char time_buf[16] = { 0 };

	icmp_echo = (struct net_icmpv4_echo_req *)net_pkt_get_data(pkt,
								&icmp_access);
	if (icmp_echo == NULL) {
		return -EIO;
	}

	net_pkt_skip(pkt, sizeof(*icmp_echo));

	if (net_pkt_remaining_data(pkt) >= sizeof(uint32_t)) {
		if (net_pkt_read_be32(pkt, &cycles)) {
			return -EIO;
		}

		cycles = k_cycle_get_32() - cycles;

		snprintf(time_buf, sizeof(time_buf),
#ifdef CONFIG_FPU
			 "time=%.2f ms",
			 (double)((uint32_t)k_cyc_to_ns_floor64(cycles) / 1000000.f)
#else
			 "time=%d ms",
			 ((uint32_t)k_cyc_to_ns_floor64(cycles) / 1000000)
#endif
			);
	}

	PR_SHELL(ping_ctx.sh, "%d bytes from %s to %s: icmp_seq=%d ttl=%d "
		 "%s\n",
		 ntohs(ip_hdr->len) - net_pkt_ipv6_ext_len(pkt) -
								NET_ICMPH_LEN,
		 net_sprint_ipv4_addr(&ip_hdr->src),
		 net_sprint_ipv4_addr(&ip_hdr->dst),
		 ntohs(icmp_echo->sequence),
		 ip_hdr->ttl,
		 time_buf);

	if (ntohs(icmp_echo->sequence) == ping_ctx.count) {
		ping_done(&ping_ctx);
	}

	return 0;
}
#else
static int handle_ipv4_echo_reply(struct net_icmp_ctx *ctx,
				  struct net_pkt *pkt,
				  struct net_icmp_ip_hdr *hdr,
				  struct net_icmp_hdr *icmp_hdr,
				  void *user_data)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(pkt);
	ARG_UNUSED(hdr);
	ARG_UNUSED(icmp_hdr);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_IPV4 */

static int parse_arg(size_t *i, size_t argc, char *argv[])
{
	int res = -1;
	const char *str = argv[*i] + 2;
	char *endptr;

	if (*str == 0) {
		if (*i + 1 >= argc) {
			return -1;
		}

		*i += 1;
		str = argv[*i];
	}

	errno = 0;
	if (strncmp(str, "0x", 2) == 0) {
		res = strtol(str, &endptr, 16);
	} else {
		res = strtol(str, &endptr, 10);
	}

	if (errno || (endptr == str)) {
		return -1;
	}

	return res;
}

static void ping_cleanup(struct ping_context *ctx)
{
	(void)net_icmp_cleanup_ctx(&ctx->icmp);
	shell_set_bypass(ctx->sh, NULL);
}

static void ping_done(struct ping_context *ctx)
{
	k_work_cancel_delayable(&ctx->work);
	ping_cleanup(ctx);
	/* Dummy write to refresh the prompt. */
	shell_fprintf(ctx->sh, SHELL_NORMAL, "");
}

static void ping_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ping_context *ctx =
			CONTAINER_OF(dwork, struct ping_context, work);
	const struct shell *sh = ctx->sh;
	struct net_icmp_ping_params params;
	int ret;

	ctx->sequence++;

	if (ctx->sequence > ctx->count) {
		PR_INFO("Ping timeout\n");
		ping_done(ctx);
		return;
	}

	params.identifier = sys_rand32_get();
	params.sequence = ctx->sequence;
	params.tc_tos = ctx->tos;
	params.priority = ctx->priority;
	params.data = NULL;
	params.data_size = ctx->payload_size;

	ret = net_icmp_send_echo_request(&ctx->icmp,
					 ctx->iface,
					 &ctx->addr,
					 &params,
					 ctx);
	if (ret != 0) {
		PR_WARNING("Failed to send ping, err: %d", ret);
		ping_done(ctx);
		return;
	}

	if (ctx->sequence < ctx->count) {
		k_work_reschedule(&ctx->work, K_MSEC(ctx->interval));
	} else {
		k_work_reschedule(&ctx->work, K_SECONDS(2));
	}
}

#define ASCII_CTRL_C 0x03

static void ping_bypass(const struct shell *sh, uint8_t *data, size_t len)
{
	ARG_UNUSED(sh);

	for (size_t i = 0; i < len; i++) {
		if (data[i] == ASCII_CTRL_C) {
			k_work_cancel_delayable(&ping_ctx.work);
			ping_cleanup(&ping_ctx);
			break;
		}
	}
}

static struct net_if *ping_select_iface(int id, struct sockaddr *target)
{
	struct net_if *iface = net_if_get_by_index(id);

	if (iface != NULL) {
		goto out;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && target->sa_family == AF_INET) {
		iface = net_if_ipv4_select_src_iface(&net_sin(target)->sin_addr);
		if (iface != NULL) {
			goto out;
		}

		iface = net_if_get_default();
		goto out;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && target->sa_family == AF_INET6) {
		struct net_nbr *nbr;
#if defined(CONFIG_NET_ROUTE)
		struct net_route_entry *route;
#endif

		iface = net_if_ipv6_select_src_iface(&net_sin6(target)->sin6_addr);
		if (iface != NULL) {
			goto out;
		}

		nbr = net_ipv6_nbr_lookup(NULL, &net_sin6(target)->sin6_addr);
		if (nbr) {
			iface = nbr->iface;
			goto out;
		}

#if defined(CONFIG_NET_ROUTE)
		route = net_route_lookup(NULL, &net_sin6(target)->sin6_addr);
		if (route) {
			iface = route->iface;
			goto out;
		}
#endif

		iface = net_if_get_default();
	}

out:
	return iface;
}

#endif /* CONFIG_NET_IP */

static int cmd_net_ping(const struct shell *sh, size_t argc, char *argv[])
{
#if !defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return -EOPNOTSUPP;
#else
	char *host = NULL;

	int count = 3;
	int interval = 1000;
	int iface_idx = -1;
	int tos = 0;
	int payload_size = 4;
	int priority = -1;
	int ret;

	for (size_t i = 1; i < argc; ++i) {

		if (*argv[i] != '-') {
			host = argv[i];
			continue;
		}

		switch (argv[i][1]) {
		case 'c':
			count = parse_arg(&i, argc, argv);
			if (count < 0) {
				PR_WARNING("Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}


			break;
		case 'i':
			interval = parse_arg(&i, argc, argv);
			if (interval < 0) {
				PR_WARNING("Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}

			break;

		case 'I':
			iface_idx = parse_arg(&i, argc, argv);
			if (iface_idx < 0 || !net_if_get_by_index(iface_idx)) {
				PR_WARNING("Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}
			break;

		case 'p':
			priority = parse_arg(&i, argc, argv);
			if (priority < 0 || priority > UINT8_MAX) {
				PR_WARNING("Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}
			break;

		case 'Q':
			tos = parse_arg(&i, argc, argv);
			if (tos < 0 || tos > UINT8_MAX) {
				PR_WARNING("Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}

			break;

		case 's':
			payload_size = parse_arg(&i, argc, argv);
			if (payload_size < 0 || payload_size > UINT16_MAX) {
				PR_WARNING("Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}

			break;

		default:
			PR_WARNING("Unrecognized argument: %s\n", argv[i]);
			return -ENOEXEC;
		}
	}

	if (!host) {
		PR_WARNING("Target host missing\n");
		return -ENOEXEC;
	}

	memset(&ping_ctx, 0, sizeof(ping_ctx));

	k_work_init_delayable(&ping_ctx.work, ping_work);

	ping_ctx.sh = sh;
	ping_ctx.count = count;
	ping_ctx.interval = interval;
	ping_ctx.priority = priority;
	ping_ctx.tos = tos;
	ping_ctx.payload_size = payload_size;

	if (IS_ENABLED(CONFIG_NET_IPV6) &&
	    net_addr_pton(AF_INET6, host, &ping_ctx.addr6.sin6_addr) == 0) {
		ping_ctx.addr6.sin6_family = AF_INET6;

		ret = net_icmp_init_ctx(&ping_ctx.icmp, NET_ICMPV6_ECHO_REPLY, 0,
					handle_ipv6_echo_reply);
		if (ret < 0) {
			PR_WARNING("Cannot initialize ICMP context for %s\n", "IPv6");
			return 0;
		}
	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
		   net_addr_pton(AF_INET, host, &ping_ctx.addr4.sin_addr) == 0) {
		ping_ctx.addr4.sin_family = AF_INET;

		ret = net_icmp_init_ctx(&ping_ctx.icmp, NET_ICMPV4_ECHO_REPLY, 0,
					handle_ipv4_echo_reply);
		if (ret < 0) {
			PR_WARNING("Cannot initialize ICMP context for %s\n", "IPv4");
			return 0;
		}
	} else {
		PR_WARNING("Invalid IP address\n");
		return 0;
	}

	ping_ctx.iface = ping_select_iface(iface_idx, &ping_ctx.addr);

	PR("PING %s\n", host);

	shell_set_bypass(sh, ping_bypass);
	k_work_reschedule(&ping_ctx.work, K_NO_WAIT);

	return 0;
#endif
}

static bool is_pkt_part_of_slab(const struct k_mem_slab *slab, const char *ptr)
{
	size_t last_offset = (slab->info.num_blocks - 1) * slab->info.block_size;
	size_t ptr_offset;

	/* Check if pointer fits into slab buffer area. */
	if ((ptr < slab->buffer) || (ptr > slab->buffer + last_offset)) {
		return false;
	}

	/* Check if pointer offset is correct. */
	ptr_offset = ptr - slab->buffer;
	if (ptr_offset % slab->info.block_size != 0) {
		return false;
	}

	return true;
}

struct ctx_pkt_slab_info {
	const void *ptr;
	bool pkt_source_found;
};

static void check_context_pool(struct net_context *context, void *user_data)
{
#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	if (!net_context_is_used(context)) {
		return;
	}

	if (context->tx_slab) {
		struct ctx_pkt_slab_info *info = user_data;
		struct k_mem_slab *slab = context->tx_slab();

		if (is_pkt_part_of_slab(slab, info->ptr)) {
			info->pkt_source_found = true;
		}
	}
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */
}

static bool is_pkt_ptr_valid(const void *ptr)
{
	struct k_mem_slab *rx, *tx;

	net_pkt_get_info(&rx, &tx, NULL, NULL);

	if (is_pkt_part_of_slab(rx, ptr) || is_pkt_part_of_slab(tx, ptr)) {
		return true;
	}

	if (IS_ENABLED(CONFIG_NET_CONTEXT_NET_PKT_POOL)) {
		struct ctx_pkt_slab_info info;

		info.ptr = ptr;
		info.pkt_source_found = false;

		net_context_foreach(check_context_pool, &info);

		if (info.pkt_source_found) {
			return true;
		}
	}

	return false;
}

static struct net_pkt *get_net_pkt(const char *ptr_str)
{
	uint8_t buf[sizeof(intptr_t)];
	intptr_t ptr = 0;
	size_t len;
	int i;

	if (ptr_str[0] == '0' && ptr_str[1] == 'x') {
		ptr_str += 2;
	}

	len = hex2bin(ptr_str, strlen(ptr_str), buf, sizeof(buf));
	if (!len) {
		return NULL;
	}

	for (i = len - 1; i >= 0; i--) {
		ptr |= buf[i] << 8 * (len - 1 - i);
	}

	return (struct net_pkt *)ptr;
}

static void net_pkt_buffer_info(const struct shell *sh, struct net_pkt *pkt)
{
	struct net_buf *buf = pkt->buffer;

	PR("net_pkt %p buffer chain:\n", pkt);
	PR("%p[%ld]", pkt, atomic_get(&pkt->atomic_ref));

	if (buf) {
		PR("->");
	}

	while (buf) {
		PR("%p[%ld/%u (%u/%u)]", buf, atomic_get(&pkt->atomic_ref),
		   buf->len, net_buf_max_len(buf), buf->size);

		buf = buf->frags;
		if (buf) {
			PR("->");
		}
	}

	PR("\n");
}

static void net_pkt_buffer_hexdump(const struct shell *sh,
				   struct net_pkt *pkt)
{
	struct net_buf *buf = pkt->buffer;
	int i = 0;

	if (!buf || buf->ref == 0) {
		return;
	}

	PR("net_pkt %p buffer chain hexdump:\n", pkt);

	while (buf) {
		PR("net_buf[%d] %p\n", i++, buf);
		shell_hexdump(sh, buf->data, buf->len);
		buf = buf->frags;
	}
}

static int cmd_net_pkt(const struct shell *sh, size_t argc, char *argv[])
{
	if (argv[1]) {
		struct net_pkt *pkt;

		pkt = get_net_pkt(argv[1]);
		if (!pkt) {
			PR_ERROR("Invalid ptr value (%s). "
				 "Example: 0x01020304\n", argv[1]);

			return -ENOEXEC;
		}

		if (!is_pkt_ptr_valid(pkt)) {
			PR_ERROR("Pointer is not recognized as net_pkt (%s).\n",
				 argv[1]);

			return -ENOEXEC;
		}

		net_pkt_buffer_info(sh, pkt);
		PR("\n");
		net_pkt_buffer_hexdump(sh, pkt);
	} else {
		PR_INFO("Pointer value must be given.\n");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_net_ppp_ping(const struct shell *sh, size_t argc,
			    char *argv[])
{
#if defined(CONFIG_NET_PPP)
	if (argv[1]) {
		int ret, idx = get_iface_idx(sh, argv[1]);

		if (idx < 0) {
			return -ENOEXEC;
		}

		ret = net_ppp_ping(idx, MSEC_PER_SEC * 1);
		if (ret < 0) {
			if (ret == -EAGAIN) {
				PR_INFO("PPP Echo-Req timeout.\n");
			} else if (ret == -ENODEV || ret == -ENOENT) {
				PR_INFO("Not a PPP interface (%d)\n", idx);
			} else {
				PR_INFO("PPP Echo-Req failed (%d)\n", ret);
			}
		} else {
			if (ret > 1000) {
				PR_INFO("%s%d msec\n",
					"Received PPP Echo-Reply in ",
					ret / 1000);
			} else {
				PR_INFO("%s%d usec\n",
					"Received PPP Echo-Reply in ", ret);
			}
		}
	} else {
		PR_INFO("PPP network interface must be given.\n");
		return -ENOEXEC;
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_L2_PPP", "PPP");
#endif
	return 0;
}

static int cmd_net_ppp_status(const struct shell *sh, size_t argc,
			      char *argv[])
{
#if defined(CONFIG_NET_PPP)
	int idx = 0;
	struct ppp_context *ctx;

	if (argv[1]) {
		idx = get_iface_idx(sh, argv[1]);
		if (idx < 0) {
			return -ENOEXEC;
		}
	}

	ctx = net_ppp_context_get(idx);
	if (!ctx) {
		PR_INFO("PPP context not found.\n");
		return -ENOEXEC;
	}

	PR("PPP phase           : %s (%d)\n", ppp_phase_str(ctx->phase),
								ctx->phase);
	PR("LCP state           : %s (%d)\n",
	   ppp_state_str(ctx->lcp.fsm.state), ctx->lcp.fsm.state);
	PR("LCP retransmits     : %u\n", ctx->lcp.fsm.retransmits);
	PR("LCP NACK loops      : %u\n", ctx->lcp.fsm.nack_loops);
	PR("LCP NACKs recv      : %u\n", ctx->lcp.fsm.recv_nack_loops);
	PR("LCP current id      : %d\n", ctx->lcp.fsm.id);
	PR("LCP ACK received    : %s\n", ctx->lcp.fsm.ack_received ?
								"yes" : "no");

#if defined(CONFIG_NET_IPV4)
	PR("IPCP state          : %s (%d)\n",
	   ppp_state_str(ctx->ipcp.fsm.state), ctx->ipcp.fsm.state);
	PR("IPCP retransmits    : %u\n", ctx->ipcp.fsm.retransmits);
	PR("IPCP NACK loops     : %u\n", ctx->ipcp.fsm.nack_loops);
	PR("IPCP NACKs recv     : %u\n", ctx->ipcp.fsm.recv_nack_loops);
	PR("IPCP current id     : %d\n", ctx->ipcp.fsm.id);
	PR("IPCP ACK received   : %s\n", ctx->ipcp.fsm.ack_received ?
								"yes" : "no");
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_IPV6)
	PR("IPv6CP state        : %s (%d)\n",
	   ppp_state_str(ctx->ipv6cp.fsm.state), ctx->ipv6cp.fsm.state);
	PR("IPv6CP retransmits  : %u\n", ctx->ipv6cp.fsm.retransmits);
	PR("IPv6CP NACK loops   : %u\n", ctx->ipv6cp.fsm.nack_loops);
	PR("IPv6CP NACKs recv   : %u\n", ctx->ipv6cp.fsm.recv_nack_loops);
	PR("IPv6CP current id   : %d\n", ctx->ipv6cp.fsm.id);
	PR("IPv6CP ACK received : %s\n", ctx->ipv6cp.fsm.ack_received ?
								"yes" : "no");
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_L2_PPP_PAP)
	PR("PAP state           : %s (%d)\n",
	   ppp_state_str(ctx->pap.fsm.state), ctx->pap.fsm.state);
	PR("PAP retransmits     : %u\n", ctx->pap.fsm.retransmits);
	PR("PAP NACK loops      : %u\n", ctx->pap.fsm.nack_loops);
	PR("PAP NACKs recv      : %u\n", ctx->pap.fsm.recv_nack_loops);
	PR("PAP current id      : %d\n", ctx->pap.fsm.id);
	PR("PAP ACK received    : %s\n", ctx->pap.fsm.ack_received ?
								"yes" : "no");
#endif /* CONFIG_NET_L2_PPP_PAP */

#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_L2_PPP and CONFIG_NET_PPP", "PPP");
#endif
	return 0;
}

static int cmd_net_route(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_NATIVE)
#if defined(CONFIG_NET_ROUTE) || defined(CONFIG_NET_ROUTE_MCAST)
	struct net_shell_user_data user_data;
#endif

#if defined(CONFIG_NET_ROUTE) || defined(CONFIG_NET_ROUTE_MCAST)
	user_data.sh = sh;
#endif

#if defined(CONFIG_NET_ROUTE)
	net_if_foreach(iface_per_route_cb, &user_data);
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_ROUTE",
		"network route");
#endif

#if defined(CONFIG_NET_ROUTE_MCAST)
	net_if_foreach(iface_per_mcast_route_cb, &user_data);
#endif
#endif
	return 0;
}

static int cmd_net_stacks(const struct shell *sh, size_t argc,
			  char *argv[])
{
#if !defined(CONFIG_KERNEL_SHELL)
	PR("Enable CONFIG_KERNEL_SHELL and type \"kernel stacks\" to see stack information.\n");
#else
	PR("Type \"kernel stacks\" to see stack information.\n");
#endif
	return 0;
}

#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
static void net_shell_print_statistics_all(struct net_shell_user_data *data)
{
	net_if_foreach(net_shell_print_statistics, data);
}
#endif

static int cmd_net_stats_all(const struct shell *sh, size_t argc,
			     char *argv[])
{
#if defined(CONFIG_NET_STATISTICS)
	struct net_shell_user_data user_data;
#endif

#if defined(CONFIG_NET_STATISTICS)
	user_data.sh = sh;

	/* Print global network statistics */
	net_shell_print_statistics_all(&user_data);
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_STATISTICS",
		"statistics");
#endif

	return 0;
}

static int cmd_net_stats_iface(const struct shell *sh, size_t argc,
			       char *argv[])
{
#if defined(CONFIG_NET_STATISTICS)
#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
	struct net_shell_user_data data;
	struct net_if *iface;
	char *endptr;
	int idx;
#endif
#endif

#if defined(CONFIG_NET_STATISTICS)
#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
	if (argv[1] == NULL) {
		PR_WARNING("Network interface index missing!\n");
		return -ENOEXEC;
	}

	idx = strtol(argv[1], &endptr, 10);
	if (*endptr != '\0') {
		PR_WARNING("Invalid index %s\n", argv[1]);
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOEXEC;
	}

	data.sh = sh;

	net_shell_print_statistics(iface, &data);
#else
	PR_INFO("Per network interface statistics not collected.\n");
	PR_INFO("Please enable CONFIG_NET_STATISTICS_PER_INTERFACE\n");
#endif /* CONFIG_NET_STATISTICS_PER_INTERFACE */
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_STATISTICS",
		"statistics");
#endif

	return 0;
}

static int cmd_net_stats(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_STATISTICS)
	if (!argv[1]) {
		cmd_net_stats_all(sh, argc, argv);
		return 0;
	}

	if (strcmp(argv[1], "reset") == 0) {
		net_stats_reset(NULL);
	} else {
		cmd_net_stats_iface(sh, argc, argv);
	}
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_STATISTICS",
		"statistics");
#endif

	return 0;
}

#if defined(CONFIG_NET_TCP) && defined(CONFIG_NET_NATIVE_TCP)
static struct net_context *tcp_ctx;
static const struct shell *tcp_shell;

#define TCP_CONNECT_TIMEOUT K_SECONDS(5) /* ms */
#define TCP_TIMEOUT K_SECONDS(2) /* ms */

static void tcp_connected(struct net_context *context,
			  int status,
			  void *user_data)
{
	if (status < 0) {
		PR_SHELL(tcp_shell, "TCP connection failed (%d)\n", status);

		net_context_put(context);

		tcp_ctx = NULL;
	} else {
		PR_SHELL(tcp_shell, "TCP connected\n");
	}
}

static void get_my_ipv6_addr(struct net_if *iface,
			     struct sockaddr *myaddr)
{
#if defined(CONFIG_NET_IPV6)
	const struct in6_addr *my6addr;

	my6addr = net_if_ipv6_select_src_addr(iface,
					      &net_sin6(myaddr)->sin6_addr);

	memcpy(&net_sin6(myaddr)->sin6_addr, my6addr, sizeof(struct in6_addr));

	net_sin6(myaddr)->sin6_port = 0U; /* let the IP stack to select */
#endif
}

static void get_my_ipv4_addr(struct net_if *iface,
			     struct sockaddr *myaddr)
{
#if defined(CONFIG_NET_NATIVE_IPV4)
	/* Just take the first IPv4 address of an interface. */
	memcpy(&net_sin(myaddr)->sin_addr,
	       &iface->config.ip.ipv4->unicast[0].address.in_addr,
	       sizeof(struct in_addr));

	net_sin(myaddr)->sin_port = 0U; /* let the IP stack to select */
#endif
}

static void print_connect_info(const struct shell *sh,
			       int family,
			       struct sockaddr *myaddr,
			       struct sockaddr *addr)
{
	switch (family) {
	case AF_INET:
		if (IS_ENABLED(CONFIG_NET_IPV4)) {
			PR("Connecting from %s:%u ",
			   net_sprint_ipv4_addr(&net_sin(myaddr)->sin_addr),
			   ntohs(net_sin(myaddr)->sin_port));
			PR("to %s:%u\n",
			   net_sprint_ipv4_addr(&net_sin(addr)->sin_addr),
			   ntohs(net_sin(addr)->sin_port));
		} else {
			PR_INFO("IPv4 not supported\n");
		}

		break;

	case AF_INET6:
		if (IS_ENABLED(CONFIG_NET_IPV6)) {
			PR("Connecting from [%s]:%u ",
			   net_sprint_ipv6_addr(&net_sin6(myaddr)->sin6_addr),
			   ntohs(net_sin6(myaddr)->sin6_port));
			PR("to [%s]:%u\n",
			   net_sprint_ipv6_addr(&net_sin6(addr)->sin6_addr),
			   ntohs(net_sin6(addr)->sin6_port));
		} else {
			PR_INFO("IPv6 not supported\n");
		}

		break;

	default:
		PR_WARNING("Unknown protocol family (%d)\n", family);
		break;
	}
}

static void tcp_connect(const struct shell *sh, char *host, uint16_t port,
			struct net_context **ctx)
{
	struct net_if *iface = net_if_get_default();
	struct sockaddr myaddr;
	struct sockaddr addr;
	struct net_nbr *nbr;
	int addrlen;
	int family;
	int ret;

	if (IS_ENABLED(CONFIG_NET_IPV6) && !IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = net_addr_pton(AF_INET6, host,
				    &net_sin6(&addr)->sin6_addr);
		if (ret < 0) {
			PR_WARNING("Invalid IPv6 address\n");
			return;
		}

		net_sin6(&addr)->sin6_port = htons(port);
		addrlen = sizeof(struct sockaddr_in6);

		nbr = net_ipv6_nbr_lookup(NULL, &net_sin6(&addr)->sin6_addr);
		if (nbr) {
			iface = nbr->iface;
		}

		get_my_ipv6_addr(iface, &myaddr);
		family = addr.sa_family = myaddr.sa_family = AF_INET6;

	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
		   !IS_ENABLED(CONFIG_NET_IPV6)) {
		ARG_UNUSED(nbr);

		ret = net_addr_pton(AF_INET, host, &net_sin(&addr)->sin_addr);
		if (ret < 0) {
			PR_WARNING("Invalid IPv4 address\n");
			return;
		}

		get_my_ipv4_addr(iface, &myaddr);
		net_sin(&addr)->sin_port = htons(port);
		addrlen = sizeof(struct sockaddr_in);
		family = addr.sa_family = myaddr.sa_family = AF_INET;
	} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
		   IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = net_addr_pton(AF_INET6, host,
				    &net_sin6(&addr)->sin6_addr);
		if (ret < 0) {
			ret = net_addr_pton(AF_INET, host,
					    &net_sin(&addr)->sin_addr);
			if (ret < 0) {
				PR_WARNING("Invalid IP address\n");
				return;
			}

			net_sin(&addr)->sin_port = htons(port);
			addrlen = sizeof(struct sockaddr_in);

			get_my_ipv4_addr(iface, &myaddr);
			family = addr.sa_family = myaddr.sa_family = AF_INET;
		} else {
			net_sin6(&addr)->sin6_port = htons(port);
			addrlen = sizeof(struct sockaddr_in6);

			nbr = net_ipv6_nbr_lookup(NULL,
						  &net_sin6(&addr)->sin6_addr);
			if (nbr) {
				iface = nbr->iface;
			}

			get_my_ipv6_addr(iface, &myaddr);
			family = addr.sa_family = myaddr.sa_family = AF_INET6;
		}
	} else {
		PR_WARNING("No IPv6 nor IPv4 is enabled\n");
		return;
	}

	print_connect_info(sh, family, &myaddr, &addr);

	ret = net_context_get(family, SOCK_STREAM, IPPROTO_TCP, ctx);
	if (ret < 0) {
		PR_WARNING("Cannot get TCP context (%d)\n", ret);
		return;
	}

	ret = net_context_bind(*ctx, &myaddr, addrlen);
	if (ret < 0) {
		PR_WARNING("Cannot bind TCP (%d)\n", ret);
		return;
	}

	/* Note that we cannot put shell as a user_data when connecting
	 * because the tcp_connected() will be called much later and
	 * all local stack variables are lost at that point.
	 */
	tcp_shell = sh;

#if defined(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT)
#define CONNECT_TIMEOUT K_MSEC(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT)
#else
#define CONNECT_TIMEOUT K_SECONDS(3)
#endif

	net_context_connect(*ctx, &addr, addrlen, tcp_connected,
			    CONNECT_TIMEOUT, NULL);
}

static void tcp_sent_cb(struct net_context *context,
			int status, void *user_data)
{
	PR_SHELL(tcp_shell, "Message sent\n");
}

static void tcp_recv_cb(struct net_context *context, struct net_pkt *pkt,
			union net_ip_header *ip_hdr,
			union net_proto_header *proto_hdr,
			int status, void *user_data)
{
	int ret, len;

	if (pkt == NULL) {
		if (!tcp_ctx || !net_context_is_used(tcp_ctx)) {
			return;
		}

		ret = net_context_put(tcp_ctx);
		if (ret < 0) {
			PR_SHELL(tcp_shell,
				 "Cannot close the connection (%d)\n", ret);
			return;
		}

		PR_SHELL(tcp_shell, "Connection closed by remote peer.\n");
		tcp_ctx = NULL;

		return;
	}

	len = net_pkt_remaining_data(pkt);

	(void)net_context_update_recv_wnd(context, len);

	PR_SHELL(tcp_shell, "%zu bytes received\n", net_pkt_get_len(pkt));

	net_pkt_unref(pkt);
}
#endif

static int cmd_net_tcp_connect(const struct shell *sh, size_t argc,
			       char *argv[])
{
#if defined(CONFIG_NET_TCP) && defined(CONFIG_NET_NATIVE_TCP)
	int arg = 0;

	/* tcp connect <ip> port */
	char *endptr;
	char *ip;
	uint16_t port;

	/* tcp connect <ip> port */
	if (tcp_ctx && net_context_is_used(tcp_ctx)) {
		PR("Already connected\n");
		return -ENOEXEC;
	}

	if (!argv[++arg]) {
		PR_WARNING("Peer IP address missing.\n");
		return -ENOEXEC;
	}

	ip = argv[arg];

	if (!argv[++arg]) {
		PR_WARNING("Peer port missing.\n");
		return -ENOEXEC;
	}

	port = strtol(argv[arg], &endptr, 10);
	if (*endptr != '\0') {
		PR_WARNING("Invalid port %s\n", argv[arg]);
		return -ENOEXEC;
	}

	tcp_connect(sh, ip, port, &tcp_ctx);
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_TCP and CONFIG_NET_NATIVE", "TCP");
#endif /* CONFIG_NET_NATIVE_TCP */

	return 0;
}

static int cmd_net_tcp_send(const struct shell *sh, size_t argc,
			    char *argv[])
{
#if defined(CONFIG_NET_TCP) && defined(CONFIG_NET_NATIVE_TCP)
	int arg = 0;
	int ret;
	struct net_shell_user_data user_data;

	/* tcp send <data> */
	if (!tcp_ctx || !net_context_is_used(tcp_ctx)) {
		PR_WARNING("Not connected\n");
		return -ENOEXEC;
	}

	if (!argv[++arg]) {
		PR_WARNING("No data to send.\n");
		return -ENOEXEC;
	}

	user_data.sh = sh;

	ret = net_context_send(tcp_ctx, (uint8_t *)argv[arg],
			       strlen(argv[arg]), tcp_sent_cb,
			       TCP_TIMEOUT, &user_data);
	if (ret < 0) {
		PR_WARNING("Cannot send msg (%d)\n", ret);
		return -ENOEXEC;
	}

#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_TCP and CONFIG_NET_NATIVE", "TCP");
#endif /* CONFIG_NET_NATIVE_TCP */

	return 0;
}

static int cmd_net_tcp_recv(const struct shell *sh, size_t argc,
			    char *argv[])
{
#if defined(CONFIG_NET_TCP) && defined(CONFIG_NET_NATIVE_TCP)
	int ret;
	struct net_shell_user_data user_data;

	/* tcp recv */
	if (!tcp_ctx || !net_context_is_used(tcp_ctx)) {
		PR_WARNING("Not connected\n");
		return -ENOEXEC;
	}

	user_data.sh = sh;

	ret = net_context_recv(tcp_ctx, tcp_recv_cb, K_NO_WAIT, &user_data);
	if (ret < 0) {
		PR_WARNING("Cannot recv data (%d)\n", ret);
		return -ENOEXEC;
	}

#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_TCP and CONFIG_NET_NATIVE", "TCP");
#endif /* CONFIG_NET_NATIVE_TCP */

	return 0;
}

static int cmd_net_tcp_close(const struct shell *sh, size_t argc,
			     char *argv[])
{
#if defined(CONFIG_NET_TCP) && defined(CONFIG_NET_NATIVE_TCP)
	int ret;

	/* tcp close */
	if (!tcp_ctx || !net_context_is_used(tcp_ctx)) {
		PR_WARNING("Not connected\n");
		return -ENOEXEC;
	}

	ret = net_context_put(tcp_ctx);
	if (ret < 0) {
		PR_WARNING("Cannot close the connection (%d)\n", ret);
		return -ENOEXEC;
	}

	PR("Connection closed.\n");
	tcp_ctx = NULL;
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_TCP and CONFIG_NET_NATIVE", "TCP");
#endif /* CONFIG_NET_TCP */

	return 0;
}

static int cmd_net_tcp(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return 0;
}

#if defined(CONFIG_NET_UDP) && defined(CONFIG_NET_NATIVE_UDP)
static struct net_context *udp_ctx;
static const struct shell *udp_shell;
K_SEM_DEFINE(udp_send_wait, 0, 1);

static void udp_rcvd(struct net_context *context, struct net_pkt *pkt,
		     union net_ip_header *ip_hdr,
		     union net_proto_header *proto_hdr, int status,
		     void *user_data)
{
	if (pkt) {
		size_t len = net_pkt_remaining_data(pkt);
		uint8_t byte;

		PR_SHELL(udp_shell, "Received UDP packet: ");
		for (size_t i = 0; i < len; ++i) {
			net_pkt_read_u8(pkt, &byte);
			PR_SHELL(udp_shell, "%02x ", byte);
		}
		PR_SHELL(udp_shell, "\n");

		net_pkt_unref(pkt);
	}
}

static void udp_sent(struct net_context *context, int status, void *user_data)
{
	ARG_UNUSED(context);
	ARG_UNUSED(status);
	ARG_UNUSED(user_data);

	PR_SHELL(udp_shell, "Message sent\n");
	k_sem_give(&udp_send_wait);
}
#endif

static int cmd_net_udp_bind(const struct shell *sh, size_t argc,
			    char *argv[])
{
#if !defined(CONFIG_NET_UDP) || !defined(CONFIG_NET_NATIVE_UDP)
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return -EOPNOTSUPP;
#else
	char *addr_str = NULL;
	char *endptr = NULL;
	uint16_t port;
	int ret;

	struct net_if *iface;
	struct sockaddr addr;
	int addrlen;

	if (argc < 3) {
		PR_WARNING("Not enough arguments given for udp bind command\n");
		return -EINVAL;
	}

	addr_str = argv[1];
	port = strtol(argv[2], &endptr, 0);

	if (endptr == argv[2]) {
		PR_WARNING("Invalid port number\n");
		return -EINVAL;
	}

	if (udp_ctx && net_context_is_used(udp_ctx)) {
		PR_WARNING("Network context already in use\n");
		return -EALREADY;
	}

	memset(&addr, 0, sizeof(addr));

	ret = net_ipaddr_parse(addr_str, strlen(addr_str), &addr);
	if (ret < 0) {
		PR_WARNING("Cannot parse address \"%s\"\n", addr_str);
		return ret;
	}

	ret = net_context_get(addr.sa_family, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_ctx);
	if (ret < 0) {
		PR_WARNING("Cannot get UDP context (%d)\n", ret);
		return ret;
	}

	udp_shell = sh;

	if (IS_ENABLED(CONFIG_NET_IPV6) && addr.sa_family == AF_INET6) {
		net_sin6(&addr)->sin6_port = htons(port);
		addrlen = sizeof(struct sockaddr_in6);

		iface = net_if_ipv6_select_src_iface(
				&net_sin6(&addr)->sin6_addr);
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && addr.sa_family == AF_INET) {
		net_sin(&addr)->sin_port = htons(port);
		addrlen = sizeof(struct sockaddr_in);

		iface = net_if_ipv4_select_src_iface(
				&net_sin(&addr)->sin_addr);
	} else {
		PR_WARNING("IPv6 and IPv4 are disabled, cannot %s.\n", "bind");
		goto release_ctx;
	}

	if (!iface) {
		PR_WARNING("No interface to send to given host\n");
		goto release_ctx;
	}

	net_context_set_iface(udp_ctx, iface);

	ret = net_context_bind(udp_ctx, &addr, addrlen);
	if (ret < 0) {
		PR_WARNING("Binding to UDP port failed (%d)\n", ret);
		goto release_ctx;
	}

	ret = net_context_recv(udp_ctx, udp_rcvd, K_NO_WAIT, NULL);
	if (ret < 0) {
		PR_WARNING("Receiving from UDP port failed (%d)\n", ret);
		goto release_ctx;
	}

	return 0;

release_ctx:
	ret = net_context_put(udp_ctx);
	if (ret < 0) {
		PR_WARNING("Cannot put UDP context (%d)\n", ret);
	}

	return 0;
#endif
}

static int cmd_net_udp_close(const struct shell *sh, size_t argc,
			     char *argv[])
{
#if !defined(CONFIG_NET_UDP) || !defined(CONFIG_NET_NATIVE_UDP)
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return -EOPNOTSUPP;
#else
	int ret;

	if (!udp_ctx || !net_context_is_used(udp_ctx)) {
		PR_WARNING("Network context is not used. Cannot close.\n");
		return -EINVAL;
	}

	ret = net_context_put(udp_ctx);
	if (ret < 0) {
		PR_WARNING("Cannot close UDP port (%d)\n", ret);
	}

	return 0;
#endif
}

static int cmd_net_udp_send(const struct shell *sh, size_t argc,
			    char *argv[])
{
#if !defined(CONFIG_NET_UDP) || !defined(CONFIG_NET_NATIVE_UDP)
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return -EOPNOTSUPP;
#else
	char *host = NULL;
	char *endptr = NULL;
	uint16_t port;
	uint8_t *payload = NULL;
	int ret;

	struct net_if *iface;
	struct sockaddr addr;
	int addrlen;

	if (argc < 4) {
		PR_WARNING("Not enough arguments given for udp send command\n");
		return -EINVAL;
	}

	host = argv[1];
	port = strtol(argv[2], &endptr, 0);
	payload = argv[3];

	if (endptr == argv[2]) {
		PR_WARNING("Invalid port number\n");
		return -EINVAL;
	}

	if (udp_ctx && net_context_is_used(udp_ctx)) {
		PR_WARNING("Network context already in use\n");
		return -EALREADY;
	}

	memset(&addr, 0, sizeof(addr));
	ret = net_ipaddr_parse(host, strlen(host), &addr);
	if (ret < 0) {
		PR_WARNING("Cannot parse address \"%s\"\n", host);
		return ret;
	}

	ret = net_context_get(addr.sa_family, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_ctx);
	if (ret < 0) {
		PR_WARNING("Cannot get UDP context (%d)\n", ret);
		return ret;
	}

	udp_shell = sh;

	if (IS_ENABLED(CONFIG_NET_IPV6) && addr.sa_family == AF_INET6) {
		net_sin6(&addr)->sin6_port = htons(port);
		addrlen = sizeof(struct sockaddr_in6);

		iface = net_if_ipv6_select_src_iface(
				&net_sin6(&addr)->sin6_addr);
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && addr.sa_family == AF_INET) {
		net_sin(&addr)->sin_port = htons(port);
		addrlen = sizeof(struct sockaddr_in);

		iface = net_if_ipv4_select_src_iface(
				&net_sin(&addr)->sin_addr);
	} else {
		PR_WARNING("IPv6 and IPv4 are disabled, cannot %s.\n", "send");
		goto release_ctx;
	}

	if (!iface) {
		PR_WARNING("No interface to send to given host\n");
		goto release_ctx;
	}

	net_context_set_iface(udp_ctx, iface);

	ret = net_context_recv(udp_ctx, udp_rcvd, K_NO_WAIT, NULL);
	if (ret < 0) {
		PR_WARNING("Setting rcv callback failed (%d)\n", ret);
		goto release_ctx;
	}

	ret = net_context_sendto(udp_ctx, payload, strlen(payload), &addr,
				 addrlen, udp_sent, K_FOREVER, NULL);
	if (ret < 0) {
		PR_WARNING("Sending packet failed (%d)\n", ret);
		goto release_ctx;
	}

	ret = k_sem_take(&udp_send_wait, K_SECONDS(2));
	if (ret == -EAGAIN) {
		PR_WARNING("UDP packet sending failed\n");
	}

release_ctx:
	ret = net_context_put(udp_ctx);
	if (ret < 0) {
		PR_WARNING("Cannot put UDP context (%d)\n", ret);
	}

	return 0;
#endif
}

static int cmd_net_udp(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return 0;
}

#if defined(CONFIG_NET_L2_VIRTUAL)
static void virtual_iface_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char *name, buf[CONFIG_NET_L2_VIRTUAL_MAX_NAME_LEN];
	struct net_if *orig_iface;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return;
	}

	if (*count == 0) {
		PR("Interface  Attached-To  Description\n");
		(*count)++;
	}

	orig_iface = net_virtual_get_iface(iface);

	name = net_virtual_get_name(iface, buf, sizeof(buf));

	PR("%d          %c            %s\n",
	   net_if_get_by_iface(iface),
	   orig_iface ? net_if_get_by_iface(orig_iface) + '0' : '-',
	   name);

	(*count)++;
}

static void attached_iface_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char buf[CONFIG_NET_L2_VIRTUAL_MAX_NAME_LEN];
	const char *name;
	struct virtual_interface_context *ctx, *tmp;

	if (sys_slist_is_empty(&iface->config.virtual_interfaces)) {
		return;
	}

	if (*count == 0) {
		PR("Interface  Below-of  Description\n");
		(*count)++;
	}

	PR("%d          ", net_if_get_by_iface(iface));

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&iface->config.virtual_interfaces,
					  ctx, tmp, node) {
		if (ctx->virtual_iface == iface) {
			continue;
		}

		PR("%d ", net_if_get_by_iface(ctx->virtual_iface));
	}

	name = net_virtual_get_name(iface, buf, sizeof(buf));
	if (name == NULL) {
		name = iface2str(iface, NULL);
	}

	PR("        %s\n", name);

	(*count)++;
}
#endif /* CONFIG_NET_L2_VIRTUAL */

static int cmd_net_virtual(const struct shell *sh, size_t argc,
			   char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_L2_VIRTUAL)
	struct net_shell_user_data user_data;
	int count = 0;

	user_data.sh = sh;
	user_data.user_data = &count;

	net_if_foreach(virtual_iface_cb, &user_data);

	count = 0;
	PR("\n");

	net_if_foreach(attached_iface_cb, &user_data);
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_L2_VIRTUAL",
		"virtual network interface");
#endif
	return 0;
}

#if defined(CONFIG_NET_VLAN)
static void iface_vlan_del_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	uint16_t vlan_tag = POINTER_TO_UINT(data->user_data);
	int ret;

	ret = net_eth_vlan_disable(iface, vlan_tag);
	if (ret < 0) {
		if (ret != -ESRCH) {
			PR_WARNING("Cannot delete VLAN tag %d from "
				   "interface %d (%p)\n",
				   vlan_tag,
				   net_if_get_by_iface(iface),
				   iface);
		}

		return;
	}

	PR("VLAN tag %d removed from interface %d (%p)\n", vlan_tag,
	   net_if_get_by_iface(iface), iface);
}

static void iface_vlan_cb(struct net_if *iface, void *user_data)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	int i;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (*count == 0) {
		PR("    Interface  Type     Tag\n");
	}

	if (!ctx->vlan_enabled) {
		PR_WARNING("VLAN tag(s) not set\n");
		return;
	}

	for (i = 0; i < NET_VLAN_MAX_COUNT; i++) {
		if (!ctx->vlan[i].iface || ctx->vlan[i].iface != iface) {
			continue;
		}

		if (ctx->vlan[i].tag == NET_VLAN_TAG_UNSPEC) {
			continue;
		}

		PR("[%d] %p %s %d\n", net_if_get_by_iface(iface), iface,
		   iface2str(iface, NULL), ctx->vlan[i].tag);

		break;
	}

	(*count)++;
}
#endif /* CONFIG_NET_VLAN */

static int cmd_net_vlan(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_VLAN)
	struct net_shell_user_data user_data;
	int count;
#endif

#if defined(CONFIG_NET_VLAN)
	count = 0;

	user_data.sh = sh;
	user_data.user_data = &count;

	net_if_foreach(iface_vlan_cb, &user_data);
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_VLAN", "VLAN");
#endif /* CONFIG_NET_VLAN */

	return 0;
}

static int cmd_net_vlan_add(const struct shell *sh, size_t argc,
			    char *argv[])
{
#if defined(CONFIG_NET_VLAN)
	int arg = 0;
	int ret;
	uint16_t tag;
	struct net_if *iface;
	char *endptr;
	uint32_t iface_idx;
#endif

#if defined(CONFIG_NET_VLAN)
	/* vlan add <tag> <interface index> */
	if (!argv[++arg]) {
		PR_WARNING("VLAN tag missing.\n");
		goto usage;
	}

	tag = strtol(argv[arg], &endptr, 10);
	if (*endptr != '\0') {
		PR_WARNING("Invalid tag %s\n", argv[arg]);
		return -ENOEXEC;
	}

	if (!argv[++arg]) {
		PR_WARNING("Network interface index missing.\n");
		goto usage;
	}

	iface_idx = strtol(argv[arg], &endptr, 10);
	if (*endptr != '\0') {
		PR_WARNING("Invalid index %s\n", argv[arg]);
		goto usage;
	}

	iface = net_if_get_by_index(iface_idx);
	if (!iface) {
		PR_WARNING("Network interface index %d is invalid.\n",
			   iface_idx);
		goto usage;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		PR_WARNING("Network interface %d (%p) is not ethernet interface\n",
			   net_if_get_by_iface(iface), iface);
		return -ENOEXEC;
	}

	ret = net_eth_vlan_enable(iface, tag);
	if (ret < 0) {
		if (ret == -ENOENT) {
			PR_WARNING("No IP address configured.\n");
		}

		PR_WARNING("Cannot set VLAN tag (%d)\n", ret);

		return -ENOEXEC;
	}

	PR("VLAN tag %d set to interface %d (%p)\n", tag,
	   net_if_get_by_iface(iface), iface);

	return 0;

usage:
	PR("Usage:\n");
	PR("\tvlan add <tag> <interface index>\n");
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_VLAN", "VLAN");
#endif /* CONFIG_NET_VLAN */

	return 0;
}

static int cmd_net_vlan_del(const struct shell *sh, size_t argc,
			    char *argv[])
{
#if defined(CONFIG_NET_VLAN)
	int arg = 0;
	struct net_shell_user_data user_data;
	char *endptr;
	uint16_t tag;
#endif

#if defined(CONFIG_NET_VLAN)
	/* vlan del <tag> */
	if (!argv[++arg]) {
		PR_WARNING("VLAN tag missing.\n");
		goto usage;
	}

	tag = strtol(argv[arg], &endptr, 10);
	if (*endptr != '\0') {
		PR_WARNING("Invalid tag %s\n", argv[arg]);
		return -ENOEXEC;
	}

	user_data.sh = sh;
	user_data.user_data = UINT_TO_POINTER((uint32_t)tag);

	net_if_foreach(iface_vlan_del_cb, &user_data);

	return 0;

usage:
	PR("Usage:\n");
	PR("\tvlan del <tag>\n");
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_VLAN", "VLAN");
#endif /* CONFIG_NET_VLAN */

	return 0;
}

static int cmd_net_suspend(const struct shell *sh, size_t argc,
			   char *argv[])
{
#if defined(CONFIG_NET_POWER_MANAGEMENT)
	if (argv[1]) {
		struct net_if *iface = NULL;
		const struct device *dev;
		int idx;
		int ret;

		idx = get_iface_idx(sh, argv[1]);
		if (idx < 0) {
			return -ENOEXEC;
		}

		iface = net_if_get_by_index(idx);
		if (!iface) {
			PR_WARNING("No such interface in index %d\n", idx);
			return -ENOEXEC;
		}

		dev = net_if_get_device(iface);

		ret = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
		if (ret != 0) {
			PR_INFO("Iface could not be suspended: ");

			if (ret == -EBUSY) {
				PR_INFO("device is busy\n");
			} else if (ret == -EALREADY) {
				PR_INFO("dehive is already suspended\n");
			}
		}
	} else {
		PR("Usage:\n");
		PR("\tsuspend <iface index>\n");
	}
#else
	PR_INFO("You need a network driver supporting Power Management.\n");
#endif /* CONFIG_NET_POWER_MANAGEMENT */

	return 0;
}

static int cmd_net_resume(const struct shell *sh, size_t argc,
			  char *argv[])
{
#if defined(CONFIG_NET_POWER_MANAGEMENT)
	if (argv[1]) {
		struct net_if *iface = NULL;
		const struct device *dev;
		int idx;
		int ret;

		idx = get_iface_idx(sh, argv[1]);
		if (idx < 0) {
			return -ENOEXEC;
		}

		iface = net_if_get_by_index(idx);
		if (!iface) {
			PR_WARNING("No such interface in index %d\n", idx);
			return -ENOEXEC;
		}

		dev = net_if_get_device(iface);

		ret = pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);
		if (ret != 0) {
			PR_INFO("Iface could not be resumed\n");
		}

	} else {
		PR("Usage:\n");
		PR("\tresume <iface index>\n");
	}
#else
	PR_INFO("You need a network driver supporting Power Management.\n");
#endif /* CONFIG_NET_POWER_MANAGEMENT */

	return 0;
}

#if defined(CONFIG_WEBSOCKET_CLIENT)
static void websocket_context_cb(struct websocket_context *context,
				 void *user_data)
{
#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
#define ADDR_LEN NET_IPV6_ADDR_LEN
#elif defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
#define ADDR_LEN NET_IPV4_ADDR_LEN
#else
#define ADDR_LEN NET_IPV6_ADDR_LEN
#endif
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct net_context *net_ctx;
	int *count = data->user_data;
	/* +7 for []:port */
	char addr_local[ADDR_LEN + 7];
	char addr_remote[ADDR_LEN + 7] = "";

	net_ctx = z_get_fd_obj(context->real_sock, NULL, 0);
	if (net_ctx == NULL) {
		PR_ERROR("Invalid fd %d", context->real_sock);
		return;
	}

	get_addresses(net_ctx, addr_local, sizeof(addr_local),
		      addr_remote, sizeof(addr_remote));

	PR("[%2d] %p/%p\t%p   %16s\t%16s\n",
	   (*count) + 1, context, net_ctx,
	   net_context_get_iface(net_ctx),
	   addr_local, addr_remote);

	(*count)++;
}
#endif /* CONFIG_WEBSOCKET_CLIENT */

static int cmd_net_websocket(const struct shell *sh, size_t argc,
			     char *argv[])
{
#if defined(CONFIG_WEBSOCKET_CLIENT)
	struct net_shell_user_data user_data;
	int count = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR("     websocket/net_ctx\tIface         "
	   "Local              \tRemote\n");

	user_data.sh = sh;
	user_data.user_data = &count;

	websocket_context_foreach(websocket_context_cb, &user_data);

	if (count == 0) {
		PR("No connections\n");
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_WEBSOCKET_CLIENT",
		"Websocket");
#endif /* CONFIG_WEBSOCKET_CLIENT */

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_arp,
	SHELL_CMD(flush, NULL, "Remove all entries from ARP cache.",
		  cmd_net_arp_flush),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_capture,
	SHELL_CMD(setup, NULL, "Setup network packet capture.\n"
		  "'net capture setup <remote-ip-addr> <local-addr> <peer-addr>'\n"
		  "<remote> is the (outer) endpoint IP address,\n"
		  "<local> is the (inner) local IP address,\n"
		  "<peer> is the (inner) peer IP address\n"
		  "Local and Peer addresses can have UDP port number in them (optional)\n"
		  "like 198.0.51.2:9000 or [2001:db8:100::2]:4242",
		  cmd_net_capture_setup),
	SHELL_CMD(cleanup, NULL, "Cleanup network packet capture.",
		  cmd_net_capture_cleanup),
	SHELL_CMD(enable, NULL, "Enable network packet capture for a given "
		  "network interface.\n"
		  "'net capture enable <interface index>'",
		  cmd_net_capture_enable),
	SHELL_CMD(disable, NULL, "Disable network packet capture.",
		  cmd_net_capture_disable),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_dns,
	SHELL_CMD(cancel, NULL, "Cancel all pending requests.",
		  cmd_net_dns_cancel),
	SHELL_CMD(query, NULL,
		  "'net dns <hostname> [A or AAAA]' queries IPv4 address "
		  "(default) or IPv6 address for a host name.",
		  cmd_net_dns_query),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_events,
	SHELL_CMD(on, NULL, "Turn on network event monitoring.",
		  cmd_net_events_on),
	SHELL_CMD(off, NULL, "Turn off network event monitoring.",
		  cmd_net_events_off),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_gptp,
	SHELL_CMD(port, NULL,
		  "'net gptp [<port>]' prints detailed information about "
		  "gPTP port.",
		  cmd_net_gptp_port),
	SHELL_SUBCMD_SET_END
);

#if !defined(NET_VLAN_MAX_COUNT)
#define MAX_IFACE_COUNT NET_IF_MAX_CONFIGS
#else
#define MAX_IFACE_COUNT NET_VLAN_MAX_COUNT
#endif

#if defined(CONFIG_NET_SHELL_DYN_CMD_COMPLETION)

#define MAX_IFACE_HELP_STR_LEN sizeof("longbearername (0xabcd0123)")
#define MAX_IFACE_STR_LEN sizeof("xxx")

static char iface_help_buffer[MAX_IFACE_COUNT][MAX_IFACE_HELP_STR_LEN];
static char iface_index_buffer[MAX_IFACE_COUNT][MAX_IFACE_STR_LEN];

static char *set_iface_index_buffer(size_t idx)
{
	struct net_if *iface = net_if_get_by_index(idx);

	if (!iface) {
		return NULL;
	}

	snprintk(iface_index_buffer[idx], MAX_IFACE_STR_LEN, "%zu", idx);

	return iface_index_buffer[idx];
}

static char *set_iface_index_help(size_t idx)
{
	struct net_if *iface = net_if_get_by_index(idx);

	if (!iface) {
		return NULL;
	}

	snprintk(iface_help_buffer[idx], MAX_IFACE_HELP_STR_LEN,
		 "%s (%p)", iface2str(iface, NULL), iface);

	return iface_help_buffer[idx];
}

static void iface_index_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(iface_index, iface_index_get);

static void iface_index_get(size_t idx, struct shell_static_entry *entry)
{
	entry->handler = NULL;
	entry->help  = set_iface_index_help(idx);
	entry->subcmd = &iface_index;
	entry->syntax = set_iface_index_buffer(idx);
}

#define IFACE_DYN_CMD &iface_index

#if defined(CONFIG_NET_PPP)
static char *set_iface_ppp_index_buffer(size_t idx)
{
	struct net_if *iface = net_if_get_by_index(idx);

	if (!iface) {
		return NULL;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(PPP)) {
		return NULL;
	}

	snprintk(iface_index_buffer[idx], MAX_IFACE_STR_LEN, "%zu", idx);

	return iface_index_buffer[idx];
}

static char *set_iface_ppp_index_help(size_t idx)
{
	struct net_if *iface = net_if_get_by_index(idx);

	if (!iface) {
		return NULL;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(PPP)) {
		return NULL;
	}

	snprintk(iface_help_buffer[idx], MAX_IFACE_HELP_STR_LEN,
		 "%s (%p)", iface2str(iface, NULL), iface);

	return iface_help_buffer[idx];
}

static void iface_ppp_index_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(iface_ppp_index, iface_ppp_index_get);

static void iface_ppp_index_get(size_t idx, struct shell_static_entry *entry)
{
	entry->handler = NULL;
	entry->help  = set_iface_ppp_index_help(idx);
	entry->subcmd = &iface_ppp_index;
	entry->syntax = set_iface_ppp_index_buffer(idx);
}

#define IFACE_PPP_DYN_CMD &iface_ppp_index
#else
#define IFACE_PPP_DYN_CMD NULL
#endif /* CONFIG_NET_PPP */

#else
#define IFACE_DYN_CMD NULL
#define IFACE_PPP_DYN_CMD NULL
#endif /* CONFIG_NET_SHELL_DYN_CMD_COMPLETION */

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_iface,
	SHELL_CMD(up, IFACE_DYN_CMD,
		  "'net iface up <index>' takes network interface up.",
		  cmd_net_iface_up),
	SHELL_CMD(down, IFACE_DYN_CMD,
		  "'net iface down <index>' takes network interface "
		  "down.",
		  cmd_net_iface_down),
	SHELL_CMD(show, IFACE_DYN_CMD,
		  "'net iface <index>' shows network interface "
		  "information.",
		  cmd_net_iface),
	SHELL_CMD(set_mac, IFACE_DYN_CMD,
		  "'net iface set_mac <index> <MAC>' sets MAC address for the network interface.",
		  cmd_net_set_mac),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_ip,
	SHELL_CMD(add, NULL,
		  "'net ipv4 add <index> <address>' adds the address to the interface.",
		  cmd_net_ip_add),
	SHELL_CMD(del, NULL,
		  "'net ipv4 del <index> <address>' deletes the address from the interface.",
		  cmd_net_ip_del),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_ip6,
	SHELL_CMD(add, NULL,
		  "'net ipv6 add <index> <address>' adds the address to the interface.",
		  cmd_net_ip6_add),
	SHELL_CMD(del, NULL,
		  "'net ipv6 del <index> <address>' deletes the address from the interface.",
		  cmd_net_ip6_del),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_route,
	SHELL_CMD(add, NULL,
		  "'net route add <index> <destination> <gateway>'"
		  " adds the route to the destination.",
		  cmd_net_ip6_route_add),
	SHELL_CMD(del, NULL,
		  "'net route del <index> <destination>'"
		  " deletes the route to the destination.",
		  cmd_net_ip6_route_del),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_ppp,
	SHELL_CMD(ping, IFACE_PPP_DYN_CMD,
		  "'net ppp ping <index>' sends Echo-request to PPP interface.",
		  cmd_net_ppp_ping),
	SHELL_CMD(status, NULL,
		  "'net ppp status' prints information about PPP.",
		  cmd_net_ppp_status),
	SHELL_SUBCMD_SET_END
);

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_SHELL_DYN_CMD_COMPLETION)
static
char nbr_address_buffer[CONFIG_NET_IPV6_MAX_NEIGHBORS][NET_IPV6_ADDR_LEN];

static void nbr_address_cb(struct net_nbr *nbr, void *user_data)
{
	int *count = user_data;

	if (*count >= CONFIG_NET_IPV6_MAX_NEIGHBORS) {
		return;
	}

	snprintk(nbr_address_buffer[*count], NET_IPV6_ADDR_LEN,
		 "%s", net_sprint_ipv6_addr(&net_ipv6_nbr_data(nbr)->addr));

	(*count)++;
}

static void nbr_populate_addresses(void)
{
	int count = 0;

	net_ipv6_nbr_foreach(nbr_address_cb, &count);
}

static char *set_nbr_address(size_t idx)
{
	if (idx == 0) {
		memset(nbr_address_buffer, 0, sizeof(nbr_address_buffer));
		nbr_populate_addresses();
	}

	if (idx >= CONFIG_NET_IPV6_MAX_NEIGHBORS) {
		return NULL;
	}

	if (!nbr_address_buffer[idx][0]) {
		return NULL;
	}

	return nbr_address_buffer[idx];
}

static void nbr_address_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(nbr_address, nbr_address_get);

#define NBR_ADDRESS_CMD &nbr_address

static void nbr_address_get(size_t idx, struct shell_static_entry *entry)
{
	entry->handler = NULL;
	entry->help  = NULL;
	entry->subcmd = &nbr_address;
	entry->syntax = set_nbr_address(idx);
}

#else
#define NBR_ADDRESS_CMD NULL
#endif /* CONFIG_NET_IPV6 && CONFIG_NET_SHELL_DYN_CMD_COMPLETION */

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_nbr,
	SHELL_CMD(rm, NBR_ADDRESS_CMD,
		  "'net nbr rm <address>' removes neighbor from cache.",
		  cmd_net_nbr_rm),
	SHELL_SUBCMD_SET_END
);

#if defined(CONFIG_NET_STATISTICS) && \
	defined(CONFIG_NET_STATISTICS_PER_INTERFACE) && \
	defined(CONFIG_NET_SHELL_DYN_CMD_COMPLETION)
#define STATS_IFACE_CMD &iface_index
#else
#define STATS_IFACE_CMD NULL
#endif /* CONFIG_NET_STATISTICS && CONFIG_NET_STATISTICS_PER_INTERFACE &&
	* CONFIG_NET_SHELL_DYN_CMD_COMPLETION
	*/

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_stats,
	SHELL_CMD(all, NULL,
		  "Show network statistics for all network interfaces.",
		  cmd_net_stats_all),
	SHELL_CMD(iface, STATS_IFACE_CMD,
		  "'net stats <index>' shows network statistics for "
		  "one specific network interface.",
		  cmd_net_stats_iface),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_tcp,
	SHELL_CMD(connect, NULL,
		  "'net tcp connect <address> <port>' connects to TCP peer.",
		  cmd_net_tcp_connect),
	SHELL_CMD(send, NULL,
		  "'net tcp send <data>' sends data to peer using TCP.",
		  cmd_net_tcp_send),
	SHELL_CMD(recv, NULL,
		  "'net tcp recv' receives data using TCP.",
		  cmd_net_tcp_recv),
	SHELL_CMD(close, NULL,
		  "'net tcp close' closes TCP connection.", cmd_net_tcp_close),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_vlan,
	SHELL_CMD(add, NULL,
		  "'net vlan add <tag> <index>' adds VLAN tag to the "
		  "network interface.",
		  cmd_net_vlan_add),
	SHELL_CMD(del, NULL,
		  "'net vlan del <tag>' deletes VLAN tag from the network "
		  "interface.",
		  cmd_net_vlan_del),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_ping,
	SHELL_CMD(--help, NULL,
		  "'net ping [-c count] [-i interval ms] [-I <iface index>] "
		  "[-Q tos] [-s payload size] [-p priority] <host>' "
		  "Send ICMPv4 or ICMPv6 Echo-Request to a network host.",
		  cmd_net_ping),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_pkt,
	SHELL_CMD(--help, NULL,
		  "'net pkt [ptr in hex]' "
		  "Print information about given net_pkt",
		  cmd_net_pkt),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_udp,
	SHELL_CMD(bind, NULL,
		  "'net udp bind <addr> <port>' binds to UDP local port.",
		  cmd_net_udp_bind),
	SHELL_CMD(close, NULL,
		  "'net udp close' closes previously bound port.",
		  cmd_net_udp_close),
	SHELL_CMD(send, NULL,
		  "'net udp send <host> <port> <payload>' "
		  "sends UDP packet to a network host.",
		  cmd_net_udp_send),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_commands,
	SHELL_CMD(allocs, NULL, "Print network memory allocations.",
		  cmd_net_allocs),
	SHELL_CMD(arp, &net_cmd_arp, "Print information about IPv4 ARP cache.",
		  cmd_net_arp),
	SHELL_CMD(capture, &net_cmd_capture,
		  "Configure network packet capture.", cmd_net_capture),
	SHELL_CMD(conn, NULL, "Print information about network connections.",
		  cmd_net_conn),
	SHELL_CMD(dns, &net_cmd_dns, "Show how DNS is configured.",
		  cmd_net_dns),
	SHELL_CMD(events, &net_cmd_events, "Monitor network management events.",
		  cmd_net_events),
	SHELL_CMD(gptp, &net_cmd_gptp, "Print information about gPTP support.",
		  cmd_net_gptp),
	SHELL_CMD(iface, &net_cmd_iface,
		  "Print information about network interfaces.",
		  cmd_net_iface),
	SHELL_CMD(ipv6, &net_cmd_ip6,
		  "Print information about IPv6 specific information and "
		  "configuration.",
		  cmd_net_ipv6),
	SHELL_CMD(ipv4, &net_cmd_ip,
		  "Print information about IPv4 specific information and "
		  "configuration.",
		  cmd_net_ipv4),
	SHELL_CMD(mem, NULL, "Print information about network memory usage.",
		  cmd_net_mem),
	SHELL_CMD(nbr, &net_cmd_nbr, "Print neighbor information.",
		  cmd_net_nbr),
	SHELL_CMD(ping, &net_cmd_ping, "Ping a network host.", cmd_net_ping),
	SHELL_CMD(pkt, &net_cmd_pkt, "net_pkt information.", cmd_net_pkt),
	SHELL_CMD(ppp, &net_cmd_ppp, "PPP information.", cmd_net_ppp_status),
	SHELL_CMD(resume, NULL, "Resume a network interface", cmd_net_resume),
	SHELL_CMD(route, &net_cmd_route, "Show network route.", cmd_net_route),
	SHELL_CMD(stacks, NULL, "Show network stacks information.",
		  cmd_net_stacks),
	SHELL_CMD(stats, &net_cmd_stats, "Show network statistics.",
		  cmd_net_stats),
	SHELL_CMD(suspend, NULL, "Suspend a network interface",
		  cmd_net_suspend),
	SHELL_CMD(tcp, &net_cmd_tcp, "Connect/send/close TCP connection.",
		  cmd_net_tcp),
	SHELL_CMD(udp, &net_cmd_udp, "Send/recv UDP packet", cmd_net_udp),
	SHELL_CMD(virtual, NULL, "Show virtual network interfaces.",
		  cmd_net_virtual),
	SHELL_CMD(vlan, &net_cmd_vlan, "Show VLAN information.", cmd_net_vlan),
	SHELL_CMD(websocket, NULL, "Print information about WebSocket "
								"connections.",
		  cmd_net_websocket),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(net, &net_commands, "Networking commands", NULL);

int net_shell_init(void)
{
#if defined(CONFIG_NET_MGMT_EVENT_MONITOR_AUTO_START)
	char *argv[] = {
		"on",
		NULL
	};

	(void)cmd_net_events_on(shell_backend_uart_get_ptr(), 1, argv);
#endif

	return 0;
}
