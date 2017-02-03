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
#include <errno.h>
#include <net/net_core.h>

#include "net_stats.h"

struct net_stats net_stats;

#ifdef CONFIG_NET_STATISTICS_PERIODIC_OUTPUT

#define PRINT_STATISTICS_INTERVAL (30 * MSEC_PER_SEC)

void net_print_statistics(void)
{
	static int64_t next_print;
	int64_t curr = k_uptime_get();

	if (!next_print || (next_print < curr &&
	    (!((curr - next_print) > PRINT_STATISTICS_INTERVAL)))) {
		int64_t new_print;

#if defined(CONFIG_NET_STATISTICS_IPV6)
		NET_INFO("IPv6 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d",
			 GET_STAT(ipv6.recv),
			 GET_STAT(ipv6.sent),
			 GET_STAT(ipv6.drop),
			 GET_STAT(ipv6.forwarded));
#if defined(CONFIG_NET_STATISTICS_IPV6_ND)
		NET_INFO("IPv6 ND recv   %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(ipv6_nd.recv),
			 GET_STAT(ipv6_nd.sent),
			 GET_STAT(ipv6_nd.drop));
#endif /* CONFIG_NET_STATISTICS_IPV6_ND */
#endif /* CONFIG_NET_STATISTICS_IPV6 */

#if defined(CONFIG_NET_STATISTICS_IPV4)
		NET_INFO("IPv4 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d",
			 GET_STAT(ipv4.recv),
			 GET_STAT(ipv4.sent),
			 GET_STAT(ipv4.drop),
			 GET_STAT(ipv4.forwarded));
#endif /* CONFIG_NET_STATISTICS_IPV4 */

		NET_INFO("IP vhlerr      %d\thblener\t%d\tlblener\t%d",
			 GET_STAT(ip_errors.vhlerr),
			 GET_STAT(ip_errors.hblenerr),
			 GET_STAT(ip_errors.lblenerr));
		NET_INFO("IP fragerr     %d\tchkerr\t%d\tprotoer\t%d",
			 GET_STAT(ip_errors.fragerr),
			 GET_STAT(ip_errors.chkerr),
			 GET_STAT(ip_errors.protoerr));

		NET_INFO("ICMP recv      %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(icmp.recv),
			 GET_STAT(icmp.sent),
			 GET_STAT(icmp.drop));
		NET_INFO("ICMP typeer    %d\tchkerr\t%d",
			 GET_STAT(icmp.typeerr),
			 GET_STAT(icmp.chkerr));

#if defined(CONFIG_NET_STATISTICS_UDP)
		NET_INFO("UDP recv       %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(udp.recv),
			 GET_STAT(udp.sent),
			 GET_STAT(udp.drop));
		NET_INFO("UDP chkerr     %d",
			 GET_STAT(udp.chkerr));
#endif

#if defined(CONFIG_NET_STATISTICS_RPL_STATS)
		NET_INFO("RPL DIS recv   %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(rpl.dis.recv),
			 GET_STAT(rpl.dis.sent),
			 GET_STAT(rpl.dis.drop));
		NET_INFO("RPL DIO recv   %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(rpl.dio.recv),
			 GET_STAT(rpl.dio.sent),
			 GET_STAT(rpl.dio.drop));
		NET_INFO("RPL DAO recv   %d\tsent\t%d\tdrop\t%d\tforwarded\t%d",
			 GET_STAT(rpl.dao.recv),
			 GET_STAT(rpl.dao.sent),
			 GET_STAT(rpl.dao.drop),
			 GET_STAT(rpl.dao.forwarded));
		NET_INFO("RPL DAOACK rcv %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(rpl.dao_ack.recv),
			 GET_STAT(rpl.dao_ack.sent),
			 GET_STAT(rpl.dao_ack.drop));
		NET_INFO("RPL overflows  %d\tl-repairs\t%d\tg-repairs\t%d",
			 GET_STAT(rpl.mem_overflows),
			 GET_STAT(rpl.local_repairs),
			 GET_STAT(rpl.global_repairs));
		NET_INFO("RPL malformed  %d\tresets   \t%d\tp-switch\t%d",
			 GET_STAT(rpl.malformed_msgs),
			 GET_STAT(rpl.resets),
			 GET_STAT(rpl.parent_switch));
		NET_INFO("RPL f-errors   %d\tl-errors\t%d\tl-warnings\t%d",
			 GET_STAT(rpl.forward_errors),
			 GET_STAT(rpl.loop_errors),
			 GET_STAT(rpl.loop_warnings));
		NET_INFO("RPL r-repairs  %d",
			 GET_STAT(rpl.root_repairs));
#endif

		NET_INFO("Bytes received %u", GET_STAT(bytes.received));
		NET_INFO("Bytes sent     %u", GET_STAT(bytes.sent));
		NET_INFO("Processing err %d", GET_STAT(processing_error));

		new_print = curr + PRINT_STATISTICS_INTERVAL;
		if (new_print > curr) {
			next_print = new_print;
		} else {
			/* Overflow */
			next_print = PRINT_STATISTICS_INTERVAL -
				(LLONG_MAX - curr);
		}
	}
}

#endif /* CONFIG_NET_STATISTICS_PERIODIC_OUTPUT */

#if defined(CONFIG_NET_STATISTICS_USER_API)

static int net_stats_get(uint32_t mgmt_request, struct net_if *iface,
			 void *data, size_t len)
{
	size_t len_chk = 0;
	void *src = NULL;

	ARG_UNUSED(iface);

	switch (NET_MGMT_GET_COMMAND(mgmt_request)) {
	case NET_REQUEST_STATS_CMD_GET_ALL:
		len_chk = sizeof(struct net_stats);
		src = &net_stats;
		break;
	case NET_REQUEST_STATS_CMD_GET_PROCESSING_ERROR:
		len_chk = sizeof(net_stats_t);
		src = &net_stats.processing_error;
		break;
	case NET_REQUEST_STATS_CMD_GET_BYTES:
		len_chk = sizeof(struct net_stats_bytes);
		src = &net_stats.bytes;
		break;
	case NET_REQUEST_STATS_CMD_GET_IP_ERRORS:
		len_chk = sizeof(struct net_stats_ip_errors);
		src = &net_stats.ip_errors;
		break;
#if defined(CONFIG_NET_STATISTICS_IPV4)
	case NET_REQUEST_STATS_CMD_GET_IPV4:
		len_chk = sizeof(struct net_stats_ip);
		src = &net_stats.ipv4;
		break;
#endif
#if defined(CONFIG_NET_STATISTICS_IPV6)
	case NET_REQUEST_STATS_CMD_GET_IPV6:
		len_chk = sizeof(struct net_stats_ip);
		src = &net_stats.ipv6;
		break;
#endif
#if defined(CONFIG_NET_STATISTICS_IPV6_ND)
	case NET_REQUEST_STATS_CMD_GET_IPV6_ND:
		len_chk = sizeof(struct net_stats_ipv6_nd);
		src = &net_stats.ipv6_nd;
		break;
#endif
#if defined(CONFIG_NET_STATISTICS_ICMP)
	case NET_REQUEST_STATS_CMD_GET_ICMP:
		len_chk = sizeof(struct net_stats_icmp);
		src = &net_stats.icmp;
		break;
#endif
#if defined(CONFIG_NET_STATISTICS_UDP)
	case NET_REQUEST_STATS_CMD_GET_UDP:
		len_chk = sizeof(struct net_stats_udp);
		src = &net_stats.udp;
		break;
#endif
#if defined(CONFIG_NET_STATISTICS_TCP)
	case NET_REQUEST_STATS_CMD_GET_TCP:
		len_chk = sizeof(struct net_stats_tcp);
		src = &net_stats.tcp;
		break;
#endif
#if defined(CONFIG_NET_STATISTICS_RPL)
	case NET_REQUEST_STATS_CMD_GET_RPL:
		len_chk = sizeof(struct net_stats_rpl);
		src = &net_stats.rpl;
		break;
#endif
	}

	if (len != len_chk || !src) {
		return -EINVAL;
	}

	memcpy(src, data, len);

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
