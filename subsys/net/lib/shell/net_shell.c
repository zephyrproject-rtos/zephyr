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
#include <zephyr/net/ppp.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/sys/printk.h>
#if defined(CONFIG_NET_L2_ETHERNET) && defined(CONFIG_NET_L2_ETHERNET_MGMT)
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#endif /* CONFIG_NET_L2_ETHERNET */

#include "common.h"

#include "route.h"
#include "icmpv6.h"
#include "icmpv4.h"
#include "connection.h"

#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
#include <zephyr/net/ethernet_mgmt.h>
#endif

#if defined(CONFIG_NET_L2_VIRTUAL)
#include <zephyr/net/virtual.h>
#endif

#if defined(CONFIG_NET_L2_VIRTUAL_MGMT)
#include <zephyr/net/virtual_mgmt.h>
#endif

#if defined(CONFIG_NET_L2_PPP)
#include <zephyr/net/ppp.h>
#include "ppp/ppp_internal.h"
#endif

#include "net_shell.h"
#include "net_stats.h"

#include <zephyr/sys/fdtable.h>
#include "websocket/websocket_internal.h"

int get_iface_idx(const struct shell *sh, char *index_str)
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

const char *addrtype2str(enum net_addr_type addr_type)
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

const char *addrstate2str(enum net_addr_state addr_state)
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

#if defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_NATIVE)
void get_addresses(struct net_context *context,
		   char addr_local[], int local_len,
		   char addr_remote[], int remote_len)
{
	if (IS_ENABLED(CONFIG_NET_IPV6) && context->local.family == AF_INET6) {
		snprintk(addr_local, local_len, "[%s]:%u",
			 net_sprint_ipv6_addr(
				 net_sin6_ptr(&context->local)->sin6_addr),
			 ntohs(net_sin6_ptr(&context->local)->sin6_port));
		snprintk(addr_remote, remote_len, "[%s]:%u",
			 net_sprint_ipv6_addr(
				 &net_sin6(&context->remote)->sin6_addr),
			 ntohs(net_sin6(&context->remote)->sin6_port));

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && context->local.family == AF_INET) {
		snprintk(addr_local, local_len, "%s:%d",
			 net_sprint_ipv4_addr(
				 net_sin_ptr(&context->local)->sin_addr),
			 ntohs(net_sin_ptr(&context->local)->sin_port));
		snprintk(addr_remote, remote_len, "%s:%d",
			 net_sprint_ipv4_addr(
				 &net_sin(&context->remote)->sin_addr),
			 ntohs(net_sin(&context->remote)->sin_port));

	} else if (context->local.family == AF_UNSPEC) {
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
#endif /* CONFIG_NET_OFFLOAD || CONFIG_NET_NATIVE */

const char *iface2str(struct net_if *iface, const char **extra)
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

#if defined(CONFIG_NET_SHELL_DYN_CMD_COMPLETION)

SHELL_DYNAMIC_CMD_CREATE(iface_index, iface_index_get);

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
#define IFACE_PPP_DYN_CMD NULL
#endif /* CONFIG_NET_SHELL_DYN_CMD_COMPLETION */

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

SHELL_CMD_REGISTER(net_old, &net_commands, "Networking commands", NULL);

/* Placeholder for net commands that are configured in the rest of the .c files */
SHELL_SUBCMD_SET_CREATE(net_cmds, (net));

SHELL_CMD_REGISTER(net, &net_cmds, "Networking commands", NULL);

int net_shell_init(void)
{
	if (IS_ENABLED(CONFIG_NET_MGMT_EVENT_MONITOR_AUTO_START)) {
		events_enable();
	}

	return 0;
}
