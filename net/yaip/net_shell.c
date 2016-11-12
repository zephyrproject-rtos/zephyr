/** @file
 * @brief Network shell module
 *
 * Provide some networking shell commands that can be useful to applications.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <stdio.h>
#include <misc/shell.h>

#include <net/net_stats.h>
#include <net/net_if.h>

#include "route.h"
#include "icmpv6.h"

#include "net_shell.h"

/*
 * Set NET_DEBUG in order to activate address printing functions
 * in net_private.h
 */
#define NET_DEBUG 1
#include "net_private.h"

#define NET_SHELL_MODULE "net"

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

#if defined(CONFIG_NET_DHCPV4)
static inline const char *dhcpv4state2str(enum net_dhcpv4_state state)
{
	switch (state) {
	case NET_DHCPV4_INIT:
		return "init";
	case NET_DHCPV4_DISCOVER:
		return "discover";
	case NET_DHCPV4_OFFER:
		return "offer";
	case NET_DHCPV4_REQUEST:
		return "request";
	case NET_DHCPV4_RENEWAL:
		return "renewal";
	case NET_DHCPV4_ACK:
		return "ack";
	}

	return "<invalid state>";
}
#endif /* CONFIG_NET_DHCPV4 */

static void iface_cb(struct net_if *iface, void *user_data)
{
#if defined(CONFIG_NET_IPV6)
	struct net_if_ipv6_prefix *prefix;
#endif
	struct net_if_addr *unicast;
	struct net_if_mcast_addr *mcast;
	struct net_if_router *router;
	int i, count;
	unsigned stack_offset, pcnt, unused, stack_size;

	stack_size = CONFIG_NET_TX_STACK_SIZE;

	net_analyze_stack_get_values(iface->tx_stack, stack_size,
				     &stack_offset, &pcnt, &unused);

	printf("Interface %p\n", iface);
	printf("====================\n");

	printf("TX stack size %u bytes unused %u usage %u/%u (%u %%)\n",
	       stack_size + stack_offset, unused,
	       stack_size - unused, stack_size, pcnt);
	printf("Link addr : %s\n", net_sprint_ll_addr(iface->link_addr.addr,
						      iface->link_addr.len));
	printf("MTU       : %d\n", iface->mtu);

#if defined(CONFIG_NET_IPV6)
	count = 0;

	printf("IPv6 unicast addresses (max %d):\n", NET_IF_MAX_IPV6_ADDR);
	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		unicast = &iface->ipv6.unicast[i];

		if (!unicast->is_used) {
			continue;
		}

		printf("\t%s %s %s%s\n",
		       net_sprint_ipv6_addr(&unicast->address.in6_addr),
		       addrtype2str(unicast->addr_type),
		       addrstate2str(unicast->addr_state),
		       unicast->is_infinite ? " infinite" : "");
		count++;
	}

	if (count == 0) {
		printf("\t<none>\n");
	}

	count = 0;

	printf("IPv6 multicast addresses (max %d):\n", NET_IF_MAX_IPV6_MADDR);
	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		mcast = &iface->ipv6.mcast[i];

		if (!mcast->is_used) {
			continue;
		}

		printf("\t%s\n",
		       net_sprint_ipv6_addr(&mcast->address.in6_addr));

		count++;
	}

	if (count == 0) {
		printf("\t<none>\n");
	}

	count = 0;

	printf("IPv6 prefixes (max %d):\n", NET_IF_MAX_IPV6_PREFIX);
	for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
		prefix = &iface->ipv6.prefix[i];

		if (!prefix->is_used) {
			continue;
		}

		printf("\t%s/%d%s\n",
		       net_sprint_ipv6_addr(&prefix->prefix),
		       prefix->len,
		       prefix->is_infinite ? " infinite" : "");

		count++;
	}

	if (count == 0) {
		printf("\t<none>\n");
	}

	router = net_if_ipv6_router_find_default(iface, NULL);
	if (router) {
		printf("IPv6 default router :\n");
		printf("\t%s%s\n",
		       net_sprint_ipv6_addr(&router->address.in6_addr),
		       router->is_infinite ? " infinite" : "");
	}

	printf("IPv6 hop limit           : %d\n", iface->hop_limit);
	printf("IPv6 base reachable time : %d\n", iface->base_reachable_time);
	printf("IPv6 reachable time      : %d\n", iface->reachable_time);
	printf("IPv6 retransmit timer    : %d\n", iface->retrans_timer);
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	count = 0;

	printf("IPv4 unicast addresses (max %d):\n", NET_IF_MAX_IPV4_ADDR);
	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		unicast = &iface->ipv4.unicast[i];

		if (!unicast->is_used) {
			continue;
		}

		printf("\t%s %s %s%s\n",
		       net_sprint_ipv4_addr(&unicast->address.in_addr),
		       addrtype2str(unicast->addr_type),
		       addrstate2str(unicast->addr_state),
		       unicast->is_infinite ? " infinite" : "");

		count++;
	}

	if (count == 0) {
		printf("\t<none>\n");
	}

	count = 0;

	printf("IPv4 multicast addresses (max %d):\n", NET_IF_MAX_IPV4_MADDR);
	for (i = 0; i < NET_IF_MAX_IPV4_MADDR; i++) {
		mcast = &iface->ipv4.mcast[i];

		if (!mcast->is_used) {
			continue;
		}

		printf("\t%s\n",
		       net_sprint_ipv4_addr(&mcast->address.in_addr));

		count++;
	}

	if (count == 0) {
		printf("\t<none>\n");
	}

	printf("IPv4 gateway : %s\n",
	       net_sprint_ipv4_addr(&iface->ipv4.gw));
	printf("IPv4 netmask : %s\n",
	       net_sprint_ipv4_addr(&iface->ipv4.netmask));
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_DHCPV4)
	printf("DHCPv4 lease time : %lu\n", iface->dhcpv4.lease_time);
	printf("DHCPv4 renew time : %lu\n", iface->dhcpv4.renewal_time);
	printf("DHCPv4 server     : %s\n",
	       net_sprint_ipv4_addr(&iface->dhcpv4.server_id));
	printf("DHCPv4 requested  : %s\n",
	       net_sprint_ipv4_addr(&iface->dhcpv4.requested_ip));
	printf("DHCPv4 state      : %s\n",
	       dhcpv4state2str(iface->dhcpv4.state));
	printf("DHCPv4 attempts   : %d\n", iface->dhcpv4.attempts);
#endif /* CONFIG_NET_DHCPV4 */
}

#if defined(CONFIG_NET_ROUTE)
static void route_cb(struct net_route_entry *entry, void *user_data)
{
	struct net_if *iface = user_data;
	sys_snode_t *test;
	int count;

	if (entry->iface != iface) {
		return;
	}

	printf("IPv6 Route %p for interface %p\n", entry, iface);
	printf("==============================================\n");

	printf("IPv6 prefix : %s/%d\n",
	       net_sprint_ipv6_addr(&entry->addr),
	       entry->prefix_len);

	count = 0;

	printf("Next hops   :\n");

	SYS_SLIST_FOR_EACH_NODE(&entry->nexthop, test) {
		struct net_route_nexthop *nexthop_route;
		struct net_linkaddr_storage *lladdr;

		nexthop_route = CONTAINER_OF(test,
					     struct net_route_nexthop,
					     node);

		if (!nexthop_route->nbr) {
			continue;
		}

		printf("\tneighbor  : %p\n", nexthop_route->nbr);

		if (nexthop_route->nbr->idx == NET_NBR_LLADDR_UNKNOWN) {
			printf("\tlink addr : <unknown>\n");
		} else {
			lladdr = net_nbr_get_lladdr(nexthop_route->nbr->idx);

			printf("\tlink addr : %s\n",
			       net_sprint_ll_addr(lladdr->addr,
						  lladdr->len));
		}

		count++;
	}

	if (count == 0) {
		printf("\t<none>\n");
	}
}

static void iface_per_route_cb(struct net_if *iface, void *user_data)
{
	net_route_foreach(route_cb, iface);
}
#endif /* CONFIG_NET_ROUTE */

#if defined(CONFIG_NET_ROUTE_MCAST)
static void route_mcast_cb(struct net_route_entry_mcast *entry,
			   void *user_data)
{
	struct net_if *iface = user_data;

	if (entry->iface != iface) {
		return;
	}

	printf("IPv6 multicast route %p for interface %p\n", entry, iface);
	printf("========================================================\n");

	printf("IPv6 group : %s\n", net_sprint_ipv6_addr(&entry->group));
	printf("Lifetime   : %lu\n", entry->lifetime);
}

static void iface_per_mcast_route_cb(struct net_if *iface, void *user_data)
{
	net_route_mcast_foreach(route_mcast_cb, NULL, iface);
}
#endif /* CONFIG_NET_ROUTE_MCAST */

#if defined(CONFIG_NET_STATISTICS)
#define GET_STAT(s) net_stats.s

static inline void net_print_statistics(void)
{
#if defined(CONFIG_NET_IPV6)
	printf("IPv6 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
	       GET_STAT(ipv6.recv),
	       GET_STAT(ipv6.sent),
	       GET_STAT(ipv6.drop),
	       GET_STAT(ipv6.forwarded));
#if defined(CONFIG_NET_IPV6_ND)
	printf("IPv6 ND recv   %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(ipv6_nd.recv),
	       GET_STAT(ipv6_nd.sent),
	       GET_STAT(ipv6_nd.drop));
#endif /* CONFIG_NET_IPV6_ND */
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	printf("IPv4 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
	       GET_STAT(ipv4.recv),
	       GET_STAT(ipv4.sent),
	       GET_STAT(ipv4.drop),
	       GET_STAT(ipv4.forwarded));
#endif /* CONFIG_NET_IPV4 */

	printf("IP vhlerr      %d\thblener\t%d\tlblener\t%d\n",
	       GET_STAT(ip_errors.vhlerr),
	       GET_STAT(ip_errors.hblenerr),
	       GET_STAT(ip_errors.lblenerr));
	printf("IP fragerr     %d\tchkerr\t%d\tprotoer\t%d\n",
	       GET_STAT(ip_errors.fragerr),
	       GET_STAT(ip_errors.chkerr),
	       GET_STAT(ip_errors.protoerr));

	printf("ICMP recv      %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(icmp.recv),
	       GET_STAT(icmp.sent),
	       GET_STAT(icmp.drop));
	printf("ICMP typeer    %d\tchkerr\t%d\n",
	       GET_STAT(icmp.typeerr),
	       GET_STAT(icmp.chkerr));

#if defined(CONFIG_NET_UDP)
	printf("UDP recv       %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(udp.recv),
	       GET_STAT(udp.sent),
	       GET_STAT(udp.drop));
	printf("UDP chkerr     %d\n",
	       GET_STAT(udp.chkerr));
#endif

#if defined(CONFIG_NET_RPL_STATS)
	printf("RPL DIS recv   %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(rpl.dis.recv),
	       GET_STAT(rpl.dis.sent),
	       GET_STAT(rpl.dis.drop));
	printf("RPL DIO recv   %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(rpl.dio.recv),
	       GET_STAT(rpl.dio.sent),
	       GET_STAT(rpl.dio.drop));
	printf("RPL DAO recv   %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
	       GET_STAT(rpl.dao.recv),
	       GET_STAT(rpl.dao.sent),
	       GET_STAT(rpl.dao.drop),
	      GET_STAT(rpl.dao.forwarded));
	printf("RPL DAOACK rcv %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(rpl.dao_ack.recv),
	       GET_STAT(rpl.dao_ack.sent),
	       GET_STAT(rpl.dao_ack.drop));
	printf("RPL overflows  %d\tl-repairs\t%d\tg-repairs\t%d\n",
	       GET_STAT(rpl.mem_overflows),
	       GET_STAT(rpl.local_repairs),
	       GET_STAT(rpl.global_repairs));
	printf("RPL malformed  %d\tresets   \t%d\tp-switch\t%d\n",
	       GET_STAT(rpl.malformed_msgs),
	       GET_STAT(rpl.resets),
	       GET_STAT(rpl.parent_switch));
	printf("RPL f-errors   %d\tl-errors\t%d\tl-warnings\t%d\n",
	       GET_STAT(rpl.forward_errors),
	       GET_STAT(rpl.loop_errors),
	       GET_STAT(rpl.loop_warnings));
	printf("RPL r-repairs  %d\n",
	       GET_STAT(rpl.root_repairs));
#endif

	printf("Processing err %d\n", GET_STAT(processing_error));
}
#endif /* CONFIG_NET_STATISTICS */

/* Put the actual shell commands after this */

static int shell_cmd_iface(int argc, char *argv[])
{
	net_if_foreach(iface_cb, NULL);

	return 0;
}

static int shell_cmd_mem(int argc, char *argv[])
{
	size_t tx_size, rx_size, data_size;
	int tx, rx, data;

	net_nbuf_get_info(&tx_size, &rx_size, &data_size, &tx, &rx, &data);

	printf("Fragment length %d bytes\n", CONFIG_NET_NBUF_DATA_SIZE);

	printf("Network buffer pools:\n");

	printf("\tTX\t%d bytes, %d elements", tx_size,
	       CONFIG_NET_NBUF_TX_COUNT);
	if (tx >= 0) {
		printf(", available %d", tx);
	}
	printf("\n");

	printf("\tRX\t%d bytes, %d elements", rx_size,
	       CONFIG_NET_NBUF_RX_COUNT);
	if (rx >= 0) {
		printf(", available %d", rx);
	}
	printf("\n");

	printf("\tDATA\t%d bytes, %d elements", data_size,
	       CONFIG_NET_NBUF_DATA_COUNT);
	if (data >= 0) {
		printf(", available %d", data);
	}
	printf("\n");

	return 0;
}

static int shell_cmd_ping(int argc, char *argv[])
{
	struct in6_addr target;
	char *host;
	int ret;

	if (!strcmp(argv[0], "ping")) {
		host = argv[1];
	} else {
		host = argv[2];
	}

	ret = net_addr_pton(AF_INET6, host, (struct sockaddr *)&target);
	if (ret < 0) {
		printf("Invalid IPv6 address\n");
		return 0;
	}

	ret = net_icmpv6_send_echo_request(net_if_get_default(), &target,
					   sys_rand32_get(), sys_rand32_get());
	if (ret < 0) {
		printf("Cannot send ping\n");
	}

	return 0;
}

static int shell_cmd_route(int argc, char *argv[])
{
#if defined(CONFIG_NET_ROUTE)
	net_if_foreach(iface_per_route_cb, NULL);
#else
	printf("Network route support not compiled in.\n");
#endif

#if defined(CONFIG_NET_ROUTE_MCAST)
	net_if_foreach(iface_per_mcast_route_cb, NULL);
#endif

	return 0;
}

static int shell_cmd_stats(int argc, char *argv[])
{
#if defined(CONFIG_NET_STATISTICS)
	net_print_statistics();
#else
	printf("Network statistics not compiled in.\n");
#endif

	return 0;
}

static int shell_cmd_help(int argc, char *argv[])
{
	/* Keep the commands in alphabetical order */
	printf("net iface\n\tPrint information about network interfaces\n");
	printf("net mem\n\tPrint network buffer information\n");
	printf("net ping <host>\n\tPing a network host\n");
	printf("net route\n\tShow network routes\n");
	printf("net stats\n\tShow network statistics\n");
	return 0;
}

static struct shell_cmd net_commands[] = {
	/* Keep the commands in alphabetical order */
	{ "help", shell_cmd_help },
	{ "iface", shell_cmd_iface },
	{ "mem", shell_cmd_mem },
	{ "ping", shell_cmd_ping },
	{ "route", shell_cmd_route },
	{ "stats", shell_cmd_stats },
	{ NULL, NULL }
};

void net_shell_init(void)
{
	SHELL_REGISTER(NET_SHELL_MODULE, net_commands);
}
