/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_STATISTICS_PERIODIC_OUTPUT)
#define SYS_LOG_DOMAIN "net/stats"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_INFO
#define NET_LOG_ENABLED 1
#endif

#include <kernel.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <net/net_core.h>

#include "net_stats.h"

/* Global network statistics.
 *
 * The variable needs to be global so that the GET_STAT() macro can access it
 * from net_shell.c
 */
struct net_stats net_stats = { 0 };

#if defined(CONFIG_NET_STATISTICS_PERIODIC_OUTPUT)

#define PRINT_STATISTICS_INTERVAL K_SECONDS(30)

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

static inline s64_t cmp_val(u64_t val1, u64_t val2)
{
	return (s64_t)(val1 - val2);
}

static inline void stats(struct net_if *iface)
{
	static u64_t next_print;
	u64_t curr = k_uptime_get();
	s64_t cmp = cmp_val(curr, next_print);
	int i;

	if (!next_print || (abs(cmp) > PRINT_STATISTICS_INTERVAL)) {
		if (iface) {
			NET_INFO("Interface %p [%d]", iface,
				 net_if_get_by_iface(iface));
		} else {
			NET_INFO("Global statistics:");
		}

#if defined(CONFIG_NET_STATISTICS_IPV6)
		NET_INFO("IPv6 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d",
			 GET_STAT(iface, ipv6.recv),
			 GET_STAT(iface, ipv6.sent),
			 GET_STAT(iface, ipv6.drop),
			 GET_STAT(iface, ipv6.forwarded));
#if defined(CONFIG_NET_STATISTICS_IPV6_ND)
		NET_INFO("IPv6 ND recv   %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(iface, ipv6_nd.recv),
			 GET_STAT(iface, ipv6_nd.sent),
			 GET_STAT(iface, ipv6_nd.drop));
#endif /* CONFIG_NET_STATISTICS_IPV6_ND */
#if defined(CONFIG_NET_STATISTICS_MLD)
		NET_INFO("IPv6 MLD recv  %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(iface, ipv6_mld.recv),
			 GET_STAT(iface, ipv6_mld.sent),
			 GET_STAT(iface, ipv6_mld.drop));
#endif /* CONFIG_NET_STATISTICS_MLD */
#endif /* CONFIG_NET_STATISTICS_IPV6 */

#if defined(CONFIG_NET_STATISTICS_IPV4)
		NET_INFO("IPv4 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d",
			 GET_STAT(iface, ipv4.recv),
			 GET_STAT(iface, ipv4.sent),
			 GET_STAT(iface, ipv4.drop),
			 GET_STAT(iface, ipv4.forwarded));
#endif /* CONFIG_NET_STATISTICS_IPV4 */

		NET_INFO("IP vhlerr      %d\thblener\t%d\tlblener\t%d",
			 GET_STAT(iface, ip_errors.vhlerr),
			 GET_STAT(iface, ip_errors.hblenerr),
			 GET_STAT(iface, ip_errors.lblenerr));
		NET_INFO("IP fragerr     %d\tchkerr\t%d\tprotoer\t%d",
			 GET_STAT(iface, ip_errors.fragerr),
			 GET_STAT(iface, ip_errors.chkerr),
			 GET_STAT(iface, ip_errors.protoerr));

		NET_INFO("ICMP recv      %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(iface, icmp.recv),
			 GET_STAT(iface, icmp.sent),
			 GET_STAT(iface, icmp.drop));
		NET_INFO("ICMP typeer    %d\tchkerr\t%d",
			 GET_STAT(iface, icmp.typeerr),
			 GET_STAT(iface, icmp.chkerr));

#if defined(CONFIG_NET_STATISTICS_UDP)
		NET_INFO("UDP recv       %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(iface, udp.recv),
			 GET_STAT(iface, udp.sent),
			 GET_STAT(iface, udp.drop));
		NET_INFO("UDP chkerr     %d",
			 GET_STAT(iface, udp.chkerr));
#endif

#if defined(CONFIG_NET_STATISTICS_TCP)
		NET_INFO("TCP bytes recv %u\tsent\t%d",
			 GET_STAT(iface, tcp.bytes.received),
			 GET_STAT(iface, tcp.bytes.sent));
		NET_INFO("TCP seg recv   %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(iface, tcp.recv),
			 GET_STAT(iface, tcp.sent),
			 GET_STAT(iface, tcp.drop));
		NET_INFO("TCP seg resent %d\tchkerr\t%d\tackerr\t%d",
			 GET_STAT(iface, tcp.resent),
			 GET_STAT(iface, tcp.chkerr),
			 GET_STAT(iface, tcp.ackerr));
		NET_INFO("TCP seg rsterr %d\trst\t%d\tre-xmit\t%d",
			 GET_STAT(iface, tcp.rsterr),
			 GET_STAT(iface, tcp.rst),
			 GET_STAT(iface, tcp.rexmit));
		NET_INFO("TCP conn drop  %d\tconnrst\t%d",
			 GET_STAT(iface, tcp.conndrop),
			 GET_STAT(iface, tcp.connrst));
#endif

#if defined(CONFIG_NET_STATISTICS_RPL)
		NET_INFO("RPL DIS recv   %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(iface, rpl.dis.recv),
			 GET_STAT(iface, rpl.dis.sent),
			 GET_STAT(iface, rpl.dis.drop));
		NET_INFO("RPL DIO recv   %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(iface, rpl.dio.recv),
			 GET_STAT(iface, rpl.dio.sent),
			 GET_STAT(iface, rpl.dio.drop));
		NET_INFO("RPL DAO recv   %d\tsent\t%d\tdrop\t%d\tforwarded\t%d",
			 GET_STAT(iface, rpl.dao.recv),
			 GET_STAT(iface, rpl.dao.sent),
			 GET_STAT(iface, rpl.dao.drop),
			 GET_STAT(iface, rpl.dao.forwarded));
		NET_INFO("RPL DAOACK rcv %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(iface, rpl.dao_ack.recv),
			 GET_STAT(iface, rpl.dao_ack.sent),
			 GET_STAT(iface, rpl.dao_ack.drop));
		NET_INFO("RPL overflows  %d\tl-repairs\t%d\tg-repairs\t%d",
			 GET_STAT(iface, rpl.mem_overflows),
			 GET_STAT(iface, rpl.local_repairs),
			 GET_STAT(iface, rpl.global_repairs));
		NET_INFO("RPL malformed  %d\tresets   \t%d\tp-switch\t%d",
			 GET_STAT(iface, rpl.malformed_msgs),
			 GET_STAT(iface, rpl.resets),
			 GET_STAT(iface, rpl.parent_switch));
		NET_INFO("RPL f-errors   %d\tl-errors\t%d\tl-warnings\t%d",
			 GET_STAT(iface, rpl.forward_errors),
			 GET_STAT(iface, rpl.loop_errors),
			 GET_STAT(iface, rpl.loop_warnings));
		NET_INFO("RPL r-repairs  %d",
			 GET_STAT(iface, rpl.root_repairs));
#endif /* CONFIG_NET_STATISTICS_RPL */

		NET_INFO("Bytes received %u", GET_STAT(iface, bytes.received));
		NET_INFO("Bytes sent     %u", GET_STAT(iface, bytes.sent));
		NET_INFO("Processing err %d",
			 GET_STAT(iface, processing_error));

#if NET_TC_COUNT > 1
#if NET_TC_TX_COUNT > 1
		NET_INFO("TX traffic class statistics:");
		NET_INFO("TC  Priority\tSent pkts\tbytes");

		for (i = 0; i < NET_TC_TX_COUNT; i++) {
			NET_INFO("[%d] %s (%d)\t%d\t\t%d", i,
				 priority2str(GET_STAT(iface,
						       tc.sent[i].priority)),
				 GET_STAT(iface, tc.sent[i].priority),
				 GET_STAT(iface, tc.sent[i].pkts),
				 GET_STAT(iface, tc.sent[i].bytes));
		}
#endif

#if NET_TC_RX_COUNT > 1
		NET_INFO("RX traffic class statistics:");
		NET_INFO("TC  Priority\tRecv pkts\tbytes");

		for (i = 0; i < NET_TC_RX_COUNT; i++) {
			NET_INFO("[%d] %s (%d)\t%d\t\t%d", i,
				 priority2str(GET_STAT(iface,
						       tc.recv[i].priority)),
				 GET_STAT(iface, tc.recv[i].priority),
				 GET_STAT(iface, tc.recv[i].pkts),
				 GET_STAT(iface, tc.recv[i].bytes));
		}
#endif
#else /* NET_TC_COUNT > 1 */
		ARG_UNUSED(i);
#endif /* NET_TC_COUNT > 1 */

		next_print = curr + PRINT_STATISTICS_INTERVAL;
	}
}

void net_print_statistics_iface(struct net_if *iface)
{
	/* In order to make the info print lines shorter, use shorter
	 * function name.
	 */
	stats(iface);
}

static void iface_cb(struct net_if *iface, void *user_data)
{
	net_print_statistics_iface(iface);
}

void net_print_statistics_all(void)
{
	net_if_foreach(iface_cb, NULL);
}

void net_print_statistics(void)
{
	net_print_statistics_iface(NULL);
}

#endif /* CONFIG_NET_STATISTICS_PERIODIC_OUTPUT */

#if defined(CONFIG_NET_STATISTICS_USER_API)

static int net_stats_get(u32_t mgmt_request, struct net_if *iface,
			 void *data, size_t len)
{
	size_t len_chk = 0;
	void *src = NULL;

	switch (NET_MGMT_GET_COMMAND(mgmt_request)) {
	case NET_REQUEST_STATS_CMD_GET_ALL:
		len_chk = sizeof(struct net_stats);
#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
		src = iface ? &iface->stats : &net_stats;
#else
		src = &net_stats;
#endif
		break;
	case NET_REQUEST_STATS_CMD_GET_PROCESSING_ERROR:
		len_chk = sizeof(net_stats_t);
		src = GET_STAT_ADDR(iface, processing_error);
		break;
	case NET_REQUEST_STATS_CMD_GET_BYTES:
		len_chk = sizeof(struct net_stats_bytes);
		src = GET_STAT_ADDR(iface, bytes);
		break;
	case NET_REQUEST_STATS_CMD_GET_IP_ERRORS:
		len_chk = sizeof(struct net_stats_ip_errors);
		src = GET_STAT_ADDR(iface, ip_errors);
		break;
#if defined(CONFIG_NET_STATISTICS_IPV4)
	case NET_REQUEST_STATS_CMD_GET_IPV4:
		len_chk = sizeof(struct net_stats_ip);
		src = GET_STAT_ADDR(iface, ipv4);
		break;
#endif
#if defined(CONFIG_NET_STATISTICS_IPV6)
	case NET_REQUEST_STATS_CMD_GET_IPV6:
		len_chk = sizeof(struct net_stats_ip);
		src = GET_STAT_ADDR(iface, ipv6);
		break;
#endif
#if defined(CONFIG_NET_STATISTICS_IPV6_ND)
	case NET_REQUEST_STATS_CMD_GET_IPV6_ND:
		len_chk = sizeof(struct net_stats_ipv6_nd);
		src = GET_STAT_ADDR(iface, ipv6_nd);
		break;
#endif
#if defined(CONFIG_NET_STATISTICS_ICMP)
	case NET_REQUEST_STATS_CMD_GET_ICMP:
		len_chk = sizeof(struct net_stats_icmp);
		src = GET_STAT_ADDR(iface, icmp);
		break;
#endif
#if defined(CONFIG_NET_STATISTICS_UDP)
	case NET_REQUEST_STATS_CMD_GET_UDP:
		len_chk = sizeof(struct net_stats_udp);
		src = GET_STAT_ADDR(iface, udp);
		break;
#endif
#if defined(CONFIG_NET_STATISTICS_TCP)
	case NET_REQUEST_STATS_CMD_GET_TCP:
		len_chk = sizeof(struct net_stats_tcp);
		src = GET_STAT_ADDR(iface, tcp);
		break;
#endif
#if defined(CONFIG_NET_STATISTICS_RPL)
	case NET_REQUEST_STATS_CMD_GET_RPL:
		len_chk = sizeof(struct net_stats_rpl);
		src = GET_STAT_ADDR(iface, rpl);
		break;
#endif
	}

	if (len != len_chk || !src) {
		return -EINVAL;
	}

	memcpy(data, src, len);

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_ALL,
				  net_stats_get);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_PROCESSING_ERROR,
				  net_stats_get);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_BYTES,
				  net_stats_get);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IP_ERRORS,
				  net_stats_get);

#if defined(CONFIG_NET_STATISTICS_IPV4)
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IPV4,
				  net_stats_get);
#endif

#if defined(CONFIG_NET_STATISTICS_IPV6)
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IPV6,
				  net_stats_get);
#endif

#if defined(CONFIG_NET_STATISTICS_IPV6_ND)
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IPV6_ND,
				  net_stats_get);
#endif

#if defined(CONFIG_NET_STATISTICS_ICMP)
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_ICMP,
				  net_stats_get);
#endif

#if defined(CONFIG_NET_STATISTICS_UDP)
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_UDP,
				  net_stats_get);
#endif

#if defined(CONFIG_NET_STATISTICS_TCP)
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_TCP,
				  net_stats_get);
#endif

#if defined(CONFIG_NET_STATISTICS_RPL)
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_RPL,
				  net_stats_get);
#endif

#endif /* CONFIG_NET_STATISTICS_USER_API */
