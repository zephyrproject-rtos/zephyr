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

#if defined(CONFIG_NET_DEBUG_CORE)
#define SYS_LOG_DOMAIN "net/core"
#endif

#include <kernel.h>
#include <string.h>
#include <net/net_core.h>

#include "net_stats.h"

struct net_stats net_stats;

#define PRINT_STATISTICS_INTERVAL (30 * MSEC_PER_SEC)

void net_print_statistics(void)
{
	static int64_t next_print;
	int64_t curr = k_uptime_get();

	if (!next_print || (next_print < curr &&
	    (!((curr - next_print) > PRINT_STATISTICS_INTERVAL)))) {
		int64_t new_print;

#if defined(CONFIG_NET_IPV6)
		NET_INFO("IPv6 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d",
			 GET_STAT(ipv6.recv),
			 GET_STAT(ipv6.sent),
			 GET_STAT(ipv6.drop),
			 GET_STAT(ipv6.forwarded));
#if defined(CONFIG_NET_IPV6_ND)
		NET_INFO("IPv6 ND recv   %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(ipv6_nd.recv),
			 GET_STAT(ipv6_nd.sent),
			 GET_STAT(ipv6_nd.drop));
#endif /* CONFIG_NET_IPV6_ND */
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
		NET_INFO("IPv4 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d",
			 GET_STAT(ipv4.recv),
			 GET_STAT(ipv4.sent),
			 GET_STAT(ipv4.drop),
			 GET_STAT(ipv4.forwarded));
#endif /* CONFIG_NET_IPV4 */

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

#if defined(CONFIG_NET_UDP)
		NET_INFO("UDP recv       %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(udp.recv),
			 GET_STAT(udp.sent),
			 GET_STAT(udp.drop));
		NET_INFO("UDP chkerr     %d",
			 GET_STAT(udp.chkerr));
#endif

#if defined(CONFIG_NET_RPL_STATS)
		NET_INFO("RPL DIS recv   %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(rpl.dis.recv),
			 GET_STAT(rpl.dis.sent),
			 GET_STAT(rpl.dis.drop));
		NET_INFO("RPL DIO recv   %d\tsent\t%d\tdrop\t%d",
			 GET_STAT(rpl.dio.recv),
			 GET_STAT(rpl.dio..sent),
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
