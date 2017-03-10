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
#include <shell/shell.h>

#include <net/net_if.h>
#include <misc/printk.h>

#include "route.h"
#include "icmpv6.h"
#include "icmpv4.h"

#if defined(CONFIG_NET_TCP)
#include "tcp.h"
#endif

#if defined(CONFIG_NET_IPV6)
#include "ipv6.h"
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

static void iface_cb(struct net_if *iface, void *user_data)
{
#if defined(CONFIG_NET_IPV6)
	struct net_if_ipv6_prefix *prefix;
	struct net_if_router *router;
#endif
	struct net_if_addr *unicast;
	struct net_if_mcast_addr *mcast;
	int i, count;

	ARG_UNUSED(user_data);

	printk("Interface %p\n", iface);
	printk("====================\n");

	printk("Link addr : %s\n", net_sprint_ll_addr(iface->link_addr.addr,
						      iface->link_addr.len));
	printk("MTU       : %d\n", iface->mtu);

#if defined(CONFIG_NET_IPV6)
	count = 0;

	printk("IPv6 unicast addresses (max %d):\n", NET_IF_MAX_IPV6_ADDR);
	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		unicast = &iface->ipv6.unicast[i];

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
	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		mcast = &iface->ipv6.mcast[i];

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
	for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
		prefix = &iface->ipv6.prefix[i];

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

	printk("IPv6 hop limit           : %d\n", iface->hop_limit);
	printk("IPv6 base reachable time : %d\n", iface->base_reachable_time);
	printk("IPv6 reachable time      : %d\n", iface->reachable_time);
	printk("IPv6 retransmit timer    : %d\n", iface->retrans_timer);
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	count = 0;

	printk("IPv4 unicast addresses (max %d):\n", NET_IF_MAX_IPV4_ADDR);
	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		unicast = &iface->ipv4.unicast[i];

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
	for (i = 0; i < NET_IF_MAX_IPV4_MADDR; i++) {
		mcast = &iface->ipv4.mcast[i];

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

	printk("IPv4 gateway : %s\n",
	       net_sprint_ipv4_addr(&iface->ipv4.gw));
	printk("IPv4 netmask : %s\n",
	       net_sprint_ipv4_addr(&iface->ipv4.netmask));
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_DHCPV4)
	printk("DHCPv4 lease time : %u\n", iface->dhcpv4.lease_time);
	printk("DHCPv4 renew time : %u\n", iface->dhcpv4.renewal_time);
	printk("DHCPv4 server     : %s\n",
	       net_sprint_ipv4_addr(&iface->dhcpv4.server_id));
	printk("DHCPv4 requested  : %s\n",
	       net_sprint_ipv4_addr(&iface->dhcpv4.requested_ip));
	printk("DHCPv4 state      : %s\n",
	       net_dhcpv4_state_name(iface->dhcpv4.state));
	printk("DHCPv4 attempts   : %d\n", iface->dhcpv4.attempts);
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

	printk("IPv6 Route %p for interface %p\n", entry, iface);
	printk("==============================================\n");

	printk("IPv6 prefix : %s/%d\n",
	       net_sprint_ipv6_addr(&entry->addr),
	       entry->prefix_len);

	count = 0;

	printk("Next hops   :\n");

	SYS_SLIST_FOR_EACH_CONTAINER(&entry->nexthop, nexthop_route, node) {
		struct net_linkaddr_storage *lladdr;

		if (!nexthop_route->nbr) {
			continue;
		}

		printk("\tneighbor  : %p\n", nexthop_route->nbr);

		if (nexthop_route->nbr->idx == NET_NBR_LLADDR_UNKNOWN) {
			printk("\tlink addr : <unknown>\n");
		} else {
			lladdr = net_nbr_get_lladdr(nexthop_route->nbr->idx);

			printk("\tlink addr : %s\n",
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
	ARG_UNUSED(user_data);

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

	printk("IPv6 multicast route %p for interface %p\n", entry, iface);
	printk("========================================================\n");

	printk("IPv6 group : %s\n", net_sprint_ipv6_addr(&entry->group));
	printk("Lifetime   : %lu\n", entry->lifetime);
}

static void iface_per_mcast_route_cb(struct net_if *iface, void *user_data)
{
	net_route_mcast_foreach(route_mcast_cb, NULL, iface);
}
#endif /* CONFIG_NET_ROUTE_MCAST */

#if defined(CONFIG_NET_STATISTICS)

static inline void net_shell_print_statistics(void)
{
#if defined(CONFIG_NET_IPV6)
	printk("IPv6 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
	       GET_STAT(ipv6.recv),
	       GET_STAT(ipv6.sent),
	       GET_STAT(ipv6.drop),
	       GET_STAT(ipv6.forwarded));
#if defined(CONFIG_NET_IPV6_ND)
	printk("IPv6 ND recv   %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(ipv6_nd.recv),
	       GET_STAT(ipv6_nd.sent),
	       GET_STAT(ipv6_nd.drop));
#endif /* CONFIG_NET_IPV6_ND */
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	printk("IPv4 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
	       GET_STAT(ipv4.recv),
	       GET_STAT(ipv4.sent),
	       GET_STAT(ipv4.drop),
	       GET_STAT(ipv4.forwarded));
#endif /* CONFIG_NET_IPV4 */

	printk("IP vhlerr      %d\thblener\t%d\tlblener\t%d\n",
	       GET_STAT(ip_errors.vhlerr),
	       GET_STAT(ip_errors.hblenerr),
	       GET_STAT(ip_errors.lblenerr));
	printk("IP fragerr     %d\tchkerr\t%d\tprotoer\t%d\n",
	       GET_STAT(ip_errors.fragerr),
	       GET_STAT(ip_errors.chkerr),
	       GET_STAT(ip_errors.protoerr));

	printk("ICMP recv      %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(icmp.recv),
	       GET_STAT(icmp.sent),
	       GET_STAT(icmp.drop));
	printk("ICMP typeer    %d\tchkerr\t%d\n",
	       GET_STAT(icmp.typeerr),
	       GET_STAT(icmp.chkerr));

#if defined(CONFIG_NET_UDP)
	printk("UDP recv       %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(udp.recv),
	       GET_STAT(udp.sent),
	       GET_STAT(udp.drop));
	printk("UDP chkerr     %d\n",
	       GET_STAT(udp.chkerr));
#endif

#if defined(CONFIG_NET_RPL_STATS)
	printk("RPL DIS recv   %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(rpl.dis.recv),
	       GET_STAT(rpl.dis.sent),
	       GET_STAT(rpl.dis.drop));
	printk("RPL DIO recv   %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(rpl.dio.recv),
	       GET_STAT(rpl.dio.sent),
	       GET_STAT(rpl.dio.drop));
	printk("RPL DAO recv   %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
	       GET_STAT(rpl.dao.recv),
	       GET_STAT(rpl.dao.sent),
	       GET_STAT(rpl.dao.drop),
	      GET_STAT(rpl.dao.forwarded));
	printk("RPL DAOACK rcv %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(rpl.dao_ack.recv),
	       GET_STAT(rpl.dao_ack.sent),
	       GET_STAT(rpl.dao_ack.drop));
	printk("RPL overflows  %d\tl-repairs\t%d\tg-repairs\t%d\n",
	       GET_STAT(rpl.mem_overflows),
	       GET_STAT(rpl.local_repairs),
	       GET_STAT(rpl.global_repairs));
	printk("RPL malformed  %d\tresets   \t%d\tp-switch\t%d\n",
	       GET_STAT(rpl.malformed_msgs),
	       GET_STAT(rpl.resets),
	       GET_STAT(rpl.parent_switch));
	printk("RPL f-errors   %d\tl-errors\t%d\tl-warnings\t%d\n",
	       GET_STAT(rpl.forward_errors),
	       GET_STAT(rpl.loop_errors),
	       GET_STAT(rpl.loop_warnings));
	printk("RPL r-repairs  %d\n",
	       GET_STAT(rpl.root_repairs));
#endif

	printk("Bytes received %u\n", GET_STAT(bytes.received));
	printk("Bytes sent     %u\n", GET_STAT(bytes.sent));
	printk("Processing err %d\n", GET_STAT(processing_error));
}
#endif /* CONFIG_NET_STATISTICS */

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
	char addr_str[ADDR_LEN];

	if (context->local.family == AF_INET6) {
		snprintk(addr_str, ADDR_LEN, "%s",
			 net_sprint_ipv6_addr(
				 net_sin6_ptr(&context->local)->sin6_addr));

		printk("%p\t%16s\t%16s\t%p\t%c%c%c\n", context, addr_str,
		       net_sprint_ipv6_addr(
			       &net_sin6(&context->remote)->sin6_addr),
		       net_context_get_iface(context),
		       net_context_get_family(context) == AF_INET6 ? '6' : '4',
		       net_context_get_type(context) == SOCK_DGRAM ? 'D' : 'S',
		       net_context_get_ip_proto(context) == IPPROTO_UDP ?
		       'U' : 'T');

		(*count)++;

		return;
	}

	if (context->local.family == AF_INET) {
		snprintk(addr_str, ADDR_LEN, "%s",
			 net_sprint_ipv4_addr(
				 net_sin_ptr(&context->local)->sin_addr));

		printk("%p\t%16s\t%16s\t%p\t%c%c%c\n", context, addr_str,
		       net_sprint_ipv4_addr(
			       &net_sin(&context->remote)->sin_addr),
		       net_context_get_iface(context),
		       net_context_get_family(context) == AF_INET6 ? '6' : '4',
		       net_context_get_type(context) == SOCK_DGRAM ? 'D' : 'S',
		       net_context_get_ip_proto(context) == IPPROTO_UDP ?
		       'U' : 'T');

		(*count)++;

		return;
	}

	if (context->local.family != AF_UNSPEC) {
		printk("Invalid address family (%d) for context %p\n",
		       context->local.family, context);
	}
}

#if defined(CONFIG_NET_TCP)
static void tcp_cb(struct net_tcp *tcp, void *user_data)
{
	int *count = user_data;
	uint16_t recv_mss = net_tcp_get_recv_mss(tcp);

	printk("%p\t%12s\t%10u%10u%11u%11u%5u\n",
	       tcp, net_tcp_state_str(net_tcp_get_state(tcp)),
	       ntohs(net_sin6_ptr(&tcp->context->local)->sin6_port),
	       ntohs(net_sin6(&tcp->context->remote)->sin6_port),
	       tcp->send_seq, tcp->send_ack, recv_mss);

	(*count)++;
}
#endif

#if defined(CONFIG_NET_DEBUG_NET_BUF)
static void allocs_cb(struct net_buf *buf,
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

	if (func_alloc) {
		if (in_use) {
			printk("%p/%d\t%5s\t%5s\t%s():%d\n", buf, buf->ref,
			       str, net_nbuf_pool2str(buf->pool), func_alloc,
			       line_alloc);
		} else {
			printk("%p\t%5s\t%5s\t%s():%d -> %s():%d\n", buf,
			       str, net_nbuf_pool2str(buf->pool), func_alloc,
			       line_alloc, func_free, line_free);
		}
	}
}
#endif /* CONFIG_NET_DEBUG_NET_BUF */

/* Put the actual shell commands after this */

static int shell_cmd_allocs(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_DEBUG_NET_BUF)
	printk("Network buffer allocations\n\n");
	printk("net_buf\t\tStatus\tPool\tFunction alloc -> freed\n");
	net_nbuf_allocs_foreach(allocs_cb, NULL);
#else
	printk("Enable CONFIG_NET_DEBUG_NET_BUF to see allocations.\n");
#endif /* CONFIG_NET_DEBUG_NET_BUF */

	return 0;
}

static int shell_cmd_conn(int argc, char *argv[])
{
	int count = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("Context   \tLocal           \tRemote          \tIface     \t"
	       "Flags\n");

	net_context_foreach(context_cb, &count);

	if (count == 0) {
		printk("No connections\n");
	}

#if defined(CONFIG_NET_TCP)
	printk("\nTCP       \tState    \tSrc port  Dst port  "
	       "Send-Seq   Send-Ack   MSS\n");

	count = 0;

	net_tcp_foreach(tcp_cb, &count);

	if (count == 0) {
		printk("No TCP connections\n");
	}
#endif

	return 0;
}

static int shell_cmd_iface(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	net_if_foreach(iface_cb, NULL);

	return 0;
}

struct ctx_info {
	int pos;
	bool are_external_pools;
	struct net_buf_pool *tx_pools[CONFIG_NET_MAX_CONTEXTS];
	struct net_buf_pool *data_pools[CONFIG_NET_MAX_CONTEXTS];
};

#if defined(CONFIG_NET_CONTEXT_NBUF_POOL)
static bool pool_found_already(struct ctx_info *info,
			       struct net_buf_pool *pool)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_CONTEXTS; i++) {
		if (info->tx_pools[i] == pool ||
		    info->data_pools[i] == pool) {
			return true;
		}
	}

	return false;
}
#endif

static void context_info(struct net_context *context, void *user_data)
{
#if defined(CONFIG_NET_CONTEXT_NBUF_POOL)
	struct ctx_info *info = user_data;
	struct net_buf_pool *pool;

	if (!net_context_is_used(context)) {
		return;
	}

	if (context->tx_pool) {
		pool = context->tx_pool();

		if (pool_found_already(info, pool)) {
			return;
		}

#if defined(CONFIG_NET_DEBUG_NET_BUF)
		printk("ETX (%s)\t%d\t%d\t%d\t%p\n",
		       pool->name, pool->pool_size, pool->buf_count,
		       pool->avail_count, pool);
#else
		printk("ETX     \t%d\t%p\n", pool->buf_count, pool);
#endif
		info->are_external_pools = true;
		info->tx_pools[info->pos] = pool;
	}

	if (context->data_pool) {
		pool = context->data_pool();

		if (pool_found_already(info, pool)) {
			return;
		}

#if defined(CONFIG_NET_DEBUG_NET_BUF)
		printk("EDATA (%s)\t%d\t%d\t%d\t%p\n",
		       pool->name, pool->pool_size, pool->buf_count,
		       pool->avail_count, pool);
#else
		printk("EDATA   \t%d\t%p\n", pool->buf_count, pool);
#endif
		info->are_external_pools = true;
		info->data_pools[info->pos] = pool;
	}

	info->pos++;
#endif /* CONFIG_NET_CONTEXT_NBUF_POOL */
}

static int shell_cmd_mem(int argc, char *argv[])
{
	struct net_buf_pool *tx, *rx, *rx_data, *tx_data;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	net_nbuf_get_info(&rx, &tx, &rx_data, &tx_data);

	printk("Fragment length %d bytes\n", CONFIG_NET_NBUF_DATA_SIZE);

	printk("Network buffer pools:\n");

#if defined(CONFIG_NET_DEBUG_NET_BUF)
	printk("Name\t\t\tSize\tCount\tAvail\tAddress\n");

	printk("RX (%s)\t\t%d\t%d\t%d\t%p\n",
	       rx->name, rx->pool_size, rx->buf_count, rx->avail_count, rx);

	printk("TX (%s)\t\t%d\t%d\t%d\t%p\n",
	       tx->name, tx->pool_size, tx->buf_count, tx->avail_count, tx);

	printk("RX DATA (%s)\t%d\t%d\t%d\t%p\n",
	       rx_data->name, rx_data->pool_size, rx_data->buf_count,
	       rx_data->avail_count, rx_data);

	printk("TX DATA (%s)\t%d\t%d\t%d\t%p\n",
	       tx_data->name, tx_data->pool_size, tx_data->buf_count,
	       tx_data->avail_count, tx_data);
#else
	printk("Name    \tCount\tAddress\n");

	printk("RX      \t%d\t%p\n", rx->buf_count, rx);
	printk("TX      \t%d\t%p\n", tx->buf_count, tx);
	printk("RX DATA \t%d\t%p\n", rx_data->buf_count, rx_data);
	printk("TX DATA \t%d\t%p\n", tx_data->buf_count, tx_data);
#endif /* CONFIG_NET_DEBUG_NET_BUF */

	if (IS_ENABLED(CONFIG_NET_CONTEXT_NBUF_POOL)) {
		struct ctx_info info;

		memset(&info, 0, sizeof(info));
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

	if (*count == 0) {
		printk("     Neighbor   Flags   Interface  State\t"
		       "Remain\tLink              Address\n");
	}

	(*count)++;

	printk("[%2d] %p %d/%d/%d/%d %p %9s\t%6d\t%17s %s\n",
	       *count, nbr, nbr->ref, net_ipv6_nbr_data(nbr)->ns_count,
	       net_ipv6_nbr_data(nbr)->is_router,
	       net_ipv6_nbr_data(nbr)->link_metric,
	       nbr->iface,
	       net_nbr_state2str(net_ipv6_nbr_data(nbr)->state),
	       k_delayed_work_remaining_get(
		       &net_ipv6_nbr_data(nbr)->reachable),
	       nbr->idx == NET_NBR_LLADDR_UNKNOWN ? "?" :
	       net_sprint_ll_addr(
		       net_nbr_get_lladdr(nbr->idx)->addr,
		       net_nbr_get_lladdr(nbr->idx)->len),
	       net_sprint_ipv6_addr(&net_ipv6_nbr_data(nbr)->addr));
}
#endif

static int shell_cmd_nbr(int argc, char *argv[])
{
#if defined(CONFIG_NET_IPV6)
	int count = 0;
	int arg = 1;

	if (strcmp(argv[0], "nbr")) {
		arg++;
	}

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

		if (!net_ipv6_nbr_rm(net_if_get_default(), &addr)) {
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

static int shell_cmd_ping(int argc, char *argv[])
{
#if defined(CONFIG_NET_IPV6)
	struct in6_addr ipv6_target;
#endif
#if defined(CONFIG_NET_IPV4)
	struct in_addr ipv4_target;
#endif
	char *host;
	int ret;

	ARG_UNUSED(argc);

	if (!strcmp(argv[0], "ping")) {
		host = argv[1];
	} else {
		host = argv[2];
	}

#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
	ret = net_addr_pton(AF_INET6, host, &ipv6_target);
	if (ret < 0) {
		printk("Invalid IPv6 address\n");
		return 0;
	}

	ret = net_icmpv6_send_echo_request(net_if_get_default(),
					   &ipv6_target,
					   sys_rand32_get(),
					   sys_rand32_get());
	if (ret < 0) {
		printk("Cannot send IPv6 ping\n");
	}
#endif

#if defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
	ret = net_addr_pton(AF_INET, host, &ipv4_target);
	if (ret < 0) {
		printk("Invalid IPv4 address\n");
		return 0;
	}

	ret = net_icmpv4_send_echo_request(net_if_get_default(),
					   &ipv4_target,
					   sys_rand32_get(),
					   sys_rand32_get());
	if (ret < 0) {
		printk("Cannot send IPv4 ping\n");
	}
#endif

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
	ret = net_addr_pton(AF_INET6, host, &ipv6_target);
	if (ret < 0) {
		ret = net_addr_pton(AF_INET, host, &ipv4_target);
		if (ret < 0) {
			printk("Invalid IP address\n");
			return 0;
		}

		ret = net_icmpv4_send_echo_request(net_if_get_default(),
						   &ipv4_target,
						   sys_rand32_get(),
						   sys_rand32_get());
		if (ret < 0) {
			printk("Cannot send IPv4 ping\n");
		}

		return 0;
	} else {
		ret = net_icmpv6_send_echo_request(net_if_get_default(),
						   &ipv6_target,
						   sys_rand32_get(),
						   sys_rand32_get());
		if (ret < 0) {
			printk("Cannot send IPv6 ping\n");
		}
	}
#endif

	return 0;
}

static int shell_cmd_route(int argc, char *argv[])
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

#if defined(CONFIG_INIT_STACKS)
extern char _main_stack[];
extern char _interrupt_stack[];
#endif

static int shell_cmd_stacks(int argc, char *argv[])
{
#if defined(CONFIG_INIT_STACKS)
	unsigned int stack_offset, pcnt, unused;
#endif
	struct net_stack_info *info;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	for (info = __net_stack_start; info != __net_stack_end; info++) {
		net_analyze_stack_get_values(info->stack, info->size,
					     &stack_offset, &pcnt, &unused);

#if defined(CONFIG_INIT_STACKS)
		printk("%s [%s] stack size %zu/%zu bytes unused %u usage"
		       " %zu/%zu (%u %%)\n",
		       info->pretty_name, info->name, info->orig_size,
		       info->size + stack_offset, unused,
		       info->size - unused, info->size, pcnt);
#else
		printk("%s [%s] stack size %u usage not available\n",
		       info->pretty_name, info->name, info->orig_size);
#endif
	}

#if defined(CONFIG_INIT_STACKS)
	net_analyze_stack_get_values(_main_stack, CONFIG_MAIN_STACK_SIZE,
				     &stack_offset, &pcnt, &unused);
	printk("%s [%s] stack size %d/%d bytes unused %u usage"
	       " %d/%d (%u %%)\n",
	       "main", "_main_stack", CONFIG_MAIN_STACK_SIZE,
	       CONFIG_MAIN_STACK_SIZE + stack_offset, unused,
	       CONFIG_MAIN_STACK_SIZE - unused, CONFIG_MAIN_STACK_SIZE, pcnt);

	net_analyze_stack_get_values(_interrupt_stack, CONFIG_ISR_STACK_SIZE,
				     &stack_offset, &pcnt, &unused);
	printk("%s [%s] stack size %d/%d bytes unused %u usage"
	       " %d/%d (%u %%)\n",
	       "ISR", "_interrupt_stack", CONFIG_ISR_STACK_SIZE,
	       CONFIG_ISR_STACK_SIZE + stack_offset, unused,
	       CONFIG_ISR_STACK_SIZE - unused, CONFIG_ISR_STACK_SIZE, pcnt);
#endif

	return 0;
}

static int shell_cmd_stats(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_STATISTICS)
	net_shell_print_statistics();
#else
	printk("Network statistics not compiled in.\n");
#endif

	return 0;
}

static int shell_cmd_help(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Keep the commands in alphabetical order */
	printk("net allocs\n\tPrint network buffer allocations\n");
	printk("net conn\n\tPrint information about network connections\n");
	printk("net iface\n\tPrint information about network interfaces\n");
	printk("net mem\n\tPrint network buffer information\n");
	printk("net nbr\n\tPrint neighbor information\n");
	printk("net nbr rm <IPv6 address>\n\tRemove neighbor from cache\n");
	printk("net ping <host>\n\tPing a network host\n");
	printk("net route\n\tShow network routes\n");
	printk("net stacks\n\tShow network stacks information\n");
	printk("net stats\n\tShow network statistics\n");
	return 0;
}

static struct shell_cmd net_commands[] = {
	/* Keep the commands in alphabetical order */
	{ "allocs", shell_cmd_allocs, NULL },
	{ "conn", shell_cmd_conn, NULL },
	{ "help", shell_cmd_help, NULL },
	{ "iface", shell_cmd_iface, NULL },
	{ "mem", shell_cmd_mem, NULL },
	{ "nbr", shell_cmd_nbr, NULL },
	{ "ping", shell_cmd_ping, NULL },
	{ "route", shell_cmd_route, NULL },
	{ "stacks", shell_cmd_stacks, NULL },
	{ "stats", shell_cmd_stats, NULL },
	{ NULL, NULL, NULL }
};

void net_shell_init(void)
{
	SHELL_REGISTER(NET_SHELL_MODULE, net_commands);
}
