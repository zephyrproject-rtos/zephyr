/** @file
 * @brief Network shell module
 *
 * Provide some networking shell commands that can be useful to applications.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_shell, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <kernel_internal.h>
#include <stdlib.h>
#include <stdio.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>

#include <net/net_if.h>
#include <net/dns_resolve.h>
#include <net/ppp.h>
#include <net/net_stats.h>
#include <sys/printk.h>

#include "route.h"
#include "icmpv6.h"
#include "icmpv4.h"
#include "connection.h"

#if defined(CONFIG_NET_TCP)
#include "tcp_internal.h"
#endif

#include "ipv6.h"

#if defined(CONFIG_NET_ARP)
#include "ethernet/arp.h"
#endif

#if defined(CONFIG_NET_L2_ETHERNET)
#include <net/ethernet.h>
#endif

#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
#include <net/ethernet_mgmt.h>
#endif

#if defined(CONFIG_NET_GPTP)
#include <net/gptp.h>
#include "ethernet/gptp/gptp_messages.h"
#include "ethernet/gptp/gptp_md.h"
#include "ethernet/gptp/gptp_state.h"
#include "ethernet/gptp/gptp_data_set.h"
#include "ethernet/gptp/gptp_private.h"
#endif

#if defined(CONFIG_NET_L2_PPP)
#include <net/ppp.h>
#include "ppp/ppp_internal.h"
#endif

#include "net_shell.h"
#include "net_stats.h"

#include <sys/fdtable.h>
#include "websocket/websocket_internal.h"

#define PR(fmt, ...)						\
	shell_fprintf(shell, SHELL_NORMAL, fmt, ##__VA_ARGS__)

#define PR_SHELL(shell, fmt, ...)				\
	shell_fprintf(shell, SHELL_NORMAL, fmt, ##__VA_ARGS__)

#define PR_ERROR(fmt, ...)					\
	shell_fprintf(shell, SHELL_ERROR, fmt, ##__VA_ARGS__)

#define PR_INFO(fmt, ...)					\
	shell_fprintf(shell, SHELL_INFO, fmt, ##__VA_ARGS__)

#define PR_WARNING(fmt, ...)					\
	shell_fprintf(shell, SHELL_WARNING, fmt, ##__VA_ARGS__)

#include "net_private.h"

struct net_shell_user_data {
	const struct shell *shell;
	void *user_data;
};

/* net_stack dedicated section limiters */
extern struct net_stack_info __net_stack_start[];
extern struct net_stack_info __net_stack_end[];

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

#ifdef CONFIG_NET_L2_CANBUS
	if (net_if_l2(iface) == &NET_L2_GET_NAME(CANBUS)) {
		if (extra) {
			*extra = "======";
		}

		return "CANBUS";
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
	EC(ETHERNET_PROMISC_MODE,         "Promiscuous mode"),
	EC(ETHERNET_PRIORITY_QUEUES,      "Priority queues"),
	EC(ETHERNET_HW_FILTERING,         "MAC address filtering"),
};

static void print_supported_ethernet_capabilities(
	const struct shell *shell, struct net_if *iface)
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

static void iface_cb(struct net_if *iface, void *user_data)
{
#if defined(CONFIG_NET_NATIVE)
	struct net_shell_user_data *data = user_data;
	const struct shell *shell = data->shell;

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
#if defined(CONFIG_NET_IPV4) || defined(CONFIG_NET_IPV6)
	struct net_if_addr *unicast;
	struct net_if_mcast_addr *mcast;
#endif
#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
	struct ethernet_req_params params;
	int ret;
#endif
	const char *extra;
#if defined(CONFIG_NET_IPV4) || defined(CONFIG_NET_IPV6)
	int i, count;
#endif

	if (data->user_data && data->user_data != iface) {
		return;
	}

	PR("\nInterface %p (%s) [%d]\n", iface, iface2str(iface, &extra),
	   net_if_get_by_iface(iface));
	PR("===========================%s\n", extra);

	if (!net_if_is_up(iface)) {
		PR_INFO("Interface is down.\n");
		return;
	}

	if (net_if_get_link_addr(iface) &&
	    net_if_get_link_addr(iface)->addr) {
		PR("Link addr : %s\n",
		   net_sprint_ll_addr(net_if_get_link_addr(iface)->addr,
				      net_if_get_link_addr(iface)->len));
	}

	PR("MTU       : %d\n", net_if_get_mtu(iface));

#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
	count = 0;
	ret = net_mgmt(NET_REQUEST_ETHERNET_GET_PRIORITY_QUEUES_NUM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));

	if (!ret && params.priority_queues_num) {
		count = params.priority_queues_num;
		PR("Priority queues:\n");
		for (i = 0; i < count; ++i) {
			params.qav_param.queue_id = i;
			params.qav_param.type = ETHERNET_QAV_PARAM_TYPE_STATUS;
			ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QAV_PARAM,
				       iface,
				       &params,
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
		print_supported_ethernet_capabilities(shell, iface);
	}
#endif /* CONFIG_NET_L2_ETHERNET */

#if defined(CONFIG_NET_IPV6)
	count = 0;

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
		PR_WARNING("IPv4 not supported for this interface.\n");
		return;
	}

	count = 0;

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

#else
	ARG_UNUSED(iface);
	ARG_UNUSED(user_data);

#endif /* CONFIG_NET_NATIVE */
}

#if defined(CONFIG_NET_ROUTE) && defined(CONFIG_NET_NATIVE)
static void route_cb(struct net_route_entry *entry, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *shell = data->shell;
	struct net_if *iface = data->user_data;
	struct net_route_nexthop *nexthop_route;
	int count;

	if (entry->iface != iface) {
		return;
	}

	PR("IPv6 prefix : %s/%d\n", net_sprint_ipv6_addr(&entry->addr),
	   entry->prefix_len);

	count = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&entry->nexthop, nexthop_route, node) {
		struct net_linkaddr_storage *lladdr;

		if (!nexthop_route->nbr) {
			continue;
		}

		PR("\tneighbor : %p\t", nexthop_route->nbr);

		if (nexthop_route->nbr->idx == NET_NBR_LLADDR_UNKNOWN) {
			PR("addr : <unknown>\n");
		} else {
			lladdr = net_nbr_get_lladdr(nexthop_route->nbr->idx);

			PR("addr : %s\n", net_sprint_ll_addr(lladdr->addr,
							     lladdr->len));
		}

		count++;
	}

	if (count == 0) {
		PR("\t<none>\n");
	}
}

static void iface_per_route_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *shell = data->shell;
	const char *extra;

	PR("\nIPv6 routes for interface %p (%s)\n", iface,
	   iface2str(iface, &extra));
	PR("=======================================%s\n", extra);

	data->user_data = iface;

	net_route_foreach(route_cb, data);
}
#endif /* CONFIG_NET_ROUTE */

#if defined(CONFIG_NET_ROUTE_MCAST) && defined(CONFIG_NET_NATIVE)
static void route_mcast_cb(struct net_route_entry_mcast *entry,
			   void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *shell = data->shell;
	struct net_if *iface = data->user_data;
	const char *extra;

	if (entry->iface != iface) {
		return;
	}

	PR("IPv6 multicast route %p for interface %p (%s)\n", entry,
	   iface, iface2str(iface, &extra));
	PR("==========================================================="
	   "%s\n", extra);

	PR("IPv6 group : %s\n", net_sprint_ipv6_addr(&entry->group));
	PR("Lifetime   : %u\n", entry->lifetime);
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
			    const struct shell *shell)
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
			    const struct shell *shell)
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

static void print_tc_tx_stats(const struct shell *shell, struct net_if *iface)
{
#if NET_TC_TX_COUNT > 1
	int i;

	PR("TX traffic class statistics:\n");

#if defined(CONFIG_NET_CONTEXT_TIMESTAMP) || \
				defined(CONFIG_NET_PKT_TXTIME_STATS)
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
			PR("[%d] %s (%d)\t%d\t\t%d\t%lu us\n", i,
			   priority2str(GET_STAT(iface, tc.sent[i].priority)),
			   GET_STAT(iface, tc.sent[i].priority),
			   GET_STAT(iface, tc.sent[i].pkts),
			   GET_STAT(iface, tc.sent[i].bytes),
			   (u32_t)(GET_STAT(iface,
					    tc.sent[i].tx_time.sum) /
				   (u64_t)count));
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
#endif /* CONFIG_NET_CONTEXT_TIMESTAMP */
#else
	ARG_UNUSED(shell);

#if defined(CONFIG_NET_PKT_TXTIME_STATS)
	net_stats_t count = GET_STAT(iface, tx_time.count);

	if (count != 0) {
		PR("Avg %s net_pkt (%u) time %lu us\n", "TX", count,
		   (u32_t)(GET_STAT(iface, tx_time.sum) / (u64_t)count));
	}
#else
	ARG_UNUSED(iface);
#endif /* CONFIG_NET_PKT_TXTIME_STATS */
#endif /* NET_TC_TX_COUNT > 1 */
}

static void print_tc_rx_stats(const struct shell *shell, struct net_if *iface)
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
			PR("[%d] %s (%d)\t%d\t\t%d\t%lu us\n", i,
			   priority2str(GET_STAT(iface, tc.recv[i].priority)),
			   GET_STAT(iface, tc.recv[i].priority),
			   GET_STAT(iface, tc.recv[i].pkts),
			   GET_STAT(iface, tc.recv[i].bytes),
			   (u32_t)(GET_STAT(iface,
					    tc.recv[i].rx_time.sum) /
				   (u64_t)count));
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
	ARG_UNUSED(shell);

#if defined(CONFIG_NET_PKT_RXTIME_STATS)
	net_stats_t count = GET_STAT(iface, rx_time.count);

	if (count != 0) {
		PR("Avg %s net_pkt (%u) time %lu us\n", "RX", count,
		   (u32_t)(GET_STAT(iface, rx_time.sum) / (u64_t)count));
	}
#else
	ARG_UNUSED(iface);
#endif /* CONFIG_NET_PKT_RXTIME_STATS */

#endif /* NET_TC_RX_COUNT > 1 */
}

static void net_shell_print_statistics(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *shell = data->shell;

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

#if defined(CONFIG_NET_STATISTICS_UDP) && defined(CONFIG_NET_NATIVE_UDP)
	PR("UDP recv       %d\tsent\t%d\tdrop\t%d\n",
	   GET_STAT(iface, udp.recv),
	   GET_STAT(iface, udp.sent),
	   GET_STAT(iface, udp.drop));
	PR("UDP chkerr     %d\n",
	   GET_STAT(iface, udp.chkerr));
#endif

#if defined(CONFIG_NET_STATISTICS_TCP) && defined(CONFIG_NET_NATIVE_TCP)
	PR("TCP bytes recv %u\tsent\t%d\n",
	   GET_STAT(iface, tcp.bytes.received),
	   GET_STAT(iface, tcp.bytes.sent));
	PR("TCP seg recv   %d\tsent\t%d\tdrop\t%d\n",
	   GET_STAT(iface, tcp.recv),
	   GET_STAT(iface, tcp.sent),
	   GET_STAT(iface, tcp.drop));
	PR("TCP seg resent %d\tchkerr\t%d\tackerr\t%d\n",
	   GET_STAT(iface, tcp.resent),
	   GET_STAT(iface, tcp.chkerr),
	   GET_STAT(iface, tcp.ackerr));
	PR("TCP seg rsterr %d\trst\t%d\tre-xmit\t%d\n",
	   GET_STAT(iface, tcp.rsterr),
	   GET_STAT(iface, tcp.rst),
	   GET_STAT(iface, tcp.rexmit));
	PR("TCP conn drop  %d\tconnrst\t%d\n",
	   GET_STAT(iface, tcp.conndrop),
	   GET_STAT(iface, tcp.connrst));
#endif

#if defined(CONFIG_NET_CONTEXT_TIMESTAMP) && defined(CONFIG_NET_NATIVE)
	if (GET_STAT(iface, tx_time.count) > 0) {
		PR("Network pkt TX time %lu us\n",
		   (u32_t)(GET_STAT(iface, tx_time.sum) /
			   (u64_t)GET_STAT(iface, tx_time.count)));
	}
#endif

	PR("Bytes received %u\n", GET_STAT(iface, bytes.received));
	PR("Bytes sent     %u\n", GET_STAT(iface, bytes.sent));
	PR("Processing err %d\n", GET_STAT(iface, processing_error));

	print_tc_tx_stats(shell, iface);
	print_tc_rx_stats(shell, iface);

#if defined(CONFIG_NET_STATISTICS_ETHERNET) && \
					defined(CONFIG_NET_STATISTICS_USER_API)
	if (iface && net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		struct net_stats_eth eth_data;
		int ret;

		ret = net_mgmt(NET_REQUEST_STATS_GET_ETHERNET, iface,
			       &eth_data, sizeof(eth_data));
		if (!ret) {
			print_eth_stats(iface, &eth_data, shell);
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
			print_ppp_stats(iface, &ppp_data, shell);
		}
	}
#endif /* CONFIG_NET_STATISTICS_PPP && CONFIG_NET_STATISTICS_USER_API */
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
	const struct shell *shell = data->shell;
	int *count = data->user_data;
	/* +7 for []:port */
	char addr_local[ADDR_LEN + 7];
	char addr_remote[ADDR_LEN + 7] = "";

	get_addresses(context, addr_local, sizeof(addr_local),
		      addr_remote, sizeof(addr_remote));

	PR("[%2d] %p\t%p    %c%c%c   %16s\t%16s\n",
	   (*count) + 1, context,
	   net_context_get_iface(context),
	   net_context_get_family(context) == AF_INET6 ? '6' :
	   (net_context_get_family(context) == AF_INET ? '4' : ' '),
	   net_context_get_type(context) == SOCK_DGRAM ? 'D' :
	   (net_context_get_type(context) == SOCK_STREAM ? 'S' :
	    (net_context_get_type(context) == SOCK_RAW ? 'R' : ' ')),
	   net_context_get_ip_proto(context) == IPPROTO_UDP ? 'U' :
	   (net_context_get_ip_proto(context) == IPPROTO_TCP ? 'T' : ' '),
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
	const struct shell *shell = data->shell;
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
#ifdef CONFIG_NET_L2_CANBUS
	if (conn->local_addr.sa_family == AF_CAN) {
		snprintk(addr_local, sizeof(addr_local), "-");
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

#if defined(CONFIG_NET_TCP1) && \
	(defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_NATIVE))
static void tcp_cb(struct net_tcp *tcp, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *shell = data->shell;
	int *count = data->user_data;
	u16_t recv_mss = net_tcp_get_recv_mss(tcp);

	PR("%p %p   %5u    %5u %10u %10u %5u   %s\n",
	   tcp, tcp->context,
	   ntohs(net_sin6_ptr(&tcp->context->local)->sin6_port),
	   ntohs(net_sin6(&tcp->context->remote)->sin6_port),
	   tcp->send_seq, tcp->send_ack, recv_mss,
	   net_tcp_state_str(net_tcp_get_state(tcp)));

	(*count)++;
}

#if CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG
static void tcp_sent_list_cb(struct net_tcp *tcp, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *shell = data->shell;
	int *printed = data->user_data;
	struct net_pkt *pkt;
	struct net_pkt *tmp;

	if (sys_slist_is_empty(&tcp->sent_list)) {
		return;
	}

	if (!*printed) {
		PR("\nTCP packets waiting ACK:\n");
		PR("TCP             net_pkt[ref/totlen]->net_buf[ref/len]..."
		   "\n");
	}

	PR("%p      ", tcp);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&tcp->sent_list, pkt, tmp,
					  sent_list) {
		struct net_buf *frag = pkt->frags;

		if (!*printed) {
			PR("%p[%d/%zd]", pkt, atomic_get(&pkt->atomic_ref),
			       net_pkt_get_len(pkt));
			*printed = true;
		} else {
			PR("                %p[%d/%zd]",
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

	*printed = true;
}
#endif /* CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG */
#endif

#if defined(CONFIG_NET_IPV6_FRAGMENT)
static void ipv6_frag_cb(struct net_ipv6_reassembly *reass,
			 void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *shell = data->shell;
	int *count = data->user_data;
	char src[ADDR_LEN];
	int i;

	if (!*count) {
		PR("\nIPv6 reassembly Id         Remain "
		   "Src             \tDst\n");
	}

	snprintk(src, ADDR_LEN, "%s", net_sprint_ipv6_addr(&reass->src));

	PR("%p      0x%08x  %5d %16s\t%16s\n",
	   reass, reass->id,
	   k_delayed_work_remaining_get(&reass->timer),
	   src, net_sprint_ipv6_addr(&reass->dst));

	for (i = 0; i < NET_IPV6_FRAGMENTS_MAX_PKT; i++) {
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
	const struct shell *shell = data->shell;
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
			PR("%p/%d\t%5s\t%5s\t%s():%d\n",
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

static int cmd_net_allocs(const struct shell *shell, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC)
	struct net_shell_user_data user_data;
#endif

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC)
	user_data.shell = shell;

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
	const struct shell *shell = data->shell;
	int *count = data->user_data;

	if (*count == 0) {
		PR("     Interface  Link              Address\n");
	}

	PR("[%2d] %p %s %s\n", *count, entry->iface,
	   net_sprint_ll_addr(entry->eth.addr, sizeof(struct net_eth_addr)),
	   net_sprint_ipv4_addr(&entry->ip));

	(*count)++;
}
#endif /* CONFIG_NET_ARP */

#if !defined(CONFIG_NET_ARP)
static void print_arp_error(const struct shell *shell)
{
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_NATIVE, CONFIG_NET_ARP, CONFIG_NET_IPV4 and"
		" CONFIG_NET_L2_ETHERNET", "ARP");
}
#endif

static int cmd_net_arp(const struct shell *shell, size_t argc, char *argv[])
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

		user_data.shell = shell;
		user_data.user_data = &count;

		if (net_arp_foreach(arp_cb, &user_data) == 0) {
			PR("ARP cache is empty.\n");
		}
	}
#else
	print_arp_error(shell);
#endif

	return 0;
}

static int cmd_net_arp_flush(const struct shell *shell, size_t argc,
			     char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_ARP)
	PR("Flushing ARP cache.\n");
	net_arp_clear_cache(NULL);
#else
	print_arp_error(shell);
#endif

	return 0;
}

static int cmd_net_conn(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_NATIVE)
	struct net_shell_user_data user_data;
	int count = 0;

	PR("     Context   \tIface         Flags Local           \tRemote\n");

	user_data.shell = shell;
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

#if defined(CONFIG_NET_TCP1)
	PR("\nTCP        Context   Src port Dst port   "
	   "Send-Seq   Send-Ack  MSS    State\n");

	count = 0;

	net_tcp_foreach(tcp_cb, &user_data);

	if (count == 0) {
		PR("No TCP connections\n");
	} else {
#if CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG
		/* Print information about pending packets */
		count = 0;
		net_tcp_foreach(tcp_sent_list_cb, &user_data);
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
	const struct shell *shell = user_data;

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

static void print_dns_info(const struct shell *shell,
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
		s32_t remaining;

		if (!ctx->queries[i].cb) {
			continue;
		}

		remaining =
			k_delayed_work_remaining_get(&ctx->queries[i].timer);

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

static int cmd_net_dns_cancel(const struct shell *shell, size_t argc,
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

static int cmd_net_dns_query(const struct shell *shell, size_t argc,
			     char *argv[])
{

#if defined(CONFIG_DNS_RESOLVER)
#define DNS_TIMEOUT K_MSEC(2000) /* ms */
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
				(void *)shell, DNS_TIMEOUT);
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

static int cmd_net_dns(const struct shell *shell, size_t argc, char *argv[])
{
#if defined(CONFIG_DNS_RESOLVER)
	struct dns_resolve_context *ctx;
#endif

#if defined(CONFIG_DNS_RESOLVER)
	if (argv[1]) {
		/* So this is a query then */
		cmd_net_dns_query(shell, argc, argv);
		return 0;
	}

	/* DNS status */
	ctx = dns_resolve_get_default();
	if (!ctx) {
		PR_WARNING("No default DNS context found.\n");
		return -ENOEXEC;
	}

	print_dns_info(shell, ctx);
#else
	PR_INFO("DNS resolver not supported. Set CONFIG_DNS_RESOLVER to "
		"enable it.\n");
#endif

	return 0;
}

#if defined(CONFIG_NET_GPTP)
static void gptp_port_cb(int port, struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *shell = data->shell;
	int *count = data->user_data;

	if (*count == 0) {
		PR("Port Interface\n");
	}

	(*count)++;

	PR("%2d   %p\n", port, iface);
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

static void gptp_print_port_info(const struct shell *shell, int port)
{
	struct gptp_port_bmca_data *port_bmca_data;
	struct gptp_port_param_ds *port_param_ds;
	struct gptp_port_states *port_state;
	struct gptp_port_ds *port_ds;
	struct net_if *iface;
	int ret, i;

	ret = gptp_get_port_data(gptp_get_domain(),
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

	PR("Port id    : %d\n", port_ds->port_id.port_number);

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
	   (u32_t)port_ds->neighbor_prop_delay);
	PR("Propagation time threshold for %s : %u ns\n",
	   "the link attached to this port",
	   (u32_t)port_ds->neighbor_prop_delay_thresh);
	PR("Estimate of the ratio of the frequency with the peer          "
	   ": %u\n", (u32_t)port_ds->neighbor_rate_ratio);
	PR("Asymmetry on the link relative to the grand master time base  "
	   ": %lld\n", port_ds->delay_asymmetry);
	PR("Maximum interval between sync %s                        "
	   ": %llu\n", "messages",
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
	PR("Time without receiving sync %s %s      : %llu ms (%d)\n",
	   "messages", "before running BMCA",
	   (port_ds->sync_receipt_timeout_time_itv >> 16) /
					(NSEC_PER_SEC / MSEC_PER_SEC),
	   port_ds->sync_receipt_timeout);
	PR("Sync event %s                 : %llu ms\n",
	   "transmission interval for the port",
	   USCALED_NS_TO_NS(port_ds->half_sync_itv.low) /
					(NSEC_PER_USEC * USEC_PER_MSEC));
	PR("Path Delay Request %s         : %llu ms\n",
	   "transmission interval for the port",
	   USCALED_NS_TO_NS(port_ds->pdelay_req_itv.low) /
					(NSEC_PER_USEC * USEC_PER_MSEC));

	PR("\nRuntime status:\n");
	PR("Current global port state                          "
	   "      : %s\n", selected_role_str(port));
	PR("Path Delay Request state machine variables:\n");
	PR("\tCurrent state                                    "
	   ": %s\n", pdelay_req2str(port_state->pdelay_req.state));
	PR("\tInitial Path Delay Response Peer Timestamp       "
	   ": %llu\n", port_state->pdelay_req.ini_resp_evt_tstamp);
	PR("\tInitial Path Delay Response Ingress Timestamp    "
	   ": %llu\n", port_state->pdelay_req.ini_resp_ingress_tstamp);
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
	   ": %llu\n", "Message",
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
	   ": %lld\n",
	   port_state->pss_send.last_follow_up_correction_field);
	PR("\tUpstream Tx Time of the last recv PortSyncSync   "
	   ": %llu\n", port_state->pss_send.last_upstream_tx_time);
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

static int cmd_net_gptp_port(const struct shell *shell, size_t argc,
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
		gptp_print_port_info(shell, port);
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

static int cmd_net_gptp(const struct shell *shell, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_GPTP)
	/* gPTP status */
	struct gptp_domain *domain = gptp_get_domain();
	int count = 0;
	int arg = 1;
#endif

#if defined(CONFIG_NET_GPTP)
	if (argv[arg]) {
		cmd_net_gptp_port(shell, argc, argv);
	} else {
		struct net_shell_user_data user_data;

		user_data.shell = shell;
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

static int get_iface_idx(const struct shell *shell, char *index_str)
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

static int cmd_net_iface_up(const struct shell *shell, size_t argc,
			    char *argv[])
{
	struct net_if *iface;
	int idx, ret;

	idx = get_iface_idx(shell, argv[1]);
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

static int cmd_net_iface_down(const struct shell *shell, size_t argc,
			      char *argv[])
{
	struct net_if *iface;
	int idx, ret;

	idx = get_iface_idx(shell, argv[1]);
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
static u32_t time_diff(u32_t time1, u32_t time2)
{
	return (u32_t)abs((s32_t)time1 - (s32_t)time2);
}

static void address_lifetime_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *shell = data->shell;
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	const char *extra;
	int i;

	ARG_UNUSED(user_data);

	PR("\nIPv6 addresses for interface %p (%s)\n", iface,
	       iface2str(iface, &extra));
	PR("==========================================%s\n", extra);

	if (!ipv6) {
		PR("No IPv6 config found for this interface.\n");
		return;
	}

	PR("Type      \tState    \tLifetime (sec)\tAddress\n");

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		struct net_if_ipv6_prefix *prefix;
		char remaining_str[sizeof("01234567890")];
		u64_t remaining;
		u8_t prefix_len;

		if (!ipv6->unicast[i].is_used ||
		    ipv6->unicast[i].address.family != AF_INET6) {
			continue;
		}

		remaining = (u64_t)ipv6->unicast[i].lifetime.timer_timeout +
			(u64_t)ipv6->unicast[i].lifetime.wrap_counter *
			(u64_t)NET_TIMEOUT_MAX_VALUE -
			(u64_t)time_diff(k_uptime_get_32(),
				ipv6->unicast[i].lifetime.timer_start);

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
				 "%u", (u32_t)(remaining / 1000U));
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

static int cmd_net_ipv6(const struct shell *shell, size_t argc, char *argv[])
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

	user_data.shell = shell;
	user_data.user_data = NULL;

	/* Print information about address lifetime */
	net_if_foreach(address_lifetime_cb, &user_data);
#endif

	return 0;
}

static int cmd_net_iface(const struct shell *shell, size_t argc, char *argv[])
{
	struct net_if *iface = NULL;
	struct net_shell_user_data user_data;
	int idx;

	if (argv[1]) {
		idx = get_iface_idx(shell, argv[1]);
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

	user_data.shell = shell;
	user_data.user_data = iface;

	net_if_foreach(iface_cb, &user_data);

	return 0;
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
	const struct shell *shell = data->shell;
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

#if CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
		PR("%p\t%u\t%u\tETX\n",
		   slab, slab->num_blocks, k_mem_slab_num_free_get(slab));
#else
		PR("%p\t%d\tETX\n", slab, slab->num_blocks);
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
		PR("%p\t%d\t%d\tEDATA (%s)\n",
		   pool, pool->buf_count,
		   pool->avail_count, pool->name);
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

static int cmd_net_mem(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_NATIVE)
	struct k_mem_slab *rx, *tx;
	struct net_buf_pool *rx_data, *tx_data;

	net_pkt_get_info(&rx, &tx, &rx_data, &tx_data);

	PR("Fragment length %d bytes\n", CONFIG_NET_BUF_DATA_SIZE);

	PR("Network buffer pools:\n");

#if defined(CONFIG_NET_BUF_POOL_USAGE)
	PR("Address\t\tTotal\tAvail\tName\n");

	PR("%p\t%d\t%u\tRX\n",
	       rx, rx->num_blocks, k_mem_slab_num_free_get(rx));

	PR("%p\t%d\t%u\tTX\n",
	       tx, tx->num_blocks, k_mem_slab_num_free_get(tx));

	PR("%p\t%d\t%d\tRX DATA (%s)\n",
	       rx_data, rx_data->buf_count,
	       rx_data->avail_count, rx_data->name);

	PR("%p\t%d\t%d\tTX DATA (%s)\n",
	       tx_data, tx_data->buf_count,
	       tx_data->avail_count, tx_data->name);
#else
	PR("Address\t\tTotal\tName\n");

	PR("%p\t%d\tRX\n", rx, rx->num_blocks);
	PR("%p\t%d\tTX\n", tx, tx->num_blocks);
	PR("%p\t%d\tRX DATA\n", rx_data, rx_data->buf_count);
	PR("%p\t%d\tTX DATA\n", tx_data, tx_data->buf_count);
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_BUF_POOL_USAGE", "net_buf allocation");
#endif /* CONFIG_NET_BUF_POOL_USAGE */

	if (IS_ENABLED(CONFIG_NET_CONTEXT_NET_PKT_POOL)) {
		struct net_shell_user_data user_data;
		struct ctx_info info;

		(void)memset(&info, 0, sizeof(info));

		user_data.shell = shell;
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

static int cmd_net_nbr_rm(const struct shell *shell, size_t argc,
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
	const struct shell *shell = data->shell;
	int *count = data->user_data;
	char *padding = "";
	char *state_pad = "";
	const char *state_str;
#if defined(CONFIG_NET_IPV6_ND)
	s64_t remaining;
#endif

#if defined(CONFIG_NET_L2_IEEE802154)
	padding = "      ";
#endif

	if (*count == 0) {
		PR("     Neighbor   Interface        Flags State     "
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

	PR("[%2d] %p %p %5d/%d/%d/%d %s%s %6d  %17s%s %s\n",
	   *count, nbr, nbr->iface,
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
	   net_nbr_get_lladdr(nbr->idx)->len == 8U ? "" : padding,
	   net_sprint_ipv6_addr(&net_ipv6_nbr_data(nbr)->addr));
}
#endif

static int cmd_net_nbr(const struct shell *shell, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_IPV6)
	int count = 0;
	struct net_shell_user_data user_data;
#endif

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_IPV6)
	user_data.shell = shell;
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

#if defined(CONFIG_NET_IPV6) || defined(CONFIG_NET_IPV4)

K_SEM_DEFINE(ping_timeout, 0, 1);
static const struct shell *shell_for_ping;

#if defined(CONFIG_NET_NATIVE_IPV6)

static enum net_verdict handle_ipv6_echo_reply(struct net_pkt *pkt,
						struct net_ipv6_hdr *ip_hdr,
						struct net_icmp_hdr *icmp_hdr);

static struct net_icmpv6_handler ping6_handler = {
	.type = NET_ICMPV6_ECHO_REPLY,
	.code = 0,
	.handler = handle_ipv6_echo_reply,
};

static inline void remove_ipv6_ping_handler(void)
{
	net_icmpv6_unregister_handler(&ping6_handler);
}

static enum net_verdict handle_ipv6_echo_reply(struct net_pkt *pkt,
					       struct net_ipv6_hdr *ip_hdr,
					       struct net_icmp_hdr *icmp_hdr)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access,
					      struct net_icmpv6_echo_req);
	struct net_icmpv6_echo_req *icmp_echo;
	u32_t cycles;

	icmp_echo = (struct net_icmpv6_echo_req *)net_pkt_get_data(pkt,
								&icmp_access);
	if (icmp_echo == NULL) {
		return -NET_DROP;
	}

	net_pkt_skip(pkt, sizeof(*icmp_echo));
	if (net_pkt_read_be32(pkt, &cycles)) {
		return -NET_DROP;
	}

	cycles = k_cycle_get_32() - cycles;

	PR_SHELL(shell_for_ping, "%d bytes from %s to %s: icmp_seq=%d ttl=%d "
#ifdef CONFIG_IEEE802154
		 "rssi=%d "
#endif
#ifdef CONFIG_FLOAT
		 "time=%.2f ms\n",
#else
		 "time=%d ms\n",
#endif
		 ntohs(ip_hdr->len) - net_pkt_ipv6_ext_len(pkt) -
								NET_ICMPH_LEN,
		 net_sprint_ipv6_addr(&ip_hdr->src),
		 net_sprint_ipv6_addr(&ip_hdr->dst),
		 ntohs(icmp_echo->sequence),
		 ip_hdr->hop_limit,
#ifdef CONFIG_IEEE802154
		 net_pkt_ieee802154_rssi(pkt),
#endif
#ifdef CONFIG_FLOAT
		 (SYS_CLOCK_HW_CYCLES_TO_NS(cycles) / 1000000.f));
#else
		 (SYS_CLOCK_HW_CYCLES_TO_NS(cycles) / 1000000));
#endif
	k_sem_give(&ping_timeout);

	net_pkt_unref(pkt);
	return NET_OK;
}

static int ping_ipv6(const struct shell *shell,
		     char *host,
		     unsigned int count,
		     unsigned int interval)
{
	struct net_if *iface = net_if_get_default();
	int ret = 0;
	struct in6_addr ipv6_target;
	struct net_nbr *nbr;

#if defined(CONFIG_NET_ROUTE)
	struct net_route_entry *route;
#endif

	if (net_addr_pton(AF_INET6, host, &ipv6_target) < 0) {
		return -EINVAL;
	}

	net_icmpv6_register_handler(&ping6_handler);

	iface = net_if_ipv6_select_src_iface(&ipv6_target);
	if (!iface) {
		nbr = net_ipv6_nbr_lookup(NULL, &ipv6_target);
		if (nbr) {
			iface = nbr->iface;
		}
	}

#if defined(CONFIG_NET_ROUTE)
	route = net_route_lookup(NULL, &ipv6_target);
	if (route) {
		iface = route->iface;
	}
#endif

	PR("PING %s\n", host);

	for (int i = 0; i < count; ++i) {
		u32_t time_stamp = htonl(k_cycle_get_32());

		ret = net_icmpv6_send_echo_request(iface,
						   &ipv6_target,
						   sys_rand32_get(),
						   i,
						   &time_stamp,
						   sizeof(time_stamp));
		if (ret) {
			break;
		}

		k_sleep(interval);
	}

	remove_ipv6_ping_handler();

	return ret;
}
#else
#define ping_ipv6(...) -ENOTSUP
#define remove_ipv6_ping_handler()
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_NATIVE_IPV4)

static enum net_verdict handle_ipv4_echo_reply(struct net_pkt *pkt,
					       struct net_ipv4_hdr *ip_hdr,
					       struct net_icmp_hdr *icmp_hdr);

static struct net_icmpv4_handler ping4_handler = {
	.type = NET_ICMPV4_ECHO_REPLY,
	.code = 0,
	.handler = handle_ipv4_echo_reply,
};

static inline void remove_ipv4_ping_handler(void)
{
	net_icmpv4_unregister_handler(&ping4_handler);
}

static enum net_verdict handle_ipv4_echo_reply(struct net_pkt *pkt,
					       struct net_ipv4_hdr *ip_hdr,
					       struct net_icmp_hdr *icmp_hdr)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access,
					      struct net_icmpv4_echo_req);
	u32_t cycles;
	struct net_icmpv4_echo_req *icmp_echo;

	icmp_echo = (struct net_icmpv4_echo_req *)net_pkt_get_data(pkt,
								&icmp_access);
	if (icmp_echo == NULL) {
		return -NET_DROP;
	}

	net_pkt_skip(pkt, sizeof(*icmp_echo));
	if (net_pkt_read_be32(pkt, &cycles)) {
		return -NET_DROP;
	}

	cycles = k_cycle_get_32() - cycles;

	PR_SHELL(shell_for_ping, "%d bytes from %s to %s: icmp_seq=%d ttl=%d "
#ifdef CONFIG_FLOAT
		 "time=%.2f ms\n",
#else
		 "time=%d ms\n",
#endif
		 ntohs(ip_hdr->len) - net_pkt_ipv6_ext_len(pkt) -
								NET_ICMPH_LEN,
		 net_sprint_ipv4_addr(&ip_hdr->src),
		 net_sprint_ipv4_addr(&ip_hdr->dst),
		 ntohs(icmp_echo->sequence),
		 ip_hdr->ttl,
#ifdef CONFIG_FLOAT
		 (SYS_CLOCK_HW_CYCLES_TO_NS(cycles) / 1000000.f));
#else
		 (SYS_CLOCK_HW_CYCLES_TO_NS(cycles) / 1000000));
#endif
	k_sem_give(&ping_timeout);

	net_pkt_unref(pkt);
	return NET_OK;
}

static int ping_ipv4(const struct shell *shell,
		     char *host,
		     unsigned int count,
		     unsigned int interval)
{
	struct in_addr ipv4_target;
	int ret = 0;

	if (net_addr_pton(AF_INET, host, &ipv4_target) < 0) {
		return -EINVAL;
	}

	struct net_if *iface = net_if_ipv4_select_src_iface(&ipv4_target);

	net_icmpv4_register_handler(&ping4_handler);

	PR("PING %s\n", host);

	for (int i = 0; i < count; ++i) {
		u32_t time_stamp = htonl(k_cycle_get_32());

		ret = net_icmpv4_send_echo_request(iface,
						   &ipv4_target,
						   sys_rand32_get(),
						   i,
						   &time_stamp,
						   sizeof(time_stamp));
		if (ret) {
			break;
		}

		k_sleep(interval);
	}

	remove_ipv4_ping_handler();

	return ret;
}
#else
#define ping_ipv4(...) -ENOTSUP
#define remove_ipv4_ping_handler()
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

	res = strtol(str, &endptr, 10);

	if (errno || (endptr == str)) {
		return -1;
	}

	return res;
}
#endif /* CONFIG_NET_IPV6 || CONFIG_NET_IPV4 */

static int cmd_net_ping(const struct shell *shell, size_t argc, char *argv[])
{
#if !defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return -EOPNOTSUPP;
#else
	char *host = NULL;
	int ret;

	int count = 3;
	int interval = 1000;

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
		default:
			PR_WARNING("Unrecognized argument: %s\n", argv[i]);
			return -ENOEXEC;
		}
	}

	if (!host) {
		PR_WARNING("Target host missing\n");
		return -ENOEXEC;
	}

	shell_for_ping = shell;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		ret = ping_ipv6(shell, host, count, interval);
		if (!ret) {
			goto wait_reply;
		} else if (ret == -EIO) {
			PR_WARNING("Cannot send IPv6 ping\n");
			return -ENOEXEC;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = ping_ipv4(shell, host, count, interval);
		if (ret) {
			if (ret == -EIO) {
				PR_WARNING("Cannot send IPv4 ping\n");
			} else if (ret == -EINVAL) {
				PR_WARNING("Invalid IP address\n");
			}

			return -ENOEXEC;
		}
	}

wait_reply:
	ret = k_sem_take(&ping_timeout, K_SECONDS(2));
	if (ret == -EAGAIN) {
		PR_INFO("Ping timeout\n");
		remove_ipv6_ping_handler();
		remove_ipv4_ping_handler();

		return -ETIMEDOUT;
	}

	return 0;
#endif
}

static int cmd_net_ppp_ping(const struct shell *shell, size_t argc,
			    char *argv[])
{
#if defined(CONFIG_NET_PPP)
	if (argv[1]) {
		int ret, idx = get_iface_idx(shell, argv[1]);

		if (idx < 0) {
			return -ENOEXEC;
		}

		ret = net_ppp_ping(idx, K_SECONDS(1));
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

static int cmd_net_ppp_status(const struct shell *shell, size_t argc,
			      char *argv[])
{
#if defined(CONFIG_NET_PPP)
	int idx = 0;
	struct ppp_context *ctx;

	if (argv[1]) {
		idx = get_iface_idx(shell, argv[1]);
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

#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_L2_PPP and CONFIG_NET_PPP", "PPP");
#endif
	return 0;
}

static int cmd_net_route(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_NATIVE)
#if defined(CONFIG_NET_ROUTE) || defined(CONFIG_NET_ROUTE_MCAST)
	struct net_shell_user_data user_data;
#endif

#if defined(CONFIG_NET_ROUTE) || defined(CONFIG_NET_ROUTE_MCAST)
	user_data.shell = shell;
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

#if defined(CONFIG_INIT_STACKS)
extern K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(sys_work_q_stack,
			     CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE);
#endif

static int cmd_net_stacks(const struct shell *shell, size_t argc,
			  char *argv[])
{
#if defined(CONFIG_INIT_STACKS)
	unsigned int pcnt, unused;
#endif
	struct net_stack_info *info;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	for (info = __net_stack_start; info != __net_stack_end; info++) {
		net_analyze_stack_get_values(Z_THREAD_STACK_BUFFER(info->stack),
					     info->size, &pcnt, &unused);

#if defined(CONFIG_INIT_STACKS)
		/* If the index is <0, then this stack is not part of stack
		 * array so do not print the index value in this case.
		 */
		if (info->idx >= 0) {
			PR("%s-%d [%s-%d] stack size %zu/%zu bytes "
			   "unused %u usage %zu/%zu (%u %%)\n",
			   info->pretty_name, info->prio, info->name,
			   info->idx, info->orig_size,
			   info->size, unused,
			   info->size - unused, info->size, pcnt);
		} else {
			PR("%s [%s] stack size %zu/%zu bytes unused %u "
			   "usage %zu/%zu (%u %%)\n",
			   info->pretty_name, info->name, info->orig_size,
			   info->size, unused,
			   info->size - unused, info->size, pcnt);
		}
#else
		PR("%s [%s] stack size %zu usage not available\n",
		   info->pretty_name, info->name, info->orig_size);
#endif
	}

#if defined(CONFIG_INIT_STACKS)
	net_analyze_stack_get_values(Z_THREAD_STACK_BUFFER(z_main_stack),
				     K_THREAD_STACK_SIZEOF(z_main_stack),
				     &pcnt, &unused);
	PR("%s [%s] stack size %d/%d bytes unused %u usage %d/%d (%u %%)\n",
	   "main", "z_main_stack", CONFIG_MAIN_STACK_SIZE,
	   CONFIG_MAIN_STACK_SIZE, unused,
	   CONFIG_MAIN_STACK_SIZE - unused, CONFIG_MAIN_STACK_SIZE, pcnt);

	net_analyze_stack_get_values(Z_THREAD_STACK_BUFFER(_interrupt_stack),
				     K_THREAD_STACK_SIZEOF(_interrupt_stack),
				     &pcnt, &unused);
	PR("%s [%s] stack size %d/%d bytes unused %u usage %d/%d (%u %%)\n",
	   "ISR", "_interrupt_stack", CONFIG_ISR_STACK_SIZE,
	   CONFIG_ISR_STACK_SIZE, unused,
	   CONFIG_ISR_STACK_SIZE - unused, CONFIG_ISR_STACK_SIZE, pcnt);

	net_analyze_stack_get_values(Z_THREAD_STACK_BUFFER(sys_work_q_stack),
				     K_THREAD_STACK_SIZEOF(sys_work_q_stack),
				     &pcnt, &unused);
	PR("%s [%s] stack size %d/%d bytes unused %u usage %d/%d (%u %%)\n",
	   "WORKQ", "system workqueue",
	   CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE,
	   CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE, unused,
	   CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE - unused,
	   CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE, pcnt);
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_INIT_STACKS",
		"stack information");
#endif

	return 0;
}

#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
static void net_shell_print_statistics_all(struct net_shell_user_data *data)
{
	net_if_foreach(net_shell_print_statistics, data);
}
#endif

static int cmd_net_stats_all(const struct shell *shell, size_t argc,
			     char *argv[])
{
#if defined(CONFIG_NET_STATISTICS)
	struct net_shell_user_data user_data;
#endif

#if defined(CONFIG_NET_STATISTICS)
	user_data.shell = shell;

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

static int cmd_net_stats_iface(const struct shell *shell, size_t argc,
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

	data.shell = shell;

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

static int cmd_net_stats(const struct shell *shell, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_STATISTICS)
	if (!argv[1]) {
		cmd_net_stats_all(shell, argc, argv);
		return 0;
	}

	if (strcmp(argv[1], "reset") == 0) {
		net_stats_reset(NULL);
	} else {
		cmd_net_stats_iface(shell, argc, argv);
	}
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_STATISTICS",
		"statistics");
#endif

	return 0;
}

#if defined(CONFIG_NET_TCP1) && defined(CONFIG_NET_NATIVE_TCP)
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

static void print_connect_info(const struct shell *shell,
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

static void tcp_connect(const struct shell *shell, char *host, u16_t port,
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

	print_connect_info(shell, family, &myaddr, &addr);

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
	tcp_shell = shell;

	net_context_connect(*ctx, &addr, addrlen, tcp_connected,
			    K_NO_WAIT, NULL);
}

static void tcp_sent_cb(struct net_context *context,
			int status, void *user_data)
{
	PR_SHELL(tcp_shell, "Message sent\n");
}
#endif

static int cmd_net_tcp_connect(const struct shell *shell, size_t argc,
			       char *argv[])
{
#if defined(CONFIG_NET_TCP1) && defined(CONFIG_NET_NATIVE_TCP)
	int arg = 0;

	/* tcp connect <ip> port */
	char *endptr;
	char *ip;
	u16_t port;

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

	tcp_connect(shell, ip, port, &tcp_ctx);
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_TCP and CONFIG_NET_NATIVE", "TCP");
#endif /* CONFIG_NET_NATIVE_TCP */

	return 0;
}

static int cmd_net_tcp_send(const struct shell *shell, size_t argc,
			    char *argv[])
{
#if defined(CONFIG_NET_TCP1) && defined(CONFIG_NET_NATIVE_TCP)
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

	user_data.shell = shell;

	ret = net_context_send(tcp_ctx, (u8_t *)argv[arg],
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

static int cmd_net_tcp_close(const struct shell *shell, size_t argc,
			     char *argv[])
{
#if defined(CONFIG_NET_TCP1) && defined(CONFIG_NET_NATIVE_TCP)
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

static int cmd_net_tcp(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return 0;
}

#if defined(CONFIG_NET_VLAN)
static void iface_vlan_del_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *shell = data->shell;
	u16_t vlan_tag = POINTER_TO_UINT(data->user_data);
	int ret;

	ret = net_eth_vlan_disable(iface, vlan_tag);
	if (ret < 0) {
		if (ret != -ESRCH) {
			PR_WARNING("Cannot delete VLAN tag %d from "
				   "interface %p\n",
				   vlan_tag, iface);
		}

		return;
	}

	PR("VLAN tag %d removed from interface %p\n", vlan_tag, iface);
}

static void iface_vlan_cb(struct net_if *iface, void *user_data)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	struct net_shell_user_data *data = user_data;
	const struct shell *shell = data->shell;
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

static int cmd_net_vlan(const struct shell *shell, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_VLAN)
	struct net_shell_user_data user_data;
	int count;
#endif

#if defined(CONFIG_NET_VLAN)
	count = 0;

	user_data.shell = shell;
	user_data.user_data = &count;

	net_if_foreach(iface_vlan_cb, &user_data);
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_VLAN", "VLAN");
#endif /* CONFIG_NET_VLAN */

	return 0;
}

static int cmd_net_vlan_add(const struct shell *shell, size_t argc,
			    char *argv[])
{
#if defined(CONFIG_NET_VLAN)
	int arg = 0;
	int ret;
	u16_t tag;
	struct net_if *iface;
	char *endptr;
	u32_t iface_idx;
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
		PR_WARNING("Network interface %p is not ethernet interface\n",
			   iface);
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

	PR("VLAN tag %d set to interface %p\n", tag, iface);

	return 0;

usage:
	PR("Usage:\n");
	PR("\tvlan add <tag> <interface index>\n");
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_VLAN", "VLAN");
#endif /* CONFIG_NET_VLAN */

	return 0;
}

static int cmd_net_vlan_del(const struct shell *shell, size_t argc,
			    char *argv[])
{
#if defined(CONFIG_NET_VLAN)
	int arg = 0;
	struct net_shell_user_data user_data;
	char *endptr;
	u16_t tag;
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

	user_data.shell = shell;
	user_data.user_data = UINT_TO_POINTER((u32_t)tag);

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
	const struct shell *shell = data->shell;
	struct net_context *net_ctx;
	int *count = data->user_data;
	/* +7 for []:port */
	char addr_local[ADDR_LEN + 7];
	char addr_remote[ADDR_LEN + 7] = "";

	net_ctx = z_get_fd_obj(context->real_sock, NULL, 0);
	if (net_ctx == NULL) {
		PR_ERROR("Invalid fd %d");
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

static int cmd_net_websocket(const struct shell *shell, size_t argc,
			     char *argv[])
{
#if defined(CONFIG_WEBSOCKET_CLIENT)
	struct net_shell_user_data user_data;
	int count = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR("     websocket/net_ctx\tIface         "
	   "Local              \tRemote\n");

	user_data.shell = shell;
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

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_dns,
	SHELL_CMD(cancel, NULL, "Cancel all pending requests.",
		  cmd_net_dns_cancel),
	SHELL_CMD(query, NULL,
		  "'net dns <hostname> [A or AAAA]' queries IPv4 address "
		  "(default) or IPv6 address for a host name.",
		  cmd_net_dns_query),
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
		  "'net ping [-c count] [-i interval ms] <host>' "
		  "Send ICMPv4 or ICMPv6 Echo-Request to a network host.",
		  cmd_net_ping),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_commands,
	SHELL_CMD(allocs, NULL, "Print network memory allocations.",
		  cmd_net_allocs),
	SHELL_CMD(arp, &net_cmd_arp, "Print information about IPv4 ARP cache.",
		  cmd_net_arp),
	SHELL_CMD(conn, NULL, "Print information about network connections.",
		  cmd_net_conn),
	SHELL_CMD(dns, &net_cmd_dns, "Show how DNS is configured.",
		  cmd_net_dns),
	SHELL_CMD(gptp, &net_cmd_gptp, "Print information about gPTP support.",
		  cmd_net_gptp),
	SHELL_CMD(iface, &net_cmd_iface,
		  "Print information about network interfaces.",
		  cmd_net_iface),
	SHELL_CMD(ipv6, NULL,
		  "Print information about IPv6 specific information and "
		  "configuration.",
		  cmd_net_ipv6),
	SHELL_CMD(mem, NULL, "Print information about network memory usage.",
		  cmd_net_mem),
	SHELL_CMD(nbr, &net_cmd_nbr, "Print neighbor information.",
		  cmd_net_nbr),
	SHELL_CMD(ping, &net_cmd_ping, "Ping a network host.", cmd_net_ping),
	SHELL_CMD(ppp, &net_cmd_ppp, "PPP information.", cmd_net_ppp_status),
	SHELL_CMD(route, NULL, "Show network route.", cmd_net_route),
	SHELL_CMD(stacks, NULL, "Show network stacks information.",
		  cmd_net_stacks),
	SHELL_CMD(stats, &net_cmd_stats, "Show network statistics.",
		  cmd_net_stats),
	SHELL_CMD(tcp, &net_cmd_tcp, "Connect/send/close TCP connection.",
		  cmd_net_tcp),
	SHELL_CMD(vlan, &net_cmd_vlan, "Show VLAN information.", cmd_net_vlan),
	SHELL_CMD(websocket, NULL, "Print information about WebSocket "
								"connections.",
		  cmd_net_websocket),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(net, &net_commands, "Networking commands", NULL);

int net_shell_init(void)
{
	return 0;
}
