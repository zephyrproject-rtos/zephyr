/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_STATS_H__
#define __NET_STATS_H__

#if defined(CONFIG_NET_STATISTICS)

#include <net/net_ip.h>
#include <net/net_stats.h>

extern struct net_stats net_stats;

#define GET_STAT(s) net_stats.s

/* Core stats */

static inline void net_stats_update_processing_error(void)
{
	net_stats.processing_error++;
}

static inline void net_stats_update_ip_errors_protoerr(void)
{
	net_stats.ip_errors.protoerr++;
}

static inline void net_stats_update_ip_errors_vhlerr(void)
{
	net_stats.ip_errors.vhlerr++;
}

static inline void net_stats_update_bytes_recv(u32_t bytes)
{
	net_stats.bytes.received += bytes;
}

static inline void net_stats_update_bytes_sent(u32_t bytes)
{
	net_stats.bytes.sent += bytes;
}
#else
#define net_stats_update_processing_error()
#define net_stats_update_ip_errors_protoerr()
#define net_stats_update_ip_errors_vhlerr()
#define net_stats_update_bytes_recv(...)
#define net_stats_update_bytes_sent(...)
#endif /* CONFIG_NET_STATISTICS */

#if defined(CONFIG_NET_STATISTICS_IPV6)
/* IPv6 stats */

static inline void net_stats_update_ipv6_sent(void)
{
	net_stats.ipv6.sent++;
}

static inline void net_stats_update_ipv6_recv(void)
{
	net_stats.ipv6.recv++;
}

static inline void net_stats_update_ipv6_drop(void)
{
	net_stats.ipv6.drop++;
}
#else
#define net_stats_update_ipv6_drop()
#define net_stats_update_ipv6_sent()
#define net_stats_update_ipv6_recv()
#endif /* CONFIG_NET_STATISTICS_IPV6 */

#if defined(CONFIG_NET_STATISTICS_IPV6_ND)
/* IPv6 Neighbor Discovery stats*/

static inline void net_stats_update_ipv6_nd_sent(void)
{
	net_stats.ipv6_nd.sent++;
}

static inline void net_stats_update_ipv6_nd_recv(void)
{
	net_stats.ipv6_nd.recv++;
}

static inline void net_stats_update_ipv6_nd_drop(void)
{
	net_stats.ipv6_nd.drop++;
}
#else
#define net_stats_update_ipv6_nd_sent()
#define net_stats_update_ipv6_nd_recv()
#define net_stats_update_ipv6_nd_drop()
#endif /* CONFIG_NET_STATISTICS_IPV6_ND */

#if defined(CONFIG_NET_STATISTICS_IPV4)
/* IPv4 stats */

static inline void net_stats_update_ipv4_drop(void)
{
	net_stats.ipv4.drop++;
}

static inline void net_stats_update_ipv4_sent(void)
{
	net_stats.ipv4.sent++;
}

static inline void net_stats_update_ipv4_recv(void)
{
	net_stats.ipv4.recv++;
}
#else
#define net_stats_update_ipv4_drop()
#define net_stats_update_ipv4_sent()
#define net_stats_update_ipv4_recv()
#endif /* CONFIG_NET_STATISTICS_IPV4 */

#if defined(CONFIG_NET_STATISTICS_ICMP)
/* Common ICMPv4/ICMPv6 stats */
static inline void net_stats_update_icmp_sent(void)
{
	net_stats.icmp.sent++;
}

static inline void net_stats_update_icmp_recv(void)
{
	net_stats.icmp.recv++;
}

static inline void net_stats_update_icmp_drop(void)
{
	net_stats.icmp.drop++;
}
#else
#define net_stats_update_icmp_sent()
#define net_stats_update_icmp_recv()
#define net_stats_update_icmp_drop()
#endif /* CONFIG_NET_STATISTICS_ICMP */

#if defined(CONFIG_NET_STATISTICS_UDP)
/* UDP stats */
static inline void net_stats_update_udp_sent(void)
{
	net_stats.udp.sent++;
}

static inline void net_stats_update_udp_recv(void)
{
	net_stats.udp.recv++;
}

static inline void net_stats_update_udp_drop(void)
{
	net_stats.udp.drop++;
}

static inline void net_stats_update_udp_chkerr(void)
{
	net_stats.udp.chkerr++;
}
#else
#define net_stats_update_udp_sent()
#define net_stats_update_udp_recv()
#define net_stats_update_udp_drop()
#define net_stats_update_udp_chkerr()
#endif /* CONFIG_NET_STATISTICS_UDP */

#if defined(CONFIG_NET_STATISTICS_TCP)
/* TCP stats */
static inline void net_stats_update_tcp_sent(u32_t bytes)
{
	net_stats.tcp.bytes.sent += bytes;
}

static inline void net_stats_update_tcp_recv(u32_t bytes)
{
	net_stats.tcp.bytes.received += bytes;
}

static inline void net_stats_update_tcp_resent(u32_t bytes)
{
	net_stats.tcp.resent += bytes;
}

static inline void net_stats_update_tcp_seg_sent(void)
{
	net_stats.tcp.sent++;
}

static inline void net_stats_update_tcp_seg_recv(void)
{
	net_stats.tcp.recv++;
}

static inline void net_stats_update_tcp_seg_drop(void)
{
	net_stats.tcp.drop++;
}

static inline void net_stats_update_tcp_seg_rst(void)
{
	net_stats.tcp.rst++;
}

static inline void net_stats_update_tcp_seg_conndrop(void)
{
	net_stats.tcp.conndrop++;
}

static inline void net_stats_update_tcp_seg_connrst(void)
{
	net_stats.tcp.connrst++;
}

static inline void net_stats_update_tcp_seg_chkerr(void)
{
	net_stats.tcp.chkerr++;
}

static inline void net_stats_update_tcp_seg_ackerr(void)
{
	net_stats.tcp.ackerr++;
}

static inline void net_stats_update_tcp_seg_rsterr(void)
{
	net_stats.tcp.rsterr++;
}

static inline void net_stats_update_tcp_seg_rexmit(void)
{
	net_stats.tcp.rexmit++;
}
#else
#define net_stats_update_tcp_sent(...)
#define net_stats_update_tcp_resent(...)
#define net_stats_update_tcp_recv(...)
#define net_stats_update_tcp_seg_sent()
#define net_stats_update_tcp_seg_recv()
#define net_stats_update_tcp_seg_drop()
#define net_stats_update_tcp_seg_rst()
#define net_stats_update_tcp_seg_conndrop()
#define net_stats_update_tcp_seg_connrst()
#define net_stats_update_tcp_seg_chkerr()
#define net_stats_update_tcp_seg_ackerr()
#define net_stats_update_tcp_seg_rsterr()
#define net_stats_update_tcp_seg_rexmit()
#endif /* CONFIG_NET_STATISTICS_TCP */

static inline void net_stats_update_per_proto_recv(enum net_ip_protocol proto)
{
	if (IS_ENABLED(CONFIG_NET_UDP) && proto == IPPROTO_UDP) {
		net_stats_update_udp_recv();
	} else if (IS_ENABLED(CONFIG_NET_TCP) && proto == IPPROTO_TCP) {
		net_stats_update_tcp_seg_recv();
	}
}

static inline void net_stats_update_per_proto_drop(enum net_ip_protocol proto)
{
	if (IS_ENABLED(CONFIG_NET_UDP) && proto == IPPROTO_UDP) {
		net_stats_update_udp_drop();
	} else if (IS_ENABLED(CONFIG_NET_TCP) && proto == IPPROTO_TCP) {
		net_stats_update_tcp_seg_drop();
	}
}

#if defined(CONFIG_NET_STATISTICS_RPL)
/* RPL stats */
static inline void net_stats_update_rpl_resets(void)
{
	net_stats.rpl.resets++;
}

static inline void net_stats_update_rpl_mem_overflows(void)
{
	net_stats.rpl.mem_overflows++;
}

static inline void net_stats_update_rpl_parent_switch(void)
{
	net_stats.rpl.parent_switch++;
}

static inline void net_stats_update_rpl_local_repairs(void)
{
	net_stats.rpl.local_repairs++;
}

static inline void net_stats_update_rpl_global_repairs(void)
{
	net_stats.rpl.global_repairs++;
}

static inline void net_stats_update_rpl_root_repairs(void)
{
	net_stats.rpl.root_repairs++;
}

static inline void net_stats_update_rpl_malformed_msgs(void)
{
	net_stats.rpl.malformed_msgs++;
}

static inline void net_stats_update_rpl_forward_errors(void)
{
	net_stats.rpl.forward_errors++;
}

static inline void net_stats_update_rpl_loop_errors(void)
{
	net_stats.rpl.loop_errors++;
}

static inline void net_stats_update_rpl_loop_warnings(void)
{
	net_stats.rpl.loop_warnings++;
}

static inline void net_stats_update_rpl_dis_sent(void)
{
	net_stats.rpl.dis.sent++;
}

static inline void net_stats_update_rpl_dio_sent(void)
{
	net_stats.rpl.dio.sent++;
}

static inline void net_stats_update_rpl_dao_sent(void)
{
	net_stats.rpl.dao.sent++;
}

static inline void net_stats_update_rpl_dao_forwarded(void)
{
	net_stats.rpl.dao.forwarded++;
}

static inline void net_stats_update_rpl_dao_ack_sent(void)
{
	net_stats.rpl.dao_ack.sent++;
}

static inline void net_stats_update_rpl_dao_ack_recv(void)
{
	net_stats.rpl.dao_ack.recv++;
}
#else
#define net_stats_update_rpl_resets()
#define net_stats_update_rpl_mem_overflows()
#define net_stats_update_rpl_parent_switch()
#define net_stats_update_rpl_local_repairs()
#define net_stats_update_rpl_global_repairs()
#define net_stats_update_rpl_root_repairs()
#define net_stats_update_rpl_malformed_msgs()
#define net_stats_update_rpl_forward_errors()
#define net_stats_update_rpl_loop_errors()
#define net_stats_update_rpl_loop_warnings()
#define net_stats_update_rpl_dis_sent()
#define net_stats_update_rpl_dio_sent()
#define net_stats_update_rpl_dao_sent()
#define net_stats_update_rpl_dao_forwarded()
#define net_stats_update_rpl_dao_ack_sent()
#define net_stats_update_rpl_dao_ack_recv()
#endif /* CONFIG_NET_STATISTICS_RPL */

#if defined(CONFIG_NET_STATISTICS_MLD)
static inline void net_stats_update_ipv6_mld_recv(void)
{
	net_stats.ipv6_mld.recv++;
}

static inline void net_stats_update_ipv6_mld_sent(void)
{
	net_stats.ipv6_mld.sent++;
}

static inline void net_stats_update_ipv6_mld_drop(void)
{
	net_stats.ipv6_mld.drop++;
}
#else
#define net_stats_update_ipv6_mld_recv()
#define net_stats_update_ipv6_mld_sent()
#define net_stats_update_ipv6_mld_drop()
#endif /* CONFIG_NET_STATISTICS_MLD */

#if defined(CONFIG_NET_STATISTICS_PERIODIC_OUTPUT)
/* A simple periodic statistic printer, used only in net core */
void net_print_statistics(void);
#else
#define net_print_statistics()
#endif

#endif /* __NET_STATS_H__ */
