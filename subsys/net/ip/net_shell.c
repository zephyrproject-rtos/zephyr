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

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <shell/shell.h>

#include <net/net_if.h>
#include <net/dns_resolve.h>
#include <misc/printk.h>

#include "route.h"
#include "icmpv6.h"
#include "icmpv4.h"
#include "connection.h"

#if defined(CONFIG_NET_TCP)
#include <net/tcp.h>
#include "tcp_internal.h"
#endif

#if defined(CONFIG_NET_IPV6)
#include "ipv6.h"
#endif

#if defined(CONFIG_HTTP)
#include <net/http.h>
#endif

#if defined(CONFIG_NET_APP)
#include <net/net_app.h>
#endif

#if defined(CONFIG_NET_RPL)
#include "rpl.h"
#endif

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

#include "net_shell.h"
#include "net_stats.h"

/*
 * Set NET_LOG_ENABLED in order to activate address printing functions
 * in net_private.h
 */
#define NET_LOG_ENABLED 1
#include "net_private.h"

#define NET_SHELL_MODULE "net"

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

#ifdef CONFIG_NET_L2_DUMMY
	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		if (extra) {
			*extra = "=====";
		}

		return "Dummy";
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

	if (extra) {
		*extra = "==============";
	}

	return "<unknown type>";
}

#if defined(CONFIG_NET_L2_ETHERNET)
struct ethernet_capabilities {
	enum ethernet_hw_caps capability;
	const char * const description;
};

#define EC(cap, desc) { .capability = cap, .description = desc }

static struct ethernet_capabilities eth_hw_caps[] = {
	EC(ETHERNET_HW_TX_CHKSUM_OFFLOAD, "TX checksum offload"),
	EC(ETHERNET_HW_RX_CHKSUM_OFFLOAD, "RX checksum offload"),
	EC(ETHERNET_HW_VLAN,              "Virtual LAN"),
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

static void print_supported_ethernet_capabilities(struct net_if *iface)
{
	enum ethernet_hw_caps caps = net_eth_get_hw_capabilities(iface);
	int i;

	for (i = 0; i < ARRAY_SIZE(eth_hw_caps); i++) {
		if (caps & eth_hw_caps[i].capability) {
			printk("\t%s\n", eth_hw_caps[i].description);
		}
	}
}
#endif /* CONFIG_NET_L2_ETHERNET */

static void iface_cb(struct net_if *iface, void *user_data)
{
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
	struct net_if_addr *unicast;
	struct net_if_mcast_addr *mcast;
#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
	struct ethernet_req_params params;
	int ret;
#endif
	const char *extra;
	int i, count;

	ARG_UNUSED(user_data);

	printk("\nInterface %p (%s) [%d]\n", iface, iface2str(iface, &extra),
	       net_if_get_by_iface(iface));
	printk("===========================%s\n", extra);

	if (!net_if_is_up(iface)) {
		printk("Interface is down.\n");
		return;
	}

	if (net_if_get_link_addr(iface) &&
	    net_if_get_link_addr(iface)->addr) {
		printk("Link addr : %s\n",
		       net_sprint_ll_addr(net_if_get_link_addr(iface)->addr,
					  net_if_get_link_addr(iface)->len));
	}

	printk("MTU       : %d\n", net_if_get_mtu(iface));

#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
	count = 0;
	ret = net_mgmt(NET_REQUEST_ETHERNET_GET_PRIORITY_QUEUES_NUM,
		       iface,
		       &params, sizeof(struct ethernet_req_params));

	if (!ret && params.priority_queues_num) {
		count = params.priority_queues_num;
		printk("Priority queues:\n");
		for (i = 0; i < count; ++i) {
			params.qav_param.queue_id = i;
			params.qav_param.type = ETHERNET_QAV_PARAM_TYPE_STATUS;
			ret = net_mgmt(NET_REQUEST_ETHERNET_GET_QAV_PARAM,
				       iface,
				       &params,
				       sizeof(struct ethernet_req_params));

			printk("\t%d: Qav ", i);
			if (ret) {
				printk("not supported\n");
			} else {
				printk("%s\n",
				       params.qav_param.enabled ?
				       "enabled" :
				       "disabled");
			}
		}
	}
#endif

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	printk("Promiscuous mode : %s\n",
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

				printk("VLAN tag  : %d (0x%x)\n",
				       eth_ctx->vlan[i].tag,
				       eth_ctx->vlan[i].tag);
			}
		} else {
			printk("VLAN not enabled\n");
		}
	}
#endif

#ifdef CONFIG_NET_L2_ETHERNET
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		printk("Ethernet capabilities supported:\n");
		print_supported_ethernet_capabilities(iface);
	}
#endif /* CONFIG_NET_L2_ETHERNET */

#if defined(CONFIG_NET_IPV6)
	count = 0;

	ipv6 = iface->config.ip.ipv6;

	printk("IPv6 unicast addresses (max %d):\n", NET_IF_MAX_IPV6_ADDR);
	for (i = 0; ipv6 && i < NET_IF_MAX_IPV6_ADDR; i++) {
		unicast = &ipv6->unicast[i];

		if (!unicast->is_used) {
			continue;
		}

		printk("\t%s %s %s%s\n",
		       net_sprint_ipv6_addr(&unicast->address.in6_addr),
		       addrtype2str(unicast->addr_type),
		       addrstate2str(unicast->addr_state),
		       unicast->is_infinite ? " infinite" : "");
		count++;
	}

	if (count == 0) {
		printk("\t<none>\n");
	}

	count = 0;

	printk("IPv6 multicast addresses (max %d):\n", NET_IF_MAX_IPV6_MADDR);
	for (i = 0; ipv6 && i < NET_IF_MAX_IPV6_MADDR; i++) {
		mcast = &ipv6->mcast[i];

		if (!mcast->is_used) {
			continue;
		}

		printk("\t%s\n",
		       net_sprint_ipv6_addr(&mcast->address.in6_addr));

		count++;
	}

	if (count == 0) {
		printk("\t<none>\n");
	}

	count = 0;

	printk("IPv6 prefixes (max %d):\n", NET_IF_MAX_IPV6_PREFIX);
	for (i = 0; ipv6 && i < NET_IF_MAX_IPV6_PREFIX; i++) {
		prefix = &ipv6->prefix[i];

		if (!prefix->is_used) {
			continue;
		}

		printk("\t%s/%d%s\n",
		       net_sprint_ipv6_addr(&prefix->prefix),
		       prefix->len,
		       prefix->is_infinite ? " infinite" : "");

		count++;
	}

	if (count == 0) {
		printk("\t<none>\n");
	}

	router = net_if_ipv6_router_find_default(iface, NULL);
	if (router) {
		printk("IPv6 default router :\n");
		printk("\t%s%s\n",
		       net_sprint_ipv6_addr(&router->address.in6_addr),
		       router->is_infinite ? " infinite" : "");
	}

	if (ipv6) {
		printk("IPv6 hop limit           : %d\n",
		       ipv6->hop_limit);
		printk("IPv6 base reachable time : %d\n",
		       ipv6->base_reachable_time);
		printk("IPv6 reachable time      : %d\n",
		       ipv6->reachable_time);
		printk("IPv6 retransmit timer    : %d\n",
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
		printk("IPv4 not supported for this interface.\n");
		return;
	}

	count = 0;

	ipv4 = iface->config.ip.ipv4;

	printk("IPv4 unicast addresses (max %d):\n", NET_IF_MAX_IPV4_ADDR);
	for (i = 0; ipv4 && i < NET_IF_MAX_IPV4_ADDR; i++) {
		unicast = &ipv4->unicast[i];

		if (!unicast->is_used) {
			continue;
		}

		printk("\t%s %s %s%s\n",
		       net_sprint_ipv4_addr(&unicast->address.in_addr),
		       addrtype2str(unicast->addr_type),
		       addrstate2str(unicast->addr_state),
		       unicast->is_infinite ? " infinite" : "");

		count++;
	}

	if (count == 0) {
		printk("\t<none>\n");
	}

	count = 0;

	printk("IPv4 multicast addresses (max %d):\n", NET_IF_MAX_IPV4_MADDR);
	for (i = 0; ipv4 && i < NET_IF_MAX_IPV4_MADDR; i++) {
		mcast = &ipv4->mcast[i];

		if (!mcast->is_used) {
			continue;
		}

		printk("\t%s\n",
		       net_sprint_ipv4_addr(&mcast->address.in_addr));

		count++;
	}

	if (count == 0) {
		printk("\t<none>\n");
	}

	if (ipv4) {
		printk("IPv4 gateway : %s\n",
		       net_sprint_ipv4_addr(&ipv4->gw));
		printk("IPv4 netmask : %s\n",
		       net_sprint_ipv4_addr(&ipv4->netmask));
	}
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_DHCPV4)
	printk("DHCPv4 lease time : %u\n",
	       iface->config.dhcpv4.lease_time);
	printk("DHCPv4 renew time : %u\n",
	       iface->config.dhcpv4.renewal_time);
	printk("DHCPv4 server     : %s\n",
	       net_sprint_ipv4_addr(&iface->config.dhcpv4.server_id));
	printk("DHCPv4 requested  : %s\n",
	       net_sprint_ipv4_addr(&iface->config.dhcpv4.requested_ip));
	printk("DHCPv4 state      : %s\n",
	       net_dhcpv4_state_name(iface->config.dhcpv4.state));
	printk("DHCPv4 attempts   : %d\n",
	       iface->config.dhcpv4.attempts);
#endif /* CONFIG_NET_DHCPV4 */
}

#if defined(CONFIG_NET_ROUTE)
static void route_cb(struct net_route_entry *entry, void *user_data)
{
	struct net_if *iface = user_data;
	struct net_route_nexthop *nexthop_route;
	int count;

	if (entry->iface != iface) {
		return;
	}

	printk("IPv6 prefix : %s/%d\n",
	       net_sprint_ipv6_addr(&entry->addr),
	       entry->prefix_len);

	count = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&entry->nexthop, nexthop_route, node) {
		struct net_linkaddr_storage *lladdr;

		if (!nexthop_route->nbr) {
			continue;
		}

		printk("\tneighbor : %p\t", nexthop_route->nbr);

		if (nexthop_route->nbr->idx == NET_NBR_LLADDR_UNKNOWN) {
			printk("addr : <unknown>\n");
		} else {
			lladdr = net_nbr_get_lladdr(nexthop_route->nbr->idx);

			printk("addr : %s\n",
			       net_sprint_ll_addr(lladdr->addr,
						  lladdr->len));
		}

		count++;
	}

	if (count == 0) {
		printk("\t<none>\n");
	}
}

static void iface_per_route_cb(struct net_if *iface, void *user_data)
{
	const char *extra;

	ARG_UNUSED(user_data);

	printk("\nIPv6 routes for interface %p (%s)\n", iface,
	       iface2str(iface, &extra));
	printk("=======================================%s\n", extra);

	net_route_foreach(route_cb, iface);
}
#endif /* CONFIG_NET_ROUTE */

#if defined(CONFIG_NET_ROUTE_MCAST)
static void route_mcast_cb(struct net_route_entry_mcast *entry,
			   void *user_data)
{
	struct net_if *iface = user_data;
	const char *extra;

	if (entry->iface != iface) {
		return;
	}

	printk("IPv6 multicast route %p for interface %p (%s)\n", entry,
	       iface, iface2str(iface, &extra));
	printk("==========================================================="
	       "%s\n", extra);

	printk("IPv6 group : %s\n", net_sprint_ipv6_addr(&entry->group));
	printk("Lifetime   : %u\n", entry->lifetime);
}

static void iface_per_mcast_route_cb(struct net_if *iface, void *user_data)
{
	net_route_mcast_foreach(route_mcast_cb, NULL, iface);
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
static void print_eth_stats(struct net_if *iface, struct net_stats_eth *data)
{
	printk("Statistics for Ethernet interface %p [%d]\n", iface,
	       net_if_get_by_iface(iface));

	printk("Bytes received   : %u\n", data->bytes.received);
	printk("Bytes sent       : %u\n", data->bytes.sent);
	printk("Packets received : %u\n", data->pkts.rx);
	printk("Packets sent     : %u\n", data->pkts.tx);
	printk("Bcast received   : %u\n", data->broadcast.rx);
	printk("Bcast sent       : %u\n", data->broadcast.tx);
	printk("Mcast received   : %u\n", data->multicast.rx);
	printk("Mcast sent       : %u\n", data->multicast.tx);

#if defined(CONFIG_NET_STATISTICS_ETHERNET_VENDOR)
	if (data->vendor) {
		printk("Vendor specific statistics for Ethernet interface %p [%d]:\n",
			iface, net_if_get_by_iface(iface));
		size_t i = 0;

		do {
			printk("%s : %u\n", data->vendor[i].key,
				data->vendor[i].value);
			i++;
		} while (data->vendor[i].key);
	}
#endif /* CONFIG_NET_STATISTICS_ETHERNET_VENDOR */
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET && CONFIG_NET_STATISTICS_USER_API */

static void net_shell_print_statistics(struct net_if *iface, void *user_data)
{
	ARG_UNUSED(user_data);

	if (iface) {
		const char *extra;

		printk("\nInterface %p (%s) [%d]\n", iface,
		       iface2str(iface, &extra),
		       net_if_get_by_iface(iface));
		printk("===========================%s\n", extra);
	} else {
		printk("\nGlobal statistics\n");
		printk("=================\n");
	}

#if defined(CONFIG_NET_IPV6)
	printk("IPv6 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
	       GET_STAT(iface, ipv6.recv),
	       GET_STAT(iface, ipv6.sent),
	       GET_STAT(iface, ipv6.drop),
	       GET_STAT(iface, ipv6.forwarded));
#if defined(CONFIG_NET_IPV6_ND)
	printk("IPv6 ND recv   %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, ipv6_nd.recv),
	       GET_STAT(iface, ipv6_nd.sent),
	       GET_STAT(iface, ipv6_nd.drop));
#endif /* CONFIG_NET_IPV6_ND */
#if defined(CONFIG_NET_STATISTICS_MLD)
	printk("IPv6 MLD recv  %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, ipv6_mld.recv),
	       GET_STAT(iface, ipv6_mld.sent),
	       GET_STAT(iface, ipv6_mld.drop));
#endif /* CONFIG_NET_STATISTICS_MLD */
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	printk("IPv4 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
	       GET_STAT(iface, ipv4.recv),
	       GET_STAT(iface, ipv4.sent),
	       GET_STAT(iface, ipv4.drop),
	       GET_STAT(iface, ipv4.forwarded));
#endif /* CONFIG_NET_IPV4 */

	printk("IP vhlerr      %d\thblener\t%d\tlblener\t%d\n",
	       GET_STAT(iface, ip_errors.vhlerr),
	       GET_STAT(iface, ip_errors.hblenerr),
	       GET_STAT(iface, ip_errors.lblenerr));
	printk("IP fragerr     %d\tchkerr\t%d\tprotoer\t%d\n",
	       GET_STAT(iface, ip_errors.fragerr),
	       GET_STAT(iface, ip_errors.chkerr),
	       GET_STAT(iface, ip_errors.protoerr));

	printk("ICMP recv      %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, icmp.recv),
	       GET_STAT(iface, icmp.sent),
	       GET_STAT(iface, icmp.drop));
	printk("ICMP typeer    %d\tchkerr\t%d\n",
	       GET_STAT(iface, icmp.typeerr),
	       GET_STAT(iface, icmp.chkerr));

#if defined(CONFIG_NET_UDP)
	printk("UDP recv       %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, udp.recv),
	       GET_STAT(iface, udp.sent),
	       GET_STAT(iface, udp.drop));
	printk("UDP chkerr     %d\n",
	       GET_STAT(iface, udp.chkerr));
#endif

#if defined(CONFIG_NET_STATISTICS_TCP)
	printk("TCP bytes recv %u\tsent\t%d\n",
	       GET_STAT(iface, tcp.bytes.received),
	       GET_STAT(iface, tcp.bytes.sent));
	printk("TCP seg recv   %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, tcp.recv),
	       GET_STAT(iface, tcp.sent),
	       GET_STAT(iface, tcp.drop));
	printk("TCP seg resent %d\tchkerr\t%d\tackerr\t%d\n",
	       GET_STAT(iface, tcp.resent),
	       GET_STAT(iface, tcp.chkerr),
	       GET_STAT(iface, tcp.ackerr));
	printk("TCP seg rsterr %d\trst\t%d\tre-xmit\t%d\n",
	       GET_STAT(iface, tcp.rsterr),
	       GET_STAT(iface, tcp.rst),
	       GET_STAT(iface, tcp.rexmit));
	printk("TCP conn drop  %d\tconnrst\t%d\n",
	       GET_STAT(iface, tcp.conndrop),
	       GET_STAT(iface, tcp.connrst));
#endif

#if defined(CONFIG_NET_STATISTICS_RPL)
	printk("RPL DIS recv   %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, rpl.dis.recv),
	       GET_STAT(iface, rpl.dis.sent),
	       GET_STAT(iface, rpl.dis.drop));
	printk("RPL DIO recv   %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, rpl.dio.recv),
	       GET_STAT(iface, rpl.dio.sent),
	       GET_STAT(iface, rpl.dio.drop));
	printk("RPL DAO recv   %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
	       GET_STAT(iface, rpl.dao.recv),
	       GET_STAT(iface, rpl.dao.sent),
	       GET_STAT(iface, rpl.dao.drop),
	      GET_STAT(iface, rpl.dao.forwarded));
	printk("RPL DAOACK rcv %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, rpl.dao_ack.recv),
	       GET_STAT(iface, rpl.dao_ack.sent),
	       GET_STAT(iface, rpl.dao_ack.drop));
	printk("RPL overflows  %d\tl-repairs\t%d\tg-repairs\t%d\n",
	       GET_STAT(iface, rpl.mem_overflows),
	       GET_STAT(iface, rpl.local_repairs),
	       GET_STAT(iface, rpl.global_repairs));
	printk("RPL malformed  %d\tresets   \t%d\tp-switch\t%d\n",
	       GET_STAT(iface, rpl.malformed_msgs),
	       GET_STAT(iface, rpl.resets),
	       GET_STAT(iface, rpl.parent_switch));
	printk("RPL f-errors   %d\tl-errors\t%d\tl-warnings\t%d\n",
	       GET_STAT(iface, rpl.forward_errors),
	       GET_STAT(iface, rpl.loop_errors),
	       GET_STAT(iface, rpl.loop_warnings));
	printk("RPL r-repairs  %d\n",
	       GET_STAT(iface, rpl.root_repairs));
#endif

	printk("Bytes received %u\n", GET_STAT(iface, bytes.received));
	printk("Bytes sent     %u\n", GET_STAT(iface, bytes.sent));
	printk("Processing err %d\n", GET_STAT(iface, processing_error));

#if NET_TC_COUNT > 1
	{
		int i;

#if NET_TC_TX_COUNT > 1
		printk("TX traffic class statistics:\n");
		printk("TC  Priority\tSent pkts\tbytes\n");

		for (i = 0; i < NET_TC_TX_COUNT; i++) {
			printk("[%d] %s (%d)\t%d\t\t%d\n", i,
			       priority2str(GET_STAT(iface,
						    tc.sent[i].priority)),
			       GET_STAT(iface, tc.sent[i].priority),
			       GET_STAT(iface, tc.sent[i].pkts),
			       GET_STAT(iface, tc.sent[i].bytes));
		}
#endif

#if NET_TC_RX_COUNT > 1
		printk("RX traffic class statistics:\n");
		printk("TC  Priority\tRecv pkts\tbytes\n");

		for (i = 0; i < NET_TC_RX_COUNT; i++) {
			printk("[%d] %s (%d)\t%d\t\t%d\n", i,
			       priority2str(GET_STAT(iface,
						    tc.recv[i].priority)),
			       GET_STAT(iface, tc.recv[i].priority),
			       GET_STAT(iface, tc.recv[i].pkts),
			       GET_STAT(iface, tc.recv[i].bytes));
		}
	}
#endif
#endif /* NET_TC_COUNT > 1 */

#if defined(CONFIG_NET_STATISTICS_ETHERNET) && \
					defined(CONFIG_NET_STATISTICS_USER_API)
	if (iface && net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		struct net_stats_eth eth_data;
		int ret;

		ret = net_mgmt(NET_REQUEST_STATS_GET_ETHERNET, iface,
			       &eth_data, sizeof(eth_data));
		if (!ret) {
			print_eth_stats(iface, &eth_data);
		}
	}
#endif /* CONFIG_NET_STATISTICS_ETHERNET && CONFIG_NET_STATISTICS_USER_API */
}
#endif /* CONFIG_NET_STATISTICS */

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

	int *count = user_data;
	/* +7 for []:port */
	char addr_local[ADDR_LEN + 7];
	char addr_remote[ADDR_LEN + 7] = "";

	get_addresses(context, addr_local, sizeof(addr_local),
		      addr_remote, sizeof(addr_remote));

	printk("[%2d] %p\t%p    %c%c%c   %16s\t%16s\n",
	       (*count) + 1, context,
	       net_context_get_iface(context),
	       net_context_get_family(context) == AF_INET6 ? '6' : '4',
	       net_context_get_type(context) == SOCK_DGRAM ? 'D' : 'S',
	       net_context_get_ip_proto(context) == IPPROTO_UDP ?
							     'U' : 'T',
	       addr_local, addr_remote);

	(*count)++;
}

#if defined(CONFIG_NET_DEBUG_CONN)
static void conn_handler_cb(struct net_conn *conn, void *user_data)
{
#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
#define ADDR_LEN NET_IPV6_ADDR_LEN
#elif defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
#define ADDR_LEN NET_IPV4_ADDR_LEN
#else
#define ADDR_LEN NET_IPV6_ADDR_LEN
#endif

	int *count = user_data;
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

	printk("[%2d] %p %p\t%s\t%16s\t%16s\n",
	       (*count) + 1, conn, conn->cb, net_proto2str(conn->proto),
	       addr_local, addr_remote);

	(*count)++;
}
#endif /* CONFIG_NET_DEBUG_CONN */

#if defined(CONFIG_NET_TCP)
static void tcp_cb(struct net_tcp *tcp, void *user_data)
{
	int *count = user_data;
	u16_t recv_mss = net_tcp_get_recv_mss(tcp);

	printk("%p %p   %5u    %5u %10u %10u %5u   %s\n",
	       tcp, tcp->context,
	       ntohs(net_sin6_ptr(&tcp->context->local)->sin6_port),
	       ntohs(net_sin6(&tcp->context->remote)->sin6_port),
	       tcp->send_seq, tcp->send_ack, recv_mss,
	       net_tcp_state_str(net_tcp_get_state(tcp)));

	(*count)++;
}

#if defined(CONFIG_NET_DEBUG_TCP)
static void tcp_sent_list_cb(struct net_tcp *tcp, void *user_data)
{
	int *printed = user_data;
	struct net_pkt *pkt;
	struct net_pkt *tmp;

	if (sys_slist_is_empty(&tcp->sent_list)) {
		return;
	}

	if (!*printed) {
		printk("\nTCP packets waiting ACK:\n");
		printk("TCP             net_pkt[ref/totlen]->net_buf[ref/len]...\n");
	}

	printk("%p      ", tcp);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&tcp->sent_list, pkt, tmp,
					  sent_list) {
		struct net_buf *frag = pkt->frags;

		if (!*printed) {
			printk("%p[%d/%zd]", pkt, pkt->ref,
			       net_pkt_get_len(pkt));
			*printed = true;
		} else {
			printk("                %p[%d/%zd]", pkt, pkt->ref,
			       net_pkt_get_len(pkt));
		}

		if (frag) {
			printk("->");
		}

		while (frag) {
			printk("%p[%d/%d]", frag, frag->ref, frag->len);

			frag = frag->frags;
			if (frag) {
				printk("->");
			}
		}

		printk("\n");
	}

	*printed = true;
}
#endif /* CONFIG_NET_DEBUG_TCP */
#endif

#if defined(CONFIG_NET_IPV6_FRAGMENT)
static void ipv6_frag_cb(struct net_ipv6_reassembly *reass,
			 void *user_data)
{
	int *count = user_data;
	char src[ADDR_LEN];
	int i;

	if (!*count) {
		printk("\nIPv6 reassembly Id         Remain Src             \tDst\n");
	}

	snprintk(src, ADDR_LEN, "%s", net_sprint_ipv6_addr(&reass->src));

	printk("%p      0x%08x  %5d %16s\t%16s\n",
	       reass, reass->id, k_delayed_work_remaining_get(&reass->timer),
	       src, net_sprint_ipv6_addr(&reass->dst));

	for (i = 0; i < NET_IPV6_FRAGMENTS_MAX_PKT; i++) {
		if (reass->pkt[i]) {
			struct net_buf *frag = reass->pkt[i]->frags;

			printk("[%d] pkt %p->", i, reass->pkt[i]);

			while (frag) {
				printk("%p", frag);

				frag = frag->frags;
				if (frag) {
					printk("->");
				}
			}

			printk("\n");
		}
	}

	(*count)++;
}
#endif /* CONFIG_NET_IPV6_FRAGMENT */

#if defined(CONFIG_NET_DEBUG_NET_PKT)
static void allocs_cb(struct net_pkt *pkt,
		      struct net_buf *buf,
		      const char *func_alloc,
		      int line_alloc,
		      const char *func_free,
		      int line_free,
		      bool in_use,
		      void *user_data)
{
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
			printk("%p/%d\t%5s\t%5s\t%s():%d\n", pkt, pkt->ref,
			       str, net_pkt_slab2str(pkt->slab), func_alloc,
			       line_alloc);
		} else {
			printk("%p\t%5s\t%5s\t%s():%d -> %s():%d\n", pkt,
			       str, net_pkt_slab2str(pkt->slab), func_alloc,
			       line_alloc, func_free, line_free);
		}
	}

	return;
buf:
	if (func_alloc) {
		struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

		if (in_use) {
			printk("%p/%d\t%5s\t%5s\t%s():%d\n", buf, buf->ref,
			       str, net_pkt_pool2str(pool), func_alloc,
			       line_alloc);
		} else {
			printk("%p\t%5s\t%5s\t%s():%d -> %s():%d\n", buf,
			       str, net_pkt_pool2str(pool), func_alloc,
			       line_alloc, func_free, line_free);
		}
	}
}
#endif /* CONFIG_NET_DEBUG_NET_PKT */

/* Put the actual shell commands after this */

int net_shell_cmd_allocs(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_DEBUG_NET_PKT)
	printk("Network memory allocations\n\n");
	printk("memory\t\tStatus\tPool\tFunction alloc -> freed\n");
	net_pkt_allocs_foreach(allocs_cb, NULL);
#else
	printk("Enable CONFIG_NET_DEBUG_NET_PKT to see allocations.\n");
#endif /* CONFIG_NET_DEBUG_NET_PKT */

	return 0;
}

#if defined(CONFIG_NET_DEBUG_APP) && \
	(defined(CONFIG_NET_APP_SERVER) || defined(CONFIG_NET_APP_CLIENT))

#if defined(CONFIG_NET_APP_TLS) || defined(CONFIG_NET_APP_DTLS)
static void print_app_sec_info(struct net_app_ctx *ctx, const char *sec_type)
{
	printk("     Security: %s  Thread id: %p\n", sec_type, ctx->tls.tid);

#if defined(CONFIG_INIT_STACKS)
	{
		unsigned int pcnt, unused;

		net_analyze_stack_get_values(
			K_THREAD_STACK_BUFFER(ctx->tls.stack),
			ctx->tls.stack_size,
			&pcnt, &unused);
		printk("     Stack: %p  Size: %d bytes unused %u usage "
		       "%u/%d (%u %%)\n",
		       ctx->tls.stack, ctx->tls.stack_size,
		       unused, ctx->tls.stack_size - unused,
		       ctx->tls.stack_size, pcnt);
	}
#endif /* CONFIG_INIT_STACKS */

	if (ctx->tls.cert_host) {
		printk("     Cert host: %s\n", ctx->tls.cert_host);
	}
}
#endif /* CONFIG_NET_APP_TLS || CONFIG_NET_APP_DTLS */

static void net_app_cb(struct net_app_ctx *ctx, void *user_data)
{
	int *count = user_data;
	char *sec_type = "none";
	char *app_type = "unknown";
	char *proto = "unknown";
	bool printed = false;

#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
#define ADDR_LEN NET_IPV6_ADDR_LEN
#elif defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
#define ADDR_LEN NET_IPV4_ADDR_LEN
#else
#define ADDR_LEN NET_IPV6_ADDR_LEN
#endif
	/* +7 for []:port */
	char addr_local[ADDR_LEN + 7];
	char addr_remote[ADDR_LEN + 7] = "";

	if (*count == 0) {
		if (ctx->app_type == NET_APP_SERVER) {
			printk("Network application server instances\n\n");
		} else if (ctx->app_type == NET_APP_CLIENT) {
			printk("Network application client instances\n\n");
		} else {
			printk("Invalid network application type %d\n",
			       ctx->app_type);
		}
	}

	if (IS_ENABLED(CONFIG_NET_APP_TLS) && ctx->is_tls) {
		if (ctx->sock_type == SOCK_STREAM) {
			sec_type = "TLS";
		}
	}

	if (IS_ENABLED(CONFIG_NET_APP_DTLS) && ctx->is_tls) {
		if (ctx->sock_type == SOCK_DGRAM) {
			sec_type = "DTLS";
		}
	}

	if (ctx->app_type == NET_APP_SERVER) {
		app_type = "server";
	} else if (ctx->app_type == NET_APP_CLIENT) {
		app_type = "client";
	}

	if (ctx->proto == IPPROTO_UDP) {
#if defined(CONFIG_NET_UDP)
		proto = "UDP";
#else
		proto = "<UDP not configured>";
#endif
	}

	if (ctx->proto == IPPROTO_TCP) {
#if defined(CONFIG_NET_TCP)
		proto = "TCP";
#else
		proto = "<TCP not configured>";
#endif
	}

	printk("[%2d] App-ctx: %p  Status: %s  Type: %s  Protocol: %s\n",
	       *count, ctx, ctx->is_enabled ? "enabled" : "disabled",
	       app_type, proto);

#if defined(CONFIG_NET_APP_TLS) || defined(CONFIG_NET_APP_DTLS)
	if (ctx->is_tls) {
		print_app_sec_info(ctx, sec_type);
	}
#endif /* CONFIG_NET_APP_TLS || CONFIG_NET_APP_DTLS */

#if defined(CONFIG_NET_IPV6)
	if (ctx->app_type == NET_APP_SERVER) {
		if (ctx->ipv6.ctx && ctx->ipv6.ctx->local.family == AF_INET6) {
			get_addresses(ctx->ipv6.ctx,
				      addr_local, sizeof(addr_local),
				      addr_remote, sizeof(addr_remote));

			printk("     Listen IPv6: %16s <- %16s\n",
			       addr_local, addr_remote);
		} else {
			printk("     Not listening IPv6 connections.\n");
		}
	} else if (ctx->app_type == NET_APP_CLIENT) {
		if (ctx->ipv6.ctx && ctx->ipv6.ctx->local.family == AF_INET6) {
			get_addresses(ctx->ipv6.ctx,
				      addr_local, sizeof(addr_local),
				      addr_remote, sizeof(addr_remote));

			printk("     Connect IPv6: %16s -> %16s\n",
			       addr_local, addr_remote);
		}
	} else {
		printk("Invalid application type %d\n", ctx->app_type);
		printed = true;
	}
#else
	printk("     IPv6 connections not enabled.\n");
#endif

#if defined(CONFIG_NET_IPV4)
	if (ctx->app_type == NET_APP_SERVER) {
		if (ctx->ipv4.ctx && ctx->ipv4.ctx->local.family == AF_INET) {
			get_addresses(ctx->ipv4.ctx,
				      addr_local, sizeof(addr_local),
				      addr_remote, sizeof(addr_remote));

			printk("     Listen IPv4: %16s <- %16s\n",
			       addr_local, addr_remote);
		} else {
			printk("     Not listening IPv4 connections.\n");
		}
	} else if (ctx->app_type == NET_APP_CLIENT) {
		if (ctx->ipv4.ctx && ctx->ipv4.ctx->local.family == AF_INET) {
			get_addresses(ctx->ipv4.ctx,
				      addr_local, sizeof(addr_local),
				      addr_remote, sizeof(addr_remote));

			printk("     Connect IPv4: %16s -> %16s\n",
			       addr_local, addr_remote);
		}
	} else {
		if (!printed) {
			printk("Invalid application type %d\n", ctx->app_type);
		}
	}
#else
	printk("     IPv4 connections not enabled.\n");
#endif

#if defined(CONFIG_NET_APP_SERVER)
#if defined(CONFIG_NET_TCP)
	{
		int i, found = 0;

		for (i = 0; i < CONFIG_NET_APP_SERVER_NUM_CONN; i++) {
			if (!ctx->server.net_ctxs[i] ||
			    !net_context_is_used(ctx->server.net_ctxs[i])) {
				continue;
			}

			get_addresses(ctx->server.net_ctxs[i],
				      addr_local, sizeof(addr_local),
				      addr_remote, sizeof(addr_remote));

			printk("     Active: %16s <- %16s\n",
			       addr_local, addr_remote);
			found++;
		}

		if (!found) {
			printk("     No active connections to this server.\n");
		}
	}
#else
	printk("     TCP not enabled for this server.\n");
#endif
#endif /* CONFIG_NET_APP_SERVER */

	(*count)++;

	return;
}
#elif defined(CONFIG_NET_DEBUG_APP)
static void net_app_cb(struct net_app_ctx *ctx, void *user_data) {}
#endif

int net_shell_cmd_app(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_DEBUG_APP)
	int i = 0;

	if (IS_ENABLED(CONFIG_NET_APP_SERVER)) {
		net_app_server_foreach(net_app_cb, &i);

		if (i == 0) {
			printk("No net app server instances found.\n");
			i = -1;
		}
	}

	if (IS_ENABLED(CONFIG_NET_APP_CLIENT)) {
		if (i) {
			printk("\n");
			i = 0;
		}

		net_app_client_foreach(net_app_cb, &i);

		if (i == 0) {
			printk("No net app client instances found.\n");
		}
	}
#else
	printk("Enable CONFIG_NET_DEBUG_APP and either CONFIG_NET_APP_CLIENT "
	       "or CONFIG_NET_APP_SERVER to see client/server instance "
	       "information.\n");
#endif

	return 0;
}

#if defined(CONFIG_NET_ARP)
static void arp_cb(struct arp_entry *entry, void *user_data)
{
	int *count = user_data;

	if (*count == 0) {
		printk("     Interface  Link              Address\n");
	}

	printk("[%2d] %p %s %s\n", *count, entry->iface,
	       net_sprint_ll_addr(entry->eth.addr,
				  sizeof(struct net_eth_addr)),
	       net_sprint_ipv4_addr(&entry->ip));

	(*count)++;
}
#endif /* CONFIG_NET_ARP */

int net_shell_cmd_arp(int argc, char *argv[])
{
	ARG_UNUSED(argc);

#if defined(CONFIG_NET_ARP)
	int arg = 1;

	if (!argv[arg]) {
		/* ARP cache content */
		int count = 0;

		if (net_arp_foreach(arp_cb, &count) == 0) {
			printk("ARP cache is empty.\n");
		}

		return 0;
	}

	if (strcmp(argv[arg], "flush") == 0) {
		printk("Flushing ARP cache.\n");
		net_arp_clear_cache(NULL);
		return 0;
	}
#else
	printk("Enable CONFIG_NET_ARP, CONFIG_NET_IPV4 and "
	       "CONFIG_NET_L2_ETHERNET to see ARP information.\n");
#endif

	return 0;
}

int net_shell_cmd_conn(int argc, char *argv[])
{
	int count = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("     Context   \tIface         Flags "
	       "Local           \tRemote\n");

	net_context_foreach(context_cb, &count);

	if (count == 0) {
		printk("No connections\n");
	}

#if defined(CONFIG_NET_DEBUG_CONN)
	printk("\n     Handler    Callback  \tProto\t"
	       "Local           \tRemote\n");

	count = 0;

	net_conn_foreach(conn_handler_cb, &count);

	if (count == 0) {
		printk("No connection handlers found.\n");
	}
#endif

#if defined(CONFIG_NET_TCP)
	printk("\nTCP        Context   Src port Dst port   Send-Seq   Send-Ack  MSS"
	       "    State\n");

	count = 0;

	net_tcp_foreach(tcp_cb, &count);

	if (count == 0) {
		printk("No TCP connections\n");
	} else {
#if defined(CONFIG_NET_DEBUG_TCP)
		/* Print information about pending packets */
		count = 0;
		net_tcp_foreach(tcp_sent_list_cb, &count);
#endif /* CONFIG_NET_DEBUG_TCP */
	}

#if !defined(CONFIG_NET_DEBUG_TCP)
	printk("\nEnable CONFIG_NET_DEBUG_TCP for additional info\n");
#endif /* CONFIG_NET_DEBUG_TCP */

#endif

#if defined(CONFIG_NET_IPV6_FRAGMENT)
	count = 0;

	net_ipv6_frag_foreach(ipv6_frag_cb, &count);

	/* Do not print anything if no fragments are pending atm */
#endif

	return 0;
}

#if defined(CONFIG_DNS_RESOLVER)
static void dns_result_cb(enum dns_resolve_status status,
			  struct dns_addrinfo *info,
			  void *user_data)
{
	bool *first = user_data;

	if (status == DNS_EAI_CANCELED) {
		printk("\nTimeout while resolving name.\n");
		*first = false;
		return;
	}

	if (status == DNS_EAI_INPROGRESS && info) {
		char addr[NET_IPV6_ADDR_LEN];

		if (*first) {
			printk("\n");
			*first = false;
		}

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
		}

		printk("\t%s\n", addr);
		return;
	}

	if (status == DNS_EAI_ALLDONE) {
		printk("All results received\n");
		*first = false;
		return;
	}

	if (status == DNS_EAI_FAIL) {
		printk("No such name found.\n");
		*first = false;
		return;
	}

	printk("Unhandled status %d received\n", status);
}

static void print_dns_info(struct dns_resolve_context *ctx)
{
	int i;

	printk("DNS servers:\n");

	for (i = 0; i < CONFIG_DNS_RESOLVER_MAX_SERVERS +
		     DNS_MAX_MCAST_SERVERS; i++) {
		if (ctx->servers[i].dns_server.sa_family == AF_INET) {
			printk("\t%s:%u\n",
			       net_sprint_ipv4_addr(
				       &net_sin(&ctx->servers[i].dns_server)->
				       sin_addr),
			       ntohs(net_sin(&ctx->servers[i].
					     dns_server)->sin_port));
		} else if (ctx->servers[i].dns_server.sa_family == AF_INET6) {
			printk("\t[%s]:%u\n",
			       net_sprint_ipv6_addr(
				       &net_sin6(&ctx->servers[i].dns_server)->
				       sin6_addr),
			       ntohs(net_sin6(&ctx->servers[i].
					     dns_server)->sin6_port));
		}
	}

	printk("Pending queries:\n");

	for (i = 0; i < CONFIG_DNS_NUM_CONCUR_QUERIES; i++) {
		s32_t remaining;

		if (!ctx->queries[i].cb) {
			continue;
		}

		remaining =
			k_delayed_work_remaining_get(&ctx->queries[i].timer);

		if (ctx->queries[i].query_type == DNS_QUERY_TYPE_A) {
			printk("\tIPv4[%u]: %s remaining %d\n",
			       ctx->queries[i].id,
			       ctx->queries[i].query,
			       remaining);
		} else if (ctx->queries[i].query_type == DNS_QUERY_TYPE_AAAA) {
			printk("\tIPv6[%u]: %s remaining %d\n",
			       ctx->queries[i].id,
			       ctx->queries[i].query,
			       remaining);
		}
	}
}
#endif

int net_shell_cmd_dns(int argc, char *argv[])
{
#if defined(CONFIG_DNS_RESOLVER)
#define DNS_TIMEOUT K_MSEC(2000) /* ms */

	struct dns_resolve_context *ctx;
	enum dns_query_type qtype = DNS_QUERY_TYPE_A;
	char *host, *type = NULL;
	bool first = true;
	int arg = 1;
	int ret, i;

	if (!argv[arg]) {
		/* DNS status */
		ctx = dns_resolve_get_default();
		if (!ctx) {
			printk("No default DNS context found.\n");
			return 0;
		}

		print_dns_info(ctx);
		return 0;
	}

	if (strcmp(argv[arg], "cancel") == 0) {
		ctx = dns_resolve_get_default();
		if (!ctx) {
			printk("No default DNS context found.\n");
			return 0;
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
			printk("Cancelled %d pending requests.\n", ret);
		} else {
			printk("No pending DNS requests.\n");
		}

		return 0;
	}

	host = argv[arg++];

	if (argv[arg]) {
		type = argv[arg];
	}

	if (type) {
		if (strcmp(type, "A") == 0) {
			qtype = DNS_QUERY_TYPE_A;
			printk("IPv4 address type\n");
		} else if (strcmp(type, "AAAA") == 0) {
			qtype = DNS_QUERY_TYPE_AAAA;
			printk("IPv6 address type\n");
		} else {
			printk("Unknown query type, specify either "
			       "A or AAAA\n");
			return 0;
		}
	}

	ret = dns_get_addr_info(host, qtype, NULL, dns_result_cb, &first,
				DNS_TIMEOUT);
	if (ret < 0) {
		printk("Cannot resolve '%s' (%d)\n", host, ret);
	} else {
		printk("Query for '%s' sent.\n", host);
	}

#else
	printk("DNS resolver not supported.\n");
#endif
	return 0;
}

#if defined(CONFIG_NET_GPTP)
static void gptp_port_cb(int port, struct net_if *iface, void *user_data)
{
	int *count = user_data;

	if (*count == 0) {
		printk("Port Interface\n");
	}

	(*count)++;

	printk("%2d   %p\n", port, iface);
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

static void gptp_print_port_info(int port)
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
		printk("Cannot get gPTP information for port %d (%d)\n",
		       port, ret);
		return;
	}

	printk("Port id    : %d\n", port_ds->port_id.port_number);

	printk("Clock id   : ");
	for (i = 0; i < sizeof(port_ds->port_id.clk_id); i++) {
		printk("%02x", port_ds->port_id.clk_id[i]);

		if (i != (sizeof(port_ds->port_id.clk_id) - 1)) {
			printk(":");
		}
	}
	printk("\n");

	printk("Version    : %d\n", port_ds->version);
	printk("AS capable : %s\n", port_ds->as_capable ? "yes" : "no");

	printk("\nConfiguration:\n");
	printk("Time synchronization and Best Master Selection enabled        "
	       ": %s\n", port_ds->ptt_port_enabled ? "yes" : "no");
	printk("The port is measuring the path delay                          "
	       ": %s\n", port_ds->is_measuring_delay ? "yes" : "no");
	printf("One way propagation time on %s    : %u ns\n",
	       "the link attached to this port",
	       (u32_t)port_ds->neighbor_prop_delay);
	printf("Propagation time threshold for %s : %u ns\n",
	       "the link attached to this port",
	       (u32_t)port_ds->neighbor_prop_delay_thresh);
	printk("Estimate of the ratio of the frequency with the peer          "
	       ": %u\n", (u32_t)port_ds->neighbor_rate_ratio);
	printk("Asymmetry on the link relative to the grand master time base  "
	       ": %lld\n", port_ds->delay_asymmetry);
	printk("Maximum interval between sync %s                        "
	       ": %llu\n", "messages", port_ds->sync_receipt_timeout_time_itv);
	printk("Maximum number of Path Delay Requests without a response      "
	       ": %d\n", port_ds->allowed_lost_responses);
	printk("Current Sync %s                        : %d\n",
	       "sequence id for this port", port_ds->sync_seq_id);
	printk("Current Path Delay Request %s          : %d\n",
	       "sequence id for this port", port_ds->pdelay_req_seq_id);
	printk("Current Announce %s                    : %d\n",
	       "sequence id for this port", port_ds->announce_seq_id);
	printk("Current Signaling %s                   : %d\n",
	       "sequence id for this port", port_ds->signaling_seq_id);
	printk("Whether neighborRateRatio %s  : %s\n",
	       "needs to be computed for this port",
	       port_ds->compute_neighbor_rate_ratio ? "yes" : "no");
	printk("Whether neighborPropDelay %s  : %s\n",
	       "needs to be computed for this port",
	       port_ds->compute_neighbor_prop_delay ? "yes" : "no");
	printk("Initial Announce Interval %s            : %d\n",
	       "as a Logarithm to base 2", port_ds->ini_log_announce_itv);
	printk("Current Announce Interval %s            : %d\n",
	       "as a Logarithm to base 2", port_ds->cur_log_announce_itv);
	printk("Initial Sync Interval %s                : %d\n",
	       "as a Logarithm to base 2", port_ds->ini_log_half_sync_itv);
	printk("Current Sync Interval %s                : %d\n",
	       "as a Logarithm to base 2", port_ds->cur_log_half_sync_itv);
	printk("Initial Path Delay Request Interval %s  : %d\n",
	       "as a Logarithm to base 2", port_ds->ini_log_pdelay_req_itv);
	printk("Current Path Delay Request Interval %s  : %d\n",
	       "as a Logarithm to base 2", port_ds->cur_log_pdelay_req_itv);
	printk("Time without receiving announce %s %s  : %d ms (%d)\n",
	       "messages", "before running BMCA",
	       gptp_uscaled_ns_to_timer_ms(
		       &port_bmca_data->ann_rcpt_timeout_time_interval),
	       port_ds->announce_receipt_timeout);
	printk("Time without receiving sync %s %s      : %llu ms (%d)\n",
	       "messages", "before running BMCA",
	       (port_ds->sync_receipt_timeout_time_itv >> 16) /
	       (NSEC_PER_SEC / MSEC_PER_SEC),
	       port_ds->sync_receipt_timeout);
	printk("Sync event %s                 : %llu ms\n",
	       "transmission interval for the port",
	       USCALED_NS_TO_NS(port_ds->half_sync_itv.low) /
	       (NSEC_PER_USEC * USEC_PER_MSEC));
	printk("Path Delay Request %s         : %llu ms\n",
	       "transmission interval for the port",
	       USCALED_NS_TO_NS(port_ds->pdelay_req_itv.low) /
	       (NSEC_PER_USEC * USEC_PER_MSEC));

	printk("\nRuntime status:\n");
	printk("Current global port state                          "
	       "      : %s\n", selected_role_str(port));
	printk("Path Delay Request state machine variables:\n");
	printk("\tCurrent state                                    "
	       ": %s\n", pdelay_req2str(port_state->pdelay_req.state));
	printk("\tInitial Path Delay Response Peer Timestamp       "
	       ": %llu\n", port_state->pdelay_req.ini_resp_evt_tstamp);
	printk("\tInitial Path Delay Response Ingress Timestamp    "
	       ": %llu\n", port_state->pdelay_req.ini_resp_ingress_tstamp);
	printk("\tPath Delay Response %s %s            : %u\n",
	       "messages", "received",
	       port_state->pdelay_req.rcvd_pdelay_resp);
	printk("\tPath Delay Follow Up %s %s           : %u\n",
	       "messages", "received",
	       port_state->pdelay_req.rcvd_pdelay_follow_up);
	printk("\tNumber of lost Path Delay Responses              "
	       ": %u\n", port_state->pdelay_req.lost_responses);
	printk("\tTimer expired send a new Path Delay Request      "
	       ": %u\n", port_state->pdelay_req.pdelay_timer_expired);
	printk("\tNeighborRateRatio has been computed successfully "
	       ": %u\n", port_state->pdelay_req.neighbor_rate_ratio_valid);
	printk("\tPath Delay has already been computed after init  "
	       ": %u\n", port_state->pdelay_req.init_pdelay_compute);
	printk("\tCount consecutive reqs with multiple responses   "
	       ": %u\n", port_state->pdelay_req.multiple_resp_count);

	printk("Path Delay Response state machine variables:\n");
	printk("\tCurrent state                                    "
	       ": %s\n", pdelay_resp2str(port_state->pdelay_resp.state));

	printk("SyncReceive state machine variables:\n");
	printk("\tCurrent state                                    "
	       ": %s\n", sync_rcv2str(port_state->sync_rcv.state));
	printk("\tA Sync %s %s                 : %s\n",
	       "Message", "has been received",
	       port_state->sync_rcv.rcvd_sync ? "yes" : "no");
	printk("\tA Follow Up %s %s            : %s\n",
	       "Message", "has been received",
	       port_state->sync_rcv.rcvd_follow_up ? "yes" : "no");
	printk("\tA Follow Up %s %s                      : %s\n",
	       "Message", "timeout",
	       port_state->sync_rcv.follow_up_timeout_expired ? "yes" : "no");
	printk("\tTime at which a Sync %s without Follow Up\n"
	       "\t                             will be discarded   "
	       ": %llu\n", "Message",
	       port_state->sync_rcv.follow_up_receipt_timeout);

	printk("SyncSend state machine variables:\n");
	printk("\tCurrent state                                    "
	       ": %s\n", sync_send2str(port_state->sync_send.state));
	printk("\tA MDSyncSend structure %s         : %s\n",
	       "has been received",
	       port_state->sync_send.rcvd_md_sync ? "yes" : "no");
	printk("\tThe timestamp for the sync msg %s : %s\n",
	       "has been received",
	       port_state->sync_send.md_sync_timestamp_avail ? "yes" : "no");

	printk("PortSyncSyncReceive state machine variables:\n");
	printk("\tCurrent state                                    "
	       ": %s\n", pss_rcv2str(port_state->pss_rcv.state));
	printf("\tGrand Master / Local Clock frequency ratio       "
	       ": %f\n", port_state->pss_rcv.rate_ratio);
	printk("\tA MDSyncReceive struct is ready to be processed  "
	       ": %s\n", port_state->pss_rcv.rcvd_md_sync ? "yes" : "no");
	printk("\tExpiry of SyncReceiptTimeoutTimer                : %s\n",
	       port_state->pss_rcv.rcv_sync_receipt_timeout_timer_expired ?
	       "yes" : "no");

	printk("PortSyncSyncSend state machine variables:\n");
	printk("\tCurrent state                                    "
	       ": %s\n", pss_send2str(port_state->pss_send.state));
	printk("\tFollow Up Correction Field of last recv PSS      "
	       ": %lld\n",
	       port_state->pss_send.last_follow_up_correction_field);
	printk("\tUpstream Tx Time of the last recv PortSyncSync   "
	       ": %llu\n", port_state->pss_send.last_upstream_tx_time);
	printk("\tSync Receipt Timeout Time of last recv PSS       "
	       ": %llu\n",
	       port_state->pss_send.last_sync_receipt_timeout_time);
	printf("\tRate Ratio of the last received PortSyncSync     "
	       ": %f\n",
	       port_state->pss_send.last_rate_ratio);
	printf("\tGM Freq Change of the last received PortSyncSync "
	       ": %f\n", port_state->pss_send.last_gm_freq_change);
	printk("\tGM Time Base Indicator of last recv PortSyncSync "
	       ": %d\n", port_state->pss_send.last_gm_time_base_indicator);
	printk("\tReceived Port Number of last recv PortSyncSync   "
	       ": %d\n",
	       port_state->pss_send.last_rcvd_port_num);
	printk("\tPortSyncSync structure is ready to be processed  "
	       ": %s\n", port_state->pss_send.rcvd_pss_sync ? "yes" : "no");
	printk("\tFlag when the %s has expired    : %s\n",
	       "half_sync_itv_timer",
	       port_state->pss_send.half_sync_itv_timer_expired ?
	       "yes" : "no");
	printk("\tHas %s expired twice            : %s\n",
	       "half_sync_itv_timer",
	       port_state->pss_send.sync_itv_timer_expired ? "yes" : "no");
	printk("\tHas syncReceiptTimeoutTime expired               "
	       ": %s\n",
	       port_state->pss_send.send_sync_receipt_timeout_timer_expired ?
	       "yes" : "no");

	printk("PortAnnounceReceive state machine variables:\n");
	printk("\tCurrent state                                    "
	       ": %s\n", pa_rcv2str(port_state->pa_rcv.state));
	printk("\tAn announce message is ready to be processed     "
	       ": %s\n",
	       port_state->pa_rcv.rcvd_announce ? "yes" : "no");

	printk("PortAnnounceInformation state machine variables:\n");
	printk("\tCurrent state                                    "
	       ": %s\n", pa_info2str(port_state->pa_info.state));
	printk("\tExpired announce information                     "
	       ": %s\n", port_state->pa_info.ann_expired ? "yes" : "no");

	printk("PortAnnounceTransmit state machine variables:\n");
	printk("\tCurrent state                                    "
	       ": %s\n", pa_transmit2str(port_state->pa_transmit.state));
	printk("\tTrigger announce information                     "
	       ": %s\n", port_state->pa_transmit.ann_trigger ? "yes" : "no");

#if defined(CONFIG_NET_GPTP_STATISTICS)
	printk("\nStatistics:\n");
	printk("Sync %s %s                 : %u\n",
	       "messages", "received", port_param_ds->rx_sync_count);
	printk("Follow Up %s %s            : %u\n",
	       "messages", "received", port_param_ds->rx_fup_count);
	printk("Path Delay Request %s %s   : %u\n",
	       "messages", "received", port_param_ds->rx_pdelay_req_count);
	printk("Path Delay Response %s %s  : %u\n",
	       "messages", "received", port_param_ds->rx_pdelay_resp_count);
	printk("Path Delay %s threshold %s : %u\n",
	       "messages", "exceeded",
	       port_param_ds->neighbor_prop_delay_exceeded);
	printk("Path Delay Follow Up %s %s : %u\n",
	       "messages", "received", port_param_ds->rx_pdelay_resp_fup_count);
	printk("Announce %s %s             : %u\n",
	       "messages", "received", port_param_ds->rx_announce_count);
	printk("ptp %s discarded                 : %u\n",
	       "messages", port_param_ds->rx_ptp_packet_discard_count);
	printk("Sync %s %s                 : %u\n",
	       "reception", "timeout",
	       port_param_ds->sync_receipt_timeout_count);
	printk("Announce %s %s             : %u\n",
	       "reception", "timeout",
	       port_param_ds->announce_receipt_timeout_count);
	printk("Path Delay Requests without a response "
	       ": %u\n", port_param_ds->pdelay_allowed_lost_resp_exceed_count);
	printk("Sync %s %s                     : %u\n",
	       "messages", "sent", port_param_ds->tx_sync_count);
	printk("Follow Up %s %s                : %u\n",
	       "messages", "sent", port_param_ds->tx_fup_count);
	printk("Path Delay Request %s %s       : %u\n",
	       "messages", "sent", port_param_ds->tx_pdelay_req_count);
	printk("Path Delay Response %s %s      : %u\n",
	       "messages", "sent", port_param_ds->tx_pdelay_resp_count);
	printk("Path Delay Response FUP %s %s  : %u\n",
	       "messages", "sent", port_param_ds->tx_pdelay_resp_fup_count);
	printk("Announce %s %s                 : %u\n",
	       "messages", "sent", port_param_ds->tx_announce_count);
#endif /* CONFIG_NET_GPTP_STATISTICS */
}
#endif /* CONFIG_NET_GPTP */

int net_shell_cmd_gptp(int argc, char *argv[])
{
#if defined(CONFIG_NET_GPTP)
	/* gPTP status */
	struct gptp_domain *domain = gptp_get_domain();
	int count = 0;
	int arg = 1;

	if (strcmp(argv[0], "gptp")) {
		arg++;
	}

	if (argv[arg]) {
		char *endptr;
		int port = strtol(argv[arg], &endptr, 10);

		if (*endptr == '\0') {
			gptp_print_port_info(port);
		} else {
			printk("Not a valid gPTP port number: %s\n", argv[arg]);
		}
	} else {
		gptp_foreach_port(gptp_port_cb, &count);

		printk("\n");

		printk("SiteSyncSync state machine variables:\n");
		printk("\tCurrent state                  "
		       ": %s\n", site_sync2str(domain->state.site_ss.state));
		printk("\tA PortSyncSync struct is ready "
		       ": %s\n", domain->state.site_ss.rcvd_pss ? "yes" : "no");

		printk("ClockSlaveSync state machine variables:\n");
		printk("\tCurrent state                  "
		       ": %s\n",
		       clk_slave2str(domain->state.clk_slave_sync.state));
		printk("\tA PortSyncSync struct is ready "
		       ": %s\n",
		       domain->state.clk_slave_sync.rcvd_pss ? "yes" : "no");
		printk("\tThe local clock has expired    "
		       ": %s\n",
		       domain->state.clk_slave_sync.rcvd_local_clk_tick ?
		       "yes" : "no");

		printk("PortRoleSelection state machine variables:\n");
		printk("\tCurrent state                  "
		       ": %s\n",
		       pr_selection2str(domain->state.pr_sel.state));

		printk("ClockMasterSyncReceive state machine variables:\n");
		printk("\tCurrent state                  "
		       ": %s\n", cms_rcv2str(
			       domain->state.clk_master_sync_receive.state));
		printk("\tA ClockSourceTime              "
		       ": %s\n",
		  domain->state.clk_master_sync_receive.rcvd_clock_source_req ?
		       "yes" : "no");
		printk("\tThe local clock has expired    "
		       ": %s\n",
		  domain->state.clk_master_sync_receive.rcvd_local_clock_tick ?
		       "yes" : "no");
	}
#else
	printk("gPTP not supported, set CONFIG_NET_GPTP to enable it.\n");
#endif
	return 0;
}

#if defined(CONFIG_NET_DEBUG_HTTP_CONN) && defined(CONFIG_HTTP_SERVER)
#define MAX_HTTP_OUTPUT_LEN 64
static char *http_str_output(char *output, int outlen, const char *str, int len)
{
	if (len > outlen) {
		len = outlen;
	}

	if (len == 0) {
		(void)memset(output, 0, outlen);
	} else {
		memcpy(output, str, len);
		output[len] = '\0';
	}

	return output;
}

static void http_server_cb(struct http_ctx *entry, void *user_data)
{
	int *count = user_data;
	static char output[MAX_HTTP_OUTPUT_LEN];
	int i;

	/* +7 for []:port */
	char addr_local[ADDR_LEN + 7];
	char addr_remote[ADDR_LEN + 7] = "";

	if (*count == 0) {
		printk("        HTTP ctx    Local           \t"
		       "Remote          \tURL\n");
	}

	(*count)++;

	for (i = 0; i < CONFIG_NET_APP_SERVER_NUM_CONN; i++) {
		if (!entry->app_ctx.server.net_ctxs[i] ||
		    !net_context_is_used(entry->app_ctx.server.net_ctxs[i])) {
			continue;
		}

		get_addresses(entry->app_ctx.server.net_ctxs[i],
			      addr_local, sizeof(addr_local),
			      addr_remote, sizeof(addr_remote));

		printk("[%2d] %c%c %p  %16s\t%16s\t%s\n",
		       *count,
		       entry->app_ctx.is_enabled ? 'E' : 'D',
		       entry->is_tls ? 'S' : ' ',
		       entry, addr_local, addr_remote,
		       http_str_output(output, sizeof(output) - 1,
				       entry->http.url, entry->http.url_len));
	}
}
#endif /* CONFIG_NET_DEBUG_HTTP_CONN && CONFIG_HTTP_SERVER */

int net_shell_cmd_http(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_DEBUG_HTTP_CONN) && defined(CONFIG_HTTP_SERVER)
	static int count;
	int arg = 1;

	count = 0;

	/* Turn off monitoring if it was enabled */
	http_server_conn_monitor(NULL, NULL);

	if (strcmp(argv[0], "http")) {
		arg++;
	}

	if (argv[arg]) {
		if (strcmp(argv[arg], "monitor") == 0) {
			printk("Activating HTTP monitor. Type \"net http\" "
			       "to disable HTTP connection monitoring.\n");
			http_server_conn_monitor(http_server_cb, &count);
		}
	} else {
		http_server_conn_foreach(http_server_cb, &count);
	}
#else
	printk("Enable CONFIG_NET_DEBUG_HTTP_CONN and CONFIG_HTTP_SERVER "
	       "to get HTTP server connection information\n");
#endif

	return 0;
}

int net_shell_cmd_iface(int argc, char *argv[])
{
	int arg = 0;
	bool up = false;
	char *endptr;
	struct net_if *iface;
	int idx, ret;

	if (strcmp(argv[arg], "iface") == 0) {
		arg++;
	}

	if (!argv[arg]) {
#if defined(CONFIG_NET_HOSTNAME_ENABLE)
		printk("Hostname: %s\n\n", net_hostname_get());
#endif
		net_if_foreach(iface_cb, NULL);

		return 0;
	}

	if (strcmp(argv[arg], "up") == 0) {
		arg++;
		up = true;
	} else if (strcmp(argv[arg], "down") == 0) {
		arg++;
	}

	if (!argv[arg]) {
		printk("Usage: net iface [up|down] [index]\n");
		return 0;
	}

	idx = strtol(argv[arg], &endptr, 10);
	if (*endptr != '\0') {
		printk("Invalid index %s\n", argv[arg]);
		return 0;
	}

	if (idx < 0 || idx > 255) {
		printk("Invalid index %d\n", idx);
		return 0;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		printk("No such interface in index %d\n", idx);
		return 0;
	}

	if (up) {
		if (net_if_is_up(iface)) {
			printk("Interface %d is already up.\n", idx);
			return 0;
		}

		ret = net_if_up(iface);
		if (ret) {
			printk("Cannot take interface %d up (%d)\n",
			       idx, ret);
		} else {
			printk("Interface %d is up\n", idx);
		}
	} else {
		ret = net_if_down(iface);
		if (ret) {
			printk("Cannot take interface %d down (%d)\n",
			       idx, ret);
		} else {
			printk("Interface %d is down\n", idx);
		}
	}

	return 0;
}

#if defined(CONFIG_NET_IPV6)
static u32_t time_diff(u32_t time1, u32_t time2)
{
	return (u32_t)abs((s32_t)time1 - (s32_t)time2);
}

static void address_lifetime_cb(struct net_if *iface, void *user_data)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	const char *extra;
	int i;

	ARG_UNUSED(user_data);

	printk("\nIPv6 addresses for interface %p (%s)\n", iface,
	       iface2str(iface, &extra));
	printk("==========================================%s\n", extra);

	if (!ipv6) {
		printk("No IPv6 config found for this interface.\n");
		return;
	}

	printk("Type      \tState    \tLifetime (sec)\tAddress\n");

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
			prefix_len = 128;
		}

		if (ipv6->unicast[i].is_infinite) {
			snprintk(remaining_str, sizeof(remaining_str) - 1,
				 "infinite");
		} else {
			snprintk(remaining_str, sizeof(remaining_str) - 1,
				 "%u", (u32_t)(remaining / 1000));
		}

		printk("%s  \t%s\t%s    \t%s/%d\n",
		       addrtype2str(ipv6->unicast[i].addr_type),
		       addrstate2str(ipv6->unicast[i].addr_state),
		       remaining_str,
		       net_sprint_ipv6_addr(
			       &ipv6->unicast[i].address.in6_addr),
		       prefix_len);
	}
}
#endif /* CONFIG_NET_IPV6 */

int net_shell_cmd_ipv6(int argc, char *argv[])
{
	int arg = 0;

	if (strcmp(argv[arg], "ipv6") == 0) {
		arg++;
	}

	if (!argv[arg]) {
		printk("IPv6 support                              : %s\n",
		       IS_ENABLED(CONFIG_NET_IPV6) ?
		       "enabled" : "disabled");
		if (!IS_ENABLED(CONFIG_NET_IPV6)) {
			return 0;
		}

#if defined(CONFIG_NET_IPV6)
		printk("IPv6 fragmentation support                : %s\n",
		       IS_ENABLED(CONFIG_NET_IPV6_FRAGMENT) ? "enabled" :
		       "disabled");
		printk("Multicast Listener Discovery support      : %s\n",
		       IS_ENABLED(CONFIG_NET_IPV6_MLD) ? "enabled" :
		       "disabled");
		printk("Neighbor cache support                    : %s\n",
		       IS_ENABLED(CONFIG_NET_IPV6_NBR_CACHE) ? "enabled" :
		       "disabled");
		printk("Neighbor discovery support                : %s\n",
		       IS_ENABLED(CONFIG_NET_IPV6_ND) ? "enabled" :
		       "disabled");
		printk("Duplicate address detection (DAD) support : %s\n",
		       IS_ENABLED(CONFIG_NET_IPV6_DAD) ? "enabled" :
		       "disabled");
		printk("Router advertisement RDNSS option support : %s\n",
		       IS_ENABLED(CONFIG_NET_IPV6_RA_RDNSS) ? "enabled" :
		       "disabled");
		printk("6lo header compression support            : %s\n",
		       IS_ENABLED(CONFIG_NET_6LO) ? "enabled" :
		       "disabled");
		if (IS_ENABLED(CONFIG_NET_6LO_CONTEXT)) {
			printk("6lo context based compression "
			       "support     : %s\n",
			       IS_ENABLED(CONFIG_NET_6LO_CONTEXT) ? "enabled" :
			       "disabled");
		}
		printk("Max number of IPv6 network interfaces "
		       "in the system          : %d\n",
		       CONFIG_NET_IF_MAX_IPV6_COUNT);
		printk("Max number of unicast IPv6 addresses "
		       "per network interface   : %d\n",
		       CONFIG_NET_IF_UNICAST_IPV6_ADDR_COUNT);
		printk("Max number of multicast IPv6 addresses "
		       "per network interface : %d\n",
		       CONFIG_NET_IF_MCAST_IPV6_ADDR_COUNT);
		printk("Max number of IPv6 prefixes per network "
		       "interface            : %d\n",
		       CONFIG_NET_IF_IPV6_PREFIX_COUNT);

		/* Print information about address lifetime */
		net_if_foreach(address_lifetime_cb, NULL);
#endif

		return 0;
	}

	return 0;
}

struct ctx_info {
	int pos;
	bool are_external_pools;
	struct k_mem_slab *tx_slabs[CONFIG_NET_MAX_CONTEXTS];
	struct net_buf_pool *data_pools[CONFIG_NET_MAX_CONTEXTS];
};

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
	struct ctx_info *info = user_data;
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

#if defined(CONFIG_NET_DEBUG_NET_PKT)
		printk("%p\t%u\t%u\tETX\n",
		       slab, slab->num_blocks, k_mem_slab_num_free_get(slab));
#else
		printk("%p\t%d\tETX\n", slab, slab->num_blocks);
#endif
		info->are_external_pools = true;
		info->tx_slabs[info->pos] = slab;
	}

	if (context->data_pool) {
		pool = context->data_pool();

		if (slab_pool_found_already(info, NULL, pool)) {
			return;
		}

#if defined(CONFIG_NET_DEBUG_NET_PKT)
		printk("%p\t%d\t%d\tEDATA (%s)\n",
		       pool, pool->buf_count,
		       pool->avail_count, pool->name);
#else
		printk("%p\t%d\tEDATA\n", pool, pool->buf_count);
#endif
		info->are_external_pools = true;
		info->data_pools[info->pos] = pool;
	}

	info->pos++;
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */
}

int net_shell_cmd_mem(int argc, char *argv[])
{
	struct k_mem_slab *rx, *tx;
	struct net_buf_pool *rx_data, *tx_data;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	net_pkt_get_info(&rx, &tx, &rx_data, &tx_data);

	printk("Fragment length %d bytes\n", CONFIG_NET_BUF_DATA_SIZE);

	printk("Network buffer pools:\n");

#if defined(CONFIG_NET_BUF_POOL_USAGE)
	printk("Address\t\tTotal\tAvail\tName\n");

	printk("%p\t%d\t%u\tRX\n",
	       rx, rx->num_blocks, k_mem_slab_num_free_get(rx));

	printk("%p\t%d\t%u\tTX\n",
	       tx, tx->num_blocks, k_mem_slab_num_free_get(tx));

	printk("%p\t%d\t%d\tRX DATA (%s)\n",
	       rx_data, rx_data->buf_count,
	       rx_data->avail_count, rx_data->name);

	printk("%p\t%d\t%d\tTX DATA (%s)\n",
	       tx_data, tx_data->buf_count,
	       tx_data->avail_count, tx_data->name);
#else
	printk("(CONFIG_NET_BUF_POOL_USAGE to see free #s)\n");
	printk("Address\t\tTotal\tName\n");

	printk("%p\t%d\tRX\n", rx, rx->num_blocks);
	printk("%p\t%d\tTX\n", tx, tx->num_blocks);
	printk("%p\t%d\tRX DATA\n", rx_data, rx_data->buf_count);
	printk("%p\t%d\tTX DATA\n", tx_data, tx_data->buf_count);
#endif /* CONFIG_NET_BUF_POOL_USAGE */

	if (IS_ENABLED(CONFIG_NET_CONTEXT_NET_PKT_POOL)) {
		struct ctx_info info;

		(void)memset(&info, 0, sizeof(info));
		net_context_foreach(context_info, &info);

		if (!info.are_external_pools) {
			printk("No external memory pools found.\n");
		}
	}

	return 0;
}

#if defined(CONFIG_NET_IPV6)
static void nbr_cb(struct net_nbr *nbr, void *user_data)
{
	int *count = user_data;
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
		printk("     Neighbor   Interface        Flags State     "
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

	printk("[%2d] %p %p %5d/%d/%d/%d %s%s %6lld  %17s%s %s\n",
	       *count, nbr, nbr->iface,
	       net_ipv6_nbr_data(nbr)->link_metric,
	       nbr->ref,
	       net_ipv6_nbr_data(nbr)->ns_count,
	       net_ipv6_nbr_data(nbr)->is_router,
	       state_str,
	       state_pad,
#if defined(CONFIG_NET_IPV6_ND)
	       remaining > 0 ? remaining : 0,
#else
	       0LL,
#endif
	       nbr->idx == NET_NBR_LLADDR_UNKNOWN ? "?" :
	       net_sprint_ll_addr(
		       net_nbr_get_lladdr(nbr->idx)->addr,
		       net_nbr_get_lladdr(nbr->idx)->len),
	       net_nbr_get_lladdr(nbr->idx)->len == 8 ? "" : padding,
	       net_sprint_ipv6_addr(&net_ipv6_nbr_data(nbr)->addr));
}
#endif

int net_shell_cmd_nbr(int argc, char *argv[])
{
#if defined(CONFIG_NET_IPV6)
	int count = 0;
	int arg = 1;

	if (argv[arg]) {
		struct in6_addr addr;
		int ret;

		if (strcmp(argv[arg], "rm")) {
			printk("Unknown command '%s'\n", argv[arg]);
			return 0;
		}

		if (!argv[++arg]) {
			printk("Neighbor IPv6 address missing.\n");
			return 0;
		}

		ret = net_addr_pton(AF_INET6, argv[arg], &addr);
		if (ret < 0) {
			printk("Cannot parse '%s'\n", argv[arg]);
			return 0;
		}

		if (!net_ipv6_nbr_rm(NULL, &addr)) {
			printk("Cannot remove neighbor %s\n",
			       net_sprint_ipv6_addr(&addr));
		} else {
			printk("Neighbor %s removed.\n",
			       net_sprint_ipv6_addr(&addr));
		}
	}

	net_ipv6_nbr_foreach(nbr_cb, &count);

	if (count == 0) {
		printk("No neighbors.\n");
	}
#else
	printk("IPv6 not enabled.\n");
#endif /* CONFIG_NET_IPV6 */

	return 0;
}

#if defined(CONFIG_NET_IPV6) || defined(CONFIG_NET_IPV4)

K_SEM_DEFINE(ping_timeout, 0, 1);

#if defined(CONFIG_NET_IPV6)

static enum net_verdict _handle_ipv6_echo_reply(struct net_pkt *pkt);

static struct net_icmpv6_handler ping6_handler = {
	.type = NET_ICMPV6_ECHO_REPLY,
	.code = 0,
	.handler = _handle_ipv6_echo_reply,
};

static inline void _remove_ipv6_ping_handler(void)
{
	net_icmpv6_unregister_handler(&ping6_handler);
}

static enum net_verdict _handle_ipv6_echo_reply(struct net_pkt *pkt)
{
	printk("Received echo reply from %s to %s\n",
		net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->src),
		net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->dst));

	k_sem_give(&ping_timeout);
	_remove_ipv6_ping_handler();

	net_pkt_unref(pkt);
	return NET_OK;
}

static int _ping_ipv6(char *host)
{
	struct in6_addr ipv6_target;
	struct net_if *iface = net_if_get_default();
	struct net_nbr *nbr;
	int ret;

#if defined(CONFIG_NET_ROUTE)
	struct net_route_entry *route;
#endif

	if (net_addr_pton(AF_INET6, host, &ipv6_target) < 0) {
		return -EINVAL;
	}

	net_icmpv6_register_handler(&ping6_handler);

	nbr = net_ipv6_nbr_lookup(NULL, &ipv6_target);
	if (nbr) {
		iface = nbr->iface;
	}

#if defined(CONFIG_NET_ROUTE)
	route = net_route_lookup(NULL, &ipv6_target);
	if (route) {
		iface = route->iface;
	}
#endif

	ret = net_icmpv6_send_echo_request(iface,
					   &ipv6_target,
					   sys_rand32_get(),
					   sys_rand32_get());
	if (ret) {
		_remove_ipv6_ping_handler();
	} else {
		printk("Sent a ping to %s\n", host);
	}

	return ret;
}
#else
#define _ping_ipv6(...) -EINVAL
#define _remove_ipv6_ping_handler()
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)

static enum net_verdict _handle_ipv4_echo_reply(struct net_pkt *pkt);

static struct net_icmpv4_handler ping4_handler = {
	.type = NET_ICMPV4_ECHO_REPLY,
	.code = 0,
	.handler = _handle_ipv4_echo_reply,
};

static inline void _remove_ipv4_ping_handler(void)
{
	net_icmpv4_unregister_handler(&ping4_handler);
}

static enum net_verdict _handle_ipv4_echo_reply(struct net_pkt *pkt)
{
	printk("Received echo reply from %s to %s\n",
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src),
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst));

	k_sem_give(&ping_timeout);
	_remove_ipv4_ping_handler();

	net_pkt_unref(pkt);
	return NET_OK;
}

static int _ping_ipv4(char *host)
{
	struct in_addr ipv4_target;
	int ret;

	if (net_addr_pton(AF_INET, host, &ipv4_target) < 0) {
		return -EINVAL;
	}

	net_icmpv4_register_handler(&ping4_handler);

	ret = net_icmpv4_send_echo_request(
		net_if_ipv4_select_src_iface(&ipv4_target),
		&ipv4_target,
		sys_rand32_get(),
		sys_rand32_get());
	if (ret) {
		_remove_ipv4_ping_handler();
	} else {
		printk("Sent a ping to %s\n", host);
	}

	return ret;
}
#else
#define _ping_ipv4(...) -EINVAL
#define _remove_ipv4_ping_handler()
#endif /* CONFIG_NET_IPV4 */
#endif /* CONFIG_NET_IPV6 || CONFIG_NET_IPV4 */

int net_shell_cmd_ping(int argc, char *argv[])
{
	char *host;
	int ret;

	ARG_UNUSED(argc);

	if (!strcmp(argv[0], "ping")) {
		host = argv[1];
	} else {
		host = argv[2];
	}

	if (!host) {
		printk("Target host missing\n");
		return 0;
	}

	ret = _ping_ipv6(host);
	if (!ret) {
		goto wait_reply;
	} else if (ret == -EIO) {
		printk("Cannot send IPv6 ping\n");
		return 0;
	}

	ret = _ping_ipv4(host);
	if (ret) {
		if (ret == -EIO) {
			printk("Cannot send IPv4 ping\n");
		} else if (ret == -EINVAL) {
			printk("Invalid IP address\n");
		}

		return 0;
	}

wait_reply:
	ret = k_sem_take(&ping_timeout, K_SECONDS(2));
	if (ret == -EAGAIN) {
		printk("Ping timeout\n");
		_remove_ipv6_ping_handler();
		_remove_ipv4_ping_handler();
	}

	return 0;
}

int net_shell_cmd_route(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_ROUTE)
	net_if_foreach(iface_per_route_cb, NULL);
#else
	printk("Network route support not compiled in.\n");
#endif

#if defined(CONFIG_NET_ROUTE_MCAST)
	net_if_foreach(iface_per_mcast_route_cb, NULL);
#endif

	return 0;
}

#if defined(CONFIG_NET_RPL)
static int power(int base, unsigned int exp)
{
	int i, result = 1;

	for (i = 0; i < exp; i++) {
		result *= base;
	}

	return result;
}

static void rpl_parent(struct net_rpl_parent *parent, void *user_data)
{
	int *count = user_data;

	if (*count == 0) {
		printk("      Parent     Last TX   Rank  DTSN  Flags DAG\t\t\t"
		       "Address\n");
	}

	(*count)++;

	if (parent->dag) {
		struct net_ipv6_nbr_data *data;
		char addr[NET_IPV6_ADDR_LEN];

		data = net_rpl_get_ipv6_nbr_data(parent);
		if (data) {
			snprintk(addr, sizeof(addr), "%s",
				 net_sprint_ipv6_addr(&data->addr));
		} else {
			snprintk(addr, sizeof(addr), "<unknown>");
		}

		printk("[%2d]%s %p %7d  %5d   %3d  0x%02x  %s\t%s\n",
		       *count,
		       parent->dag->preferred_parent == parent ? "*" : " ",
		       parent, parent->last_tx_time, parent->rank,
		       parent->dtsn, parent->flags,
		       net_sprint_ipv6_addr(&parent->dag->dag_id),
		       addr);
	}
}

#endif /* CONFIG_NET_RPL */

int net_shell_cmd_rpl(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_RPL)
	struct net_rpl_instance *instance;
	enum net_rpl_mode mode;
	int i, count;

	mode = net_rpl_get_mode();
	printk("RPL Configuration\n");
	printk("=================\n");
	printk("RPL mode                     : %s\n",
	       mode == NET_RPL_MODE_MESH ? "mesh" :
	       (mode == NET_RPL_MODE_FEATHER ? "feather" :
		(mode == NET_RPL_MODE_LEAF ? "leaf" : "<unknown>")));
	printk("Used objective function      : %s\n",
	       IS_ENABLED(CONFIG_NET_RPL_MRHOF) ? "MRHOF" :
	       (IS_ENABLED(CONFIG_NET_RPL_OF0) ? "OF0" : "<unknown>"));
	printk("Used routing metric          : %s\n",
	       IS_ENABLED(CONFIG_NET_RPL_MC_NONE) ? "none" :
	       (IS_ENABLED(CONFIG_NET_RPL_MC_ETX) ? "estimated num of TX" :
		(IS_ENABLED(CONFIG_NET_RPL_MC_ENERGY) ? "energy based" :
		 "<unknown>")));
	printk("Mode of operation (MOP)      : %s\n",
	       IS_ENABLED(CONFIG_NET_RPL_MOP2) ? "Storing, no mcast (MOP2)" :
	       (IS_ENABLED(CONFIG_NET_RPL_MOP3) ? "Storing (MOP3)" :
		"<unknown>"));
	printk("Send probes to nodes         : %s\n",
	       IS_ENABLED(CONFIG_NET_RPL_PROBING) ? "enabled" : "disabled");
	printk("Max instances                : %d\n",
	       CONFIG_NET_RPL_MAX_INSTANCES);
	printk("Max DAG / instance           : %d\n",
	       CONFIG_NET_RPL_MAX_DAG_PER_INSTANCE);

	printk("Min hop rank increment       : %d\n",
	       CONFIG_NET_RPL_MIN_HOP_RANK_INC);
	printk("Initial link metric          : %d\n",
	       CONFIG_NET_RPL_INIT_LINK_METRIC);
	printk("RPL preference value         : %d\n",
	       CONFIG_NET_RPL_PREFERENCE);
	printk("DAG grounded by default      : %s\n",
	       IS_ENABLED(CONFIG_NET_RPL_GROUNDED) ? "yes" : "no");
	printk("Default instance id          : %d (0x%02x)\n",
	       CONFIG_NET_RPL_DEFAULT_INSTANCE,
	       CONFIG_NET_RPL_DEFAULT_INSTANCE);
	printk("Insert Hop-by-hop option     : %s\n",
	       IS_ENABLED(CONFIG_NET_RPL_INSERT_HBH_OPTION) ? "yes" : "no");

	printk("Specify DAG when sending DAO : %s\n",
	       IS_ENABLED(CONFIG_NET_RPL_DAO_SPECIFY_DAG) ? "yes" : "no");
	printk("DIO min interval             : %d (%d ms)\n",
	       CONFIG_NET_RPL_DIO_INTERVAL_MIN,
	       power(2, CONFIG_NET_RPL_DIO_INTERVAL_MIN));
	printk("DIO doublings interval       : %d\n",
	       CONFIG_NET_RPL_DIO_INTERVAL_DOUBLINGS);
	printk("DIO redundancy value         : %d\n",
	       CONFIG_NET_RPL_DIO_REDUNDANCY);

	printk("DAO sending timer value      : %d sec\n",
	       CONFIG_NET_RPL_DAO_TIMER);
	printk("DAO max retransmissions      : %d\n",
	       CONFIG_NET_RPL_DAO_MAX_RETRANSMISSIONS);
	printk("Node expecting DAO ack       : %s\n",
	       IS_ENABLED(CONFIG_NET_RPL_DAO_ACK) ? "yes" : "no");

	printk("Send DIS periodically        : %s\n",
	       IS_ENABLED(CONFIG_NET_RPL_DIS_SEND) ? "yes" : "no");
#if defined(CONFIG_NET_RPL_DIS_SEND)
	printk("DIS interval                 : %d sec\n",
	       CONFIG_NET_RPL_DIS_INTERVAL);
#endif

	printk("Default route lifetime unit  : %d sec\n",
	       CONFIG_NET_RPL_DEFAULT_LIFETIME_UNIT);
	printk("Default route lifetime       : %d\n",
	       CONFIG_NET_RPL_DEFAULT_LIFETIME);
#if defined(CONFIG_NET_RPL_MOP3)
	printk("Multicast route lifetime     : %d\n",
	       CONFIG_NET_RPL_MCAST_LIFETIME);
#endif
	printk("\nRuntime status\n");
	printk("==============\n");

	instance = net_rpl_get_default_instance();
	if (!instance) {
		printk("No default RPL instance found.\n");
		return 0;
	}

	printk("Default instance (id %d) : %p (%s)\n", instance->instance_id,
	       instance, instance->is_used ? "active" : "disabled");

	if (instance->default_route) {
		printk("Default route   : %s\n",
		       net_sprint_ipv6_addr(
			       &instance->default_route->address.in6_addr));
	}

#if defined(CONFIG_NET_STATISTICS_RPL)
	printk("DIO statistics  : intervals %d sent %d recv %d\n",
	       instance->dio_intervals, instance->dio_send_pkt,
	       instance->dio_recv_pkt);
#endif /* CONFIG_NET_STATISTICS_RPL */

	printk("Instance DAGs   :\n");
	for (i = 0, count = 0; i < CONFIG_NET_RPL_MAX_DAG_PER_INSTANCE; i++) {

		if (!instance->dags[i].is_used) {
			continue;
		}

		printk("[%2d]%s %s prefix %s/%d rank %d/%d ver %d flags %c%c "
			"parent %p\n",
			++count,
			&instance->dags[i] == instance->current_dag ? "*" : " ",
			net_sprint_ipv6_addr(&instance->dags[i].dag_id),
			net_sprint_ipv6_addr(
					&instance->dags[i].prefix_info.prefix),
			instance->dags[i].prefix_info.length,
			instance->dags[i].rank, instance->dags[i].min_rank,
			instance->dags[i].version,
			instance->dags[i].is_grounded ? 'G' : 'g',
			instance->dags[i].is_joined ? 'J' : 'j',
			instance->dags[i].preferred_parent);
	}
	printk("\n");

	count = 0;
	i = net_rpl_foreach_parent(rpl_parent, &count);
	if (i == 0) {
		printk("No parents found.\n");
	}

	printk("\n");
#else
	printk("RPL not enabled, set CONFIG_NET_RPL to enable it.\n");
#endif

	return 0;
}

#if defined(CONFIG_INIT_STACKS)
extern K_THREAD_STACK_DEFINE(_main_stack, CONFIG_MAIN_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(sys_work_q_stack,
			     CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE);
#endif

int net_shell_cmd_stacks(int argc, char *argv[])
{
#if defined(CONFIG_INIT_STACKS)
	unsigned int pcnt, unused;
#endif
	struct net_stack_info *info;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	for (info = __net_stack_start; info != __net_stack_end; info++) {
		net_analyze_stack_get_values(K_THREAD_STACK_BUFFER(info->stack),
					     info->size, &pcnt, &unused);

#if defined(CONFIG_INIT_STACKS)
		/* If the index is <0, then this stack is not part of stack
		 * array so do not print the index value in this case.
		 */
		if (info->idx >= 0) {
			printk("%s-%d [%s-%d] stack size %zu/%zu bytes "
			       "unused %u usage %zu/%zu (%u %%)\n",
			       info->pretty_name, info->prio, info->name,
			       info->idx, info->orig_size,
			       info->size, unused,
			       info->size - unused, info->size, pcnt);
		} else {
			printk("%s [%s] stack size %zu/%zu bytes unused %u "
			       "usage %zu/%zu (%u %%)\n",
			       info->pretty_name, info->name, info->orig_size,
			       info->size, unused,
			       info->size - unused, info->size, pcnt);
		}
#else
		printk("%s [%s] stack size %zu usage not available\n",
		       info->pretty_name, info->name, info->orig_size);
#endif
	}

#if defined(CONFIG_INIT_STACKS)
	net_analyze_stack_get_values(K_THREAD_STACK_BUFFER(_main_stack),
				     K_THREAD_STACK_SIZEOF(_main_stack),
				     &pcnt, &unused);
	printk("%s [%s] stack size %d/%d bytes unused %u usage"
	       " %d/%d (%u %%)\n",
	       "main", "_main_stack", CONFIG_MAIN_STACK_SIZE,
	       CONFIG_MAIN_STACK_SIZE, unused,
	       CONFIG_MAIN_STACK_SIZE - unused, CONFIG_MAIN_STACK_SIZE, pcnt);

	net_analyze_stack_get_values(K_THREAD_STACK_BUFFER(_interrupt_stack),
				     K_THREAD_STACK_SIZEOF(_interrupt_stack),
				     &pcnt, &unused);
	printk("%s [%s] stack size %d/%d bytes unused %u usage"
	       " %d/%d (%u %%)\n",
	       "ISR", "_interrupt_stack", CONFIG_ISR_STACK_SIZE,
	       CONFIG_ISR_STACK_SIZE, unused,
	       CONFIG_ISR_STACK_SIZE - unused, CONFIG_ISR_STACK_SIZE, pcnt);

	net_analyze_stack_get_values(K_THREAD_STACK_BUFFER(sys_work_q_stack),
				     K_THREAD_STACK_SIZEOF(sys_work_q_stack),
				     &pcnt, &unused);
	printk("%s [%s] stack size %d/%d bytes unused %u usage"
	       " %d/%d (%u %%)\n",
	       "WORKQ", "system workqueue",
	       CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE,
	       CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE, unused,
	       CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE - unused,
	       CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE, pcnt);
#else
	printk("Enable CONFIG_INIT_STACKS to see usage information.\n");
#endif

	return 0;
}

#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
static void net_shell_print_statistics_all(void)
{
	net_if_foreach(net_shell_print_statistics, NULL);
}
#endif

int net_shell_cmd_stats(int argc, char *argv[])
{
#if defined(CONFIG_NET_STATISTICS)
	int arg = 0;

	if (strcmp(argv[arg], "stats") == 0) {
		arg++;
	}

	if (!argv[arg]) {
		/* Print global network statistics */
		net_shell_print_statistics(NULL, NULL);
		return 0;
	}

#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
	if (strcmp(argv[arg], "all") == 0) {
		/* Print information about all network interfaces */
		net_shell_print_statistics_all();
	} else {
		struct net_if *iface;
		char *endptr;
		int idx;

		idx = strtol(argv[arg], &endptr, 10);
		if (*endptr != '\0') {
			printk("Invalid index %s\n", argv[arg]);
			return 0;
		}

		iface = net_if_get_by_index(idx);
		if (!iface) {
			printk("No such interface in index %d\n", idx);
			return 0;
		}

		net_shell_print_statistics(iface, NULL);
	}
#else
	printk("Per network interface statistics not collected.\n");
	printk("Please enable CONFIG_NET_STATISTICS_PER_INTERFACE\n");
#endif /* CONFIG_NET_STATISTICS_PER_INTERFACE */
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("Network statistics not compiled in.\n");
#endif

	return 0;
}

#if defined(CONFIG_NET_TCP)
static struct net_context *tcp_ctx;

#define TCP_CONNECT_TIMEOUT K_SECONDS(5) /* ms */
#define TCP_TIMEOUT K_SECONDS(2) /* ms */

static void tcp_connected(struct net_context *context,
			  int status,
			  void *user_data)
{
	if (status < 0) {
		printk("TCP connection failed (%d)\n", status);

		net_context_put(context);

		tcp_ctx = NULL;
	} else {
		printk("TCP connected\n");
	}
}

#if defined(CONFIG_NET_IPV6)
static void get_my_ipv6_addr(struct net_if *iface,
			     struct sockaddr *myaddr)
{
	const struct in6_addr *my6addr;

	my6addr = net_if_ipv6_select_src_addr(iface,
					      &net_sin6(myaddr)->sin6_addr);

	memcpy(&net_sin6(myaddr)->sin6_addr, my6addr, sizeof(struct in6_addr));

	net_sin6(myaddr)->sin6_port = 0; /* let the IP stack to select */
}
#endif

#if defined(CONFIG_NET_IPV4)
static void get_my_ipv4_addr(struct net_if *iface,
			     struct sockaddr *myaddr)
{
	/* Just take the first IPv4 address of an interface. */
	memcpy(&net_sin(myaddr)->sin_addr,
	       &iface->config.ip.ipv4->unicast[0].address.in_addr,
	       sizeof(struct in_addr));

	net_sin(myaddr)->sin_port = 0; /* let the IP stack to select */
}
#endif

static void print_connect_info(int family,
			       struct sockaddr *myaddr,
			       struct sockaddr *addr)
{
	switch (family) {
	case AF_INET:
#if defined(CONFIG_NET_IPV4)
		printk("Connecting from %s:%u ",
		       net_sprint_ipv4_addr(&net_sin(myaddr)->sin_addr),
		       ntohs(net_sin(myaddr)->sin_port));
		printk("to %s:%u\n",
		       net_sprint_ipv4_addr(&net_sin(addr)->sin_addr),
		       ntohs(net_sin(addr)->sin_port));
#else
		printk("IPv4 not supported\n");
#endif
		break;

	case AF_INET6:
#if defined(CONFIG_NET_IPV6)
		printk("Connecting from [%s]:%u ",
		       net_sprint_ipv6_addr(&net_sin6(myaddr)->sin6_addr),
		       ntohs(net_sin6(myaddr)->sin6_port));
		printk("to [%s]:%u\n",
		       net_sprint_ipv6_addr(&net_sin6(addr)->sin6_addr),
		       ntohs(net_sin6(addr)->sin6_port));
#else
		printk("IPv6 not supported\n");
#endif
		break;

	default:
		printk("Unknown protocol family (%d)\n", family);
		break;
	}
}

static int tcp_connect(char *host, u16_t port, struct net_context **ctx)
{
	struct sockaddr addr;
	struct sockaddr myaddr;
	struct net_nbr *nbr;
	struct net_if *iface = net_if_get_default();
	int addrlen;
	int family;
	int ret;

#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
	ret = net_addr_pton(AF_INET6, host, &net_sin6(&addr)->sin6_addr);
	if (ret < 0) {
		printk("Invalid IPv6 address\n");
		return 0;
	}

	net_sin6(&addr)->sin6_port = htons(port);
	addrlen = sizeof(struct sockaddr_in6);

	nbr = net_ipv6_nbr_lookup(NULL, &net_sin6(&addr)->sin6_addr);
	if (nbr) {
		iface = nbr->iface;
	}

	get_my_ipv6_addr(iface, &myaddr);
	family = addr.sa_family = myaddr.sa_family = AF_INET6;
#endif

#if defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
	ARG_UNUSED(nbr);

	ret = net_addr_pton(AF_INET, host, &net_sin(&addr)->sin_addr);
	if (ret < 0) {
		printk("Invalid IPv4 address\n");
		return 0;
	}

	get_my_ipv4_addr(iface, &myaddr);
	net_sin(&addr)->sin_port = htons(port);
	addrlen = sizeof(struct sockaddr_in);
	family = addr.sa_family = myaddr.sa_family = AF_INET;
#endif

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
	ret = net_addr_pton(AF_INET6, host, &net_sin6(&addr)->sin6_addr);
	if (ret < 0) {
		ret = net_addr_pton(AF_INET, host, &net_sin(&addr)->sin_addr);
		if (ret < 0) {
			printk("Invalid IP address\n");
			return 0;
		}

		net_sin(&addr)->sin_port = htons(port);
		addrlen = sizeof(struct sockaddr_in);

		get_my_ipv4_addr(iface, &myaddr);
		family = addr.sa_family = myaddr.sa_family = AF_INET;
	} else {
		net_sin6(&addr)->sin6_port = htons(port);
		addrlen = sizeof(struct sockaddr_in6);

		nbr = net_ipv6_nbr_lookup(NULL, &net_sin6(&addr)->sin6_addr);
		if (nbr) {
			iface = nbr->iface;
		}

		get_my_ipv6_addr(iface, &myaddr);
		family = addr.sa_family = myaddr.sa_family = AF_INET6;
	}
#endif

	print_connect_info(family, &myaddr, &addr);

	ret = net_context_get(family, SOCK_STREAM, IPPROTO_TCP, ctx);
	if (ret < 0) {
		printk("Cannot get TCP context (%d)\n", ret);
		return ret;
	}

	ret = net_context_bind(*ctx, &myaddr, addrlen);
	if (ret < 0) {
		printk("Cannot bind TCP (%d)\n", ret);
		return ret;
	}

	return net_context_connect(*ctx, &addr, addrlen, tcp_connected,
				   K_NO_WAIT, NULL);
}

static void tcp_sent_cb(struct net_context *context,
			int status,
			void *token,
			void *user_data)
{
	printk("Message sent\n");
}
#endif

int net_shell_cmd_tcp(int argc, char *argv[])
{
#if defined(CONFIG_NET_TCP)
	int arg = 1;
	int ret;

	if (argv[arg]) {
		if (!strcmp(argv[arg], "connect")) {
			/* tcp connect <ip> port */
			char *endptr;
			char *ip;
			u16_t port;

			if (tcp_ctx && net_context_is_used(tcp_ctx)) {
				printk("Already connected\n");
				return 0;
			}

			if (!argv[++arg]) {
				printk("Peer IP address missing.\n");
				return 0;
			}

			ip = argv[arg];

			if (!argv[++arg]) {
				printk("Peer port missing.\n");
				return 0;
			}

			port = strtol(argv[arg], &endptr, 10);
			if (*endptr != '\0') {
				printk("Invalid port %s\n", argv[arg]);
				return 0;
			}

			return tcp_connect(ip, port, &tcp_ctx);
		}

		if (!strcmp(argv[arg], "send")) {
			/* tcp send <data> */
			struct net_pkt *pkt;

			if (!tcp_ctx || !net_context_is_used(tcp_ctx)) {
				printk("Not connected\n");
				return 0;
			}

			if (!argv[++arg]) {
				printk("No data to send.\n");
				return 0;
			}

			pkt = net_pkt_get_tx(tcp_ctx, TCP_TIMEOUT);
			if (!pkt) {
				printk("Out of pkts, msg cannot be sent.\n");
				return 0;
			}

			ret = net_pkt_append_all(pkt, strlen(argv[arg]),
						 (u8_t *)argv[arg],
						 TCP_TIMEOUT);
			if (!ret) {
				printk("Cannot build msg (out of pkts)\n");
				net_pkt_unref(pkt);
				return 0;
			}

			ret = net_context_send(pkt, tcp_sent_cb, TCP_TIMEOUT,
					       NULL, NULL);
			if (ret < 0) {
				printk("Cannot send msg (%d)\n", ret);
				net_pkt_unref(pkt);
				return 0;
			}

			return 0;
		}

		if (!strcmp(argv[arg], "close")) {
			/* tcp close */
			if (!tcp_ctx || !net_context_is_used(tcp_ctx)) {
				printk("Not connected\n");
				return 0;
			}

			ret = net_context_put(tcp_ctx);
			if (ret < 0) {
				printk("Cannot close the connection (%d)\n",
				       ret);
				return 0;
			}

			printk("Connection closed.\n");
			tcp_ctx = NULL;

			return 0;
		}

		printk("Unknown command '%s'\n", argv[arg]);
		goto usage;
	} else {
		printk("Invalid command.\n");
	usage:
		printk("Usage:\n");
		printk("\ttcp connect <ipaddr> port\n");
		printk("\ttcp send <data>\n");
		printk("\ttcp close\n");
	}
#else
	printk("TCP not enabled.\n");
#endif /* CONFIG_NET_TCP */

	return 0;
}

#if defined(CONFIG_NET_VLAN)
static void iface_vlan_del_cb(struct net_if *iface, void *user_data)
{
	u16_t vlan_tag = POINTER_TO_UINT(user_data);
	int ret;

	ret = net_eth_vlan_disable(iface, vlan_tag);
	if (ret < 0) {
		if (ret != -ESRCH) {
			printk("Cannot delete VLAN tag %d from interface %p\n",
			       vlan_tag, iface);
		}

		return;
	}

	printk("VLAN tag %d removed from interface %p\n",
	       vlan_tag, iface);
}

static void iface_vlan_cb(struct net_if *iface, void *user_data)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	int *count = user_data;
	int i;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (*count == 0) {
		printk("    Interface  Type     Tag\n");
	}

	if (!ctx->vlan_enabled) {
		printk("VLAN tag(s) not set\n");
		return;
	}

	for (i = 0; i < NET_VLAN_MAX_COUNT; i++) {
		if (!ctx->vlan[i].iface || ctx->vlan[i].iface != iface) {
			continue;
		}

		if (ctx->vlan[i].tag == NET_VLAN_TAG_UNSPEC) {
			continue;
		}

		printk("[%d] %p %s %d\n", net_if_get_by_iface(iface), iface,
		       iface2str(iface, NULL), ctx->vlan[i].tag);

		break;
	}

	(*count)++;
}
#endif /* CONFIG_NET_VLAN */

int net_shell_cmd_vlan(int argc, char *argv[])
{
#if defined(CONFIG_NET_VLAN)
	int arg = 1;
	int ret;
	u16_t tag;

	if (!argv[arg]) {
		int count = 0;

		net_if_foreach(iface_vlan_cb, &count);

		return 0;
	}

	if (!strcmp(argv[arg], "add")) {
		/* vlan add <tag> <interface index> */
		struct net_if *iface;
		char *endptr;
		u32_t iface_idx;

		if (!argv[++arg]) {
			printk("VLAN tag missing.\n");
			return 0;
		}

		tag = strtol(argv[arg], &endptr, 10);
		if (*endptr != '\0') {
			printk("Invalid tag %s\n", argv[arg]);
			return 0;
		}

		if (!argv[++arg]) {
			printk("Network interface index missing.\n");
			return 0;
		}

		iface_idx = strtol(argv[arg], &endptr, 10);
		if (*endptr != '\0') {
			printk("Invalid index %s\n", argv[arg]);
			return 0;
		}

		iface = net_if_get_by_index(iface_idx);
		if (!iface) {
			printk("Network interface index %d is invalid.\n",
			       iface_idx);
			return 0;
		}

		if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
			printk("Network interface %p is not ethernet "
			       "interface\n", iface);
			return 0;
		}

		ret = net_eth_vlan_enable(iface, tag);
		if (ret < 0) {
			if (ret == -ENOENT) {
				printk("No IP address configured.\n");
			}

			printk("Cannot set VLAN tag (%d)\n", ret);

			return 0;
		}

		printk("VLAN tag %d set to interface %p\n", tag, iface);
		return 0;
	}

	if (!strcmp(argv[arg], "del")) {
		/* vlan del <tag> */
		char *endptr;

		if (!argv[++arg]) {
			printk("VLAN tag missing.\n");
			return 0;
		}

		tag = strtol(argv[arg], &endptr, 10);
		if (*endptr != '\0') {
			printk("Invalid tag %s\n", argv[arg]);
			return 0;
		}

		net_if_foreach(iface_vlan_del_cb,
			       UINT_TO_POINTER((u32_t)tag));

		return 0;
	}

	printk("Unknown command '%s'\n", argv[arg]);
	printk("Usage:\n");
	printk("\tvlan add <tag> <interface index>\n");
	printk("\tvlan del <tag>\n");
#else
	printk("Set CONFIG_NET_VLAN to enable virtual LAN support.\n");
#endif /* CONFIG_NET_VLAN */

	return 0;
}

static struct shell_cmd net_commands[] = {
	/* Keep the commands in alphabetical order */
	{ "allocs", net_shell_cmd_allocs,
		"\n\tPrint network memory allocations" },
	{ "app", net_shell_cmd_app,
		"\n\tPrint network application API usage information" },
	{ "arp", net_shell_cmd_arp,
		"\n\tPrint information about IPv4 ARP cache\n"
		"arp flush\n\tRemove all entries from ARP cache" },
	{ "conn", net_shell_cmd_conn,
		"\n\tPrint information about network connections" },
	{ "dns", net_shell_cmd_dns, "\n\tShow how DNS is configured\n"
		"dns cancel\n\tCancel all pending requests\n"
		"dns <hostname> [A or AAAA]\n\tQuery IPv4 address (default) or "
		"IPv6 address for a  host name" },
	{ "gptp", net_shell_cmd_gptp,
		"\n\tPrint information about gPTP support\n"
		"gptp <port>\n\tPrint detailed information about gPTP port" },
	{ "http", net_shell_cmd_http,
		"\n\tPrint information about active HTTP connections\n"
		"http monitor\n\tStart monitoring HTTP connections\n"
		"http\n\tTurn off HTTP connection monitoring" },
	{ "iface", net_shell_cmd_iface,
		"\n\tPrint information about network interfaces\n"
		"iface up [idx]\n\tTake network interface up\n"
		"iface down [idx]\n\tTake network interface down" },
	{ "ipv6", net_shell_cmd_ipv6,
		"\n\tExtra IPv6 specific information and configuration"
	},
	{ "mem", net_shell_cmd_mem,
		"\n\tPrint information about network memory usage" },
	{ "nbr", net_shell_cmd_nbr, "\n\tPrint neighbor information\n"
		"nbr rm <IPv6 address>\n\tRemove neighbor from cache" },
	{ "ping", net_shell_cmd_ping, "<host>\n\tPing a network host" },
	{ "route", net_shell_cmd_route, "\n\tShow network route" },
	{ "rpl", net_shell_cmd_rpl, "\n\tShow RPL mesh routing status" },
	{ "stacks", net_shell_cmd_stacks,
		"\n\tShow network stacks information" },
	{ "stats", net_shell_cmd_stats,
		"\n\tShow network statistics\n"
		"stats all\n\tShow network statistics for all network "
						"interfaces\n"
		"stats <idx>\n\tShow network statistics for one specific "
						"network interfaces\n" },
	{ "tcp", net_shell_cmd_tcp, "connect <ip> port\n\tConnect to TCP peer\n"
		"tcp send <data>\n\tSend data to peer using TCP\n"
		"tcp close\n\tClose TCP connection" },
	{ "vlan", net_shell_cmd_vlan, "\n\tShow VLAN information\n"
		"vlan add <vlan tag> <interface index>\n"
		"\tAdd VLAN tag to the network interface\n"
		"vlan del <vlan tag>\n"
		"\tDelete VLAN tag from the network interface\n" },
	{ NULL, NULL, NULL }
};

SHELL_REGISTER(NET_SHELL_MODULE, net_commands);
