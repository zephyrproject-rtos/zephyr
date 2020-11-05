/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_STATS_H__
#define __NET_STATS_H__

#if defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_NATIVE)

#include <stdlib.h>

#include <net/net_ip.h>
#include <net/net_stats.h>
#include <net/net_if.h>

extern struct net_stats net_stats;

#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
#define SET_STAT(cmd) (cmd)
#define GET_STAT(iface, s) (iface ? iface->stats.s : net_stats.s)
#define GET_STAT_ADDR(iface, s) (iface ? &iface->stats.s : &net_stats.s)
#else
#define SET_STAT(cmd)
#define GET_STAT(iface, s) (net_stats.s)
#define GET_STAT_ADDR(iface, s) (&GET_STAT(iface, s))
#endif

#define UPDATE_STAT_GLOBAL(cmd) (net_##cmd)
#define UPDATE_STAT(_iface, _cmd) \
	{ NET_ASSERT(_iface); (UPDATE_STAT_GLOBAL(_cmd)); \
	  SET_STAT(_iface->_cmd); }
/* Core stats */

static inline void net_stats_update_processing_error(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.processing_error++);
}

static inline void net_stats_update_ip_errors_protoerr(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.ip_errors.protoerr++);
}

static inline void net_stats_update_ip_errors_vhlerr(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.ip_errors.vhlerr++);
}

static inline void net_stats_update_bytes_recv(struct net_if *iface,
					       uint32_t bytes)
{
	UPDATE_STAT(iface, stats.bytes.received += bytes);
}

static inline void net_stats_update_bytes_sent(struct net_if *iface,
					       uint32_t bytes)
{
	UPDATE_STAT(iface, stats.bytes.sent += bytes);
}
#else
#define net_stats_update_processing_error(iface)
#define net_stats_update_ip_errors_protoerr(iface)
#define net_stats_update_ip_errors_vhlerr(iface)
#define net_stats_update_bytes_recv(iface, bytes)
#define net_stats_update_bytes_sent(iface, bytes)
#endif /* CONFIG_NET_STATISTICS */

#if defined(CONFIG_NET_STATISTICS_IPV6) && defined(CONFIG_NET_NATIVE_IPV6)
/* IPv6 stats */

static inline void net_stats_update_ipv6_sent(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.ipv6.sent++);
}

static inline void net_stats_update_ipv6_recv(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.ipv6.recv++);
}

static inline void net_stats_update_ipv6_drop(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.ipv6.drop++);
}
#else
#define net_stats_update_ipv6_drop(iface)
#define net_stats_update_ipv6_sent(iface)
#define net_stats_update_ipv6_recv(iface)
#endif /* CONFIG_NET_STATISTICS_IPV6 */

#if defined(CONFIG_NET_STATISTICS_IPV6_ND) && defined(CONFIG_NET_NATIVE_IPV6)
/* IPv6 Neighbor Discovery stats*/

static inline void net_stats_update_ipv6_nd_sent(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.ipv6_nd.sent++);
}

static inline void net_stats_update_ipv6_nd_recv(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.ipv6_nd.recv++);
}

static inline void net_stats_update_ipv6_nd_drop(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.ipv6_nd.drop++);
}
#else
#define net_stats_update_ipv6_nd_sent(iface)
#define net_stats_update_ipv6_nd_recv(iface)
#define net_stats_update_ipv6_nd_drop(iface)
#endif /* CONFIG_NET_STATISTICS_IPV6_ND */

#if defined(CONFIG_NET_STATISTICS_IPV4) && defined(CONFIG_NET_NATIVE_IPV4)
/* IPv4 stats */

static inline void net_stats_update_ipv4_drop(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.ipv4.drop++);
}

static inline void net_stats_update_ipv4_sent(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.ipv4.sent++);
}

static inline void net_stats_update_ipv4_recv(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.ipv4.recv++);
}
#else
#define net_stats_update_ipv4_drop(iface)
#define net_stats_update_ipv4_sent(iface)
#define net_stats_update_ipv4_recv(iface)
#endif /* CONFIG_NET_STATISTICS_IPV4 */

#if defined(CONFIG_NET_STATISTICS_ICMP) && defined(CONFIG_NET_NATIVE_IPV4)
/* Common ICMPv4/ICMPv6 stats */
static inline void net_stats_update_icmp_sent(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.icmp.sent++);
}

static inline void net_stats_update_icmp_recv(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.icmp.recv++);
}

static inline void net_stats_update_icmp_drop(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.icmp.drop++);
}
#else
#define net_stats_update_icmp_sent(iface)
#define net_stats_update_icmp_recv(iface)
#define net_stats_update_icmp_drop(iface)
#endif /* CONFIG_NET_STATISTICS_ICMP */

#if defined(CONFIG_NET_STATISTICS_UDP) && defined(CONFIG_NET_NATIVE_UDP)
/* UDP stats */
static inline void net_stats_update_udp_sent(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.udp.sent++);
}

static inline void net_stats_update_udp_recv(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.udp.recv++);
}

static inline void net_stats_update_udp_drop(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.udp.drop++);
}

static inline void net_stats_update_udp_chkerr(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.udp.chkerr++);
}
#else
#define net_stats_update_udp_sent(iface)
#define net_stats_update_udp_recv(iface)
#define net_stats_update_udp_drop(iface)
#define net_stats_update_udp_chkerr(iface)
#endif /* CONFIG_NET_STATISTICS_UDP */

#if defined(CONFIG_NET_STATISTICS_TCP) && defined(CONFIG_NET_NATIVE_TCP)
/* TCP stats */
static inline void net_stats_update_tcp_sent(struct net_if *iface, uint32_t bytes)
{
	UPDATE_STAT(iface, stats.tcp.bytes.sent += bytes);
}

static inline void net_stats_update_tcp_recv(struct net_if *iface, uint32_t bytes)
{
	UPDATE_STAT(iface, stats.tcp.bytes.received += bytes);
}

static inline void net_stats_update_tcp_resent(struct net_if *iface,
					       uint32_t bytes)
{
	UPDATE_STAT(iface, stats.tcp.resent += bytes);
}

static inline void net_stats_update_tcp_drop(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.tcp.drop++);
}

static inline void net_stats_update_tcp_seg_sent(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.tcp.sent++);
}

static inline void net_stats_update_tcp_seg_recv(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.tcp.recv++);
}

static inline void net_stats_update_tcp_seg_drop(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.tcp.seg_drop++);
}

static inline void net_stats_update_tcp_seg_rst(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.tcp.rst++);
}

static inline void net_stats_update_tcp_seg_conndrop(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.tcp.conndrop++);
}

static inline void net_stats_update_tcp_seg_connrst(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.tcp.connrst++);
}

static inline void net_stats_update_tcp_seg_chkerr(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.tcp.chkerr++);
}

static inline void net_stats_update_tcp_seg_ackerr(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.tcp.ackerr++);
}

static inline void net_stats_update_tcp_seg_rsterr(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.tcp.rsterr++);
}

static inline void net_stats_update_tcp_seg_rexmit(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.tcp.rexmit++);
}
#else
#define net_stats_update_tcp_sent(iface, bytes)
#define net_stats_update_tcp_resent(iface, bytes)
#define net_stats_update_tcp_recv(iface, bytes)
#define net_stats_update_tcp_drop(iface)
#define net_stats_update_tcp_seg_sent(iface)
#define net_stats_update_tcp_seg_recv(iface)
#define net_stats_update_tcp_seg_drop(iface)
#define net_stats_update_tcp_seg_rst(iface)
#define net_stats_update_tcp_seg_conndrop(iface)
#define net_stats_update_tcp_seg_connrst(iface)
#define net_stats_update_tcp_seg_chkerr(iface)
#define net_stats_update_tcp_seg_ackerr(iface)
#define net_stats_update_tcp_seg_rsterr(iface)
#define net_stats_update_tcp_seg_rexmit(iface)
#endif /* CONFIG_NET_STATISTICS_TCP */

static inline void net_stats_update_per_proto_recv(struct net_if *iface,
						   enum net_ip_protocol proto)
{
	if (!IS_ENABLED(CONFIG_NET_NATIVE)) {
		return;
	}

	if (IS_ENABLED(CONFIG_NET_UDP) && proto == IPPROTO_UDP) {
		net_stats_update_udp_recv(iface);
	} else if (IS_ENABLED(CONFIG_NET_TCP) && proto == IPPROTO_TCP) {
		net_stats_update_tcp_seg_recv(iface);
	}
}

static inline void net_stats_update_per_proto_drop(struct net_if *iface,
						   enum net_ip_protocol proto)
{
	if (!IS_ENABLED(CONFIG_NET_NATIVE)) {
		return;
	}

	if (IS_ENABLED(CONFIG_NET_UDP) && proto == IPPROTO_UDP) {
		net_stats_update_udp_drop(iface);
	} else if (IS_ENABLED(CONFIG_NET_TCP) && proto == IPPROTO_TCP) {
		net_stats_update_tcp_drop(iface);
	}
}

#if defined(CONFIG_NET_STATISTICS_MLD) && defined(CONFIG_NET_NATIVE)
static inline void net_stats_update_ipv6_mld_recv(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.ipv6_mld.recv++);
}

static inline void net_stats_update_ipv6_mld_sent(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.ipv6_mld.sent++);
}

static inline void net_stats_update_ipv6_mld_drop(struct net_if *iface)
{
	UPDATE_STAT(iface, stats.ipv6_mld.drop++);
}
#else
#define net_stats_update_ipv6_mld_recv(iface)
#define net_stats_update_ipv6_mld_sent(iface)
#define net_stats_update_ipv6_mld_drop(iface)
#endif /* CONFIG_NET_STATISTICS_MLD */

#if (defined(CONFIG_NET_CONTEXT_TIMESTAMP) || \
	defined(CONFIG_NET_PKT_TXTIME_STATS)) && defined(CONFIG_NET_STATISTICS)
static inline void net_stats_update_tx_time(struct net_if *iface,
					    uint32_t start_time,
					    uint32_t end_time)
{
	uint32_t diff = end_time - start_time;

	UPDATE_STAT(iface, stats.tx_time.sum +=
		    k_cyc_to_ns_floor64(diff) / 1000);
	UPDATE_STAT(iface, stats.tx_time.count += 1);
}
#else
#define net_stats_update_tx_time(iface, start_time, end_time)
#endif /* (TIMESTAMP || NET_PKT_TXTIME_STATS) && NET_STATISTICS */

#if defined(CONFIG_NET_PKT_TXTIME_STATS_DETAIL)
static inline void net_stats_update_tx_time_detail(struct net_if *iface,
						   uint32_t detail_stat[])
{
	int i;

	for (i = 0; i < NET_PKT_DETAIL_STATS_COUNT; i++) {
		UPDATE_STAT(iface,
			    stats.tx_time_detail[i].sum +=
			    k_cyc_to_ns_floor64(detail_stat[i]) / 1000);
		UPDATE_STAT(iface,
			    stats.tx_time_detail[i].count += 1);
	}
}
#else
#define net_stats_update_tx_time_detail(iface, detail_stat)
#endif /* NET_PKT_TXTIME_STATS_DETAIL */

#if defined(CONFIG_NET_PKT_RXTIME_STATS) && defined(CONFIG_NET_STATISTICS)
static inline void net_stats_update_rx_time(struct net_if *iface,
					    uint32_t start_time,
					    uint32_t end_time)
{
	uint32_t diff = end_time - start_time;

	UPDATE_STAT(iface, stats.rx_time.sum +=
		    k_cyc_to_ns_floor64(diff) / 1000);
	UPDATE_STAT(iface, stats.rx_time.count += 1);
}
#else
#define net_stats_update_rx_time(iface, start_time, end_time)
#endif /* NET_CONTEXT_TIMESTAMP && STATISTICS */

#if defined(CONFIG_NET_PKT_RXTIME_STATS_DETAIL)
static inline void net_stats_update_rx_time_detail(struct net_if *iface,
						   uint32_t detail_stat[])
{
	int i;

	for (i = 0; i < NET_PKT_DETAIL_STATS_COUNT; i++) {
		UPDATE_STAT(iface,
			    stats.rx_time_detail[i].sum +=
			    k_cyc_to_ns_floor64(detail_stat[i]) / 1000);
		UPDATE_STAT(iface,
			    stats.rx_time_detail[i].count += 1);
	}
}
#else
#define net_stats_update_rx_time_detail(iface, detail_stat)
#endif /* NET_PKT_RXTIME_STATS_DETAIL */

#if (NET_TC_COUNT > 1) && defined(CONFIG_NET_STATISTICS) \
	&& defined(CONFIG_NET_NATIVE)
static inline void net_stats_update_tc_sent_pkt(struct net_if *iface, uint8_t tc)
{
	UPDATE_STAT(iface, stats.tc.sent[tc].pkts++);
}

static inline void net_stats_update_tc_sent_bytes(struct net_if *iface,
						  uint8_t tc, size_t bytes)
{
	UPDATE_STAT(iface, stats.tc.sent[tc].bytes += bytes);
}

static inline void net_stats_update_tc_sent_priority(struct net_if *iface,
						     uint8_t tc, uint8_t priority)
{
	UPDATE_STAT(iface, stats.tc.sent[tc].priority = priority);
}

#if (defined(CONFIG_NET_CONTEXT_TIMESTAMP) || \
	defined(CONFIG_NET_PKT_TXTIME_STATS)) && \
	defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_NATIVE)
static inline void net_stats_update_tc_tx_time(struct net_if *iface,
					       uint8_t priority,
					       uint32_t start_time,
					       uint32_t end_time)
{
	uint32_t diff = end_time - start_time;
	int tc = net_tx_priority2tc(priority);

	UPDATE_STAT(iface, stats.tc.sent[tc].tx_time.sum +=
		    k_cyc_to_ns_floor64(diff) / 1000);
	UPDATE_STAT(iface, stats.tc.sent[tc].tx_time.count += 1);

	net_stats_update_tx_time(iface, start_time, end_time);
}
#else
#define net_stats_update_tc_tx_time(iface, tc, start_time, end_time)
#endif /* (NET_CONTEXT_TIMESTAMP || NET_PKT_TXTIME_STATS) && NET_STATISTICS */

#if defined(CONFIG_NET_PKT_TXTIME_STATS_DETAIL)
static inline void net_stats_update_tc_tx_time_detail(struct net_if *iface,
						      uint8_t priority,
						      uint32_t detail_stat[])
{
	int tc = net_tx_priority2tc(priority);
	int i;

	for (i = 0; i < NET_PKT_DETAIL_STATS_COUNT; i++) {
		UPDATE_STAT(iface,
			    stats.tc.sent[tc].tx_time_detail[i].sum +=
			    k_cyc_to_ns_floor64(detail_stat[i]) / 1000);
		UPDATE_STAT(iface,
			    stats.tc.sent[tc].tx_time_detail[i].count += 1);
	}

	net_stats_update_tx_time_detail(iface, detail_stat);
}
#else
#define net_stats_update_tc_tx_time_detail(iface, tc, detail_stat)
#endif /* CONFIG_NET_PKT_TXTIME_STATS_DETAIL */

#if defined(CONFIG_NET_PKT_RXTIME_STATS) && defined(CONFIG_NET_STATISTICS) \
	&& defined(CONFIG_NET_NATIVE)
static inline void net_stats_update_tc_rx_time(struct net_if *iface,
					       uint8_t priority,
					       uint32_t start_time,
					       uint32_t end_time)
{
	uint32_t diff = end_time - start_time;
	int tc = net_rx_priority2tc(priority);

	UPDATE_STAT(iface, stats.tc.recv[tc].rx_time.sum +=
		    k_cyc_to_ns_floor64(diff) / 1000);
	UPDATE_STAT(iface, stats.tc.recv[tc].rx_time.count += 1);

	net_stats_update_rx_time(iface, start_time, end_time);
}
#else
#define net_stats_update_tc_rx_time(iface, tc, start_time, end_time)
#endif /* NET_PKT_RXTIME_STATS && NET_STATISTICS */

#if defined(CONFIG_NET_PKT_RXTIME_STATS_DETAIL)
static inline void net_stats_update_tc_rx_time_detail(struct net_if *iface,
						      uint8_t priority,
						      uint32_t detail_stat[])
{
	int tc = net_rx_priority2tc(priority);
	int i;

	for (i = 0; i < NET_PKT_DETAIL_STATS_COUNT; i++) {
		UPDATE_STAT(iface,
			    stats.tc.recv[tc].rx_time_detail[i].sum +=
			    k_cyc_to_ns_floor64(detail_stat[i]) / 1000);
		UPDATE_STAT(iface,
			    stats.tc.recv[tc].rx_time_detail[i].count += 1);
	}

	net_stats_update_rx_time_detail(iface, detail_stat);
}
#else
#define net_stats_update_tc_rx_time_detail(iface, tc, detail_stat)
#endif /* CONFIG_NET_PKT_RXTIME_STATS_DETAIL */

static inline void net_stats_update_tc_recv_pkt(struct net_if *iface, uint8_t tc)
{
	UPDATE_STAT(iface, stats.tc.recv[tc].pkts++);
}

static inline void net_stats_update_tc_recv_bytes(struct net_if *iface,
						  uint8_t tc, size_t bytes)
{
	UPDATE_STAT(iface, stats.tc.recv[tc].bytes += bytes);
}

static inline void net_stats_update_tc_recv_priority(struct net_if *iface,
						     uint8_t tc, uint8_t priority)
{
	UPDATE_STAT(iface, stats.tc.recv[tc].priority = priority);
}
#else
#define net_stats_update_tc_sent_pkt(iface, tc)
#define net_stats_update_tc_sent_bytes(iface, tc, bytes)
#define net_stats_update_tc_sent_priority(iface, tc, priority)
#define net_stats_update_tc_recv_pkt(iface, tc)
#define net_stats_update_tc_recv_bytes(iface, tc, bytes)
#define net_stats_update_tc_recv_priority(iface, tc, priority)

#if (defined(CONFIG_NET_CONTEXT_TIMESTAMP) || \
	defined(CONFIG_NET_PKT_TXTIME_STATS)) && \
	defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_NATIVE)
static inline void net_stats_update_tc_tx_time(struct net_if *iface,
					       uint8_t pkt_priority,
					       uint32_t start_time,
					       uint32_t end_time)
{
	ARG_UNUSED(pkt_priority);

	net_stats_update_tx_time(iface, start_time, end_time);
}
#else
#define net_stats_update_tc_tx_time(iface, priority, start_time, end_time)
#endif /* (NET_CONTEXT_TIMESTAMP || NET_PKT_TXTIME_STATS) && NET_STATISTICS */

#if defined(CONFIG_NET_PKT_TXTIME_STATS_DETAIL)
static inline void net_stats_update_tc_tx_time_detail(struct net_if *iface,
						      uint8_t pkt_priority,
						      uint32_t detail_stat[])
{
	ARG_UNUSED(pkt_priority);

	net_stats_update_tx_time_detail(iface, detail_stat);
}
#else
#define net_stats_update_tc_tx_time_detail(iface, pkt_priority, detail_stat)
#endif /* CONFIG_NET_PKT_TXTIME_STATS_DETAIL */

#if defined(CONFIG_NET_PKT_RXTIME_STATS) && defined(CONFIG_NET_STATISTICS) \
	&& defined(CONFIG_NET_NATIVE)
static inline void net_stats_update_tc_rx_time(struct net_if *iface,
					       uint8_t pkt_priority,
					       uint32_t start_time,
					       uint32_t end_time)
{
	ARG_UNUSED(pkt_priority);

	net_stats_update_rx_time(iface, start_time, end_time);
}
#else
#define net_stats_update_tc_rx_time(iface, priority, start_time, end_time)
#endif /* NET_PKT_RXTIME_STATS && NET_STATISTICS */

#if defined(CONFIG_NET_PKT_RXTIME_STATS_DETAIL)
static inline void net_stats_update_tc_rx_time_detail(struct net_if *iface,
						      uint8_t pkt_priority,
						      uint32_t detail_stat[])
{
	ARG_UNUSED(pkt_priority);

	net_stats_update_rx_time_detail(iface, detail_stat);
}
#else
#define net_stats_update_tc_rx_time_detail(iface, pkt_priority, detail_stat)
#endif /* CONFIG_NET_PKT_RXTIME_STATS_DETAIL */
#endif /* NET_TC_COUNT > 1 */

#if defined(CONFIG_NET_STATISTICS_POWER_MANAGEMENT)	\
	&& defined(CONFIG_NET_STATISTICS) && defined(CONFIG_NET_NATIVE)
static inline void net_stats_add_suspend_start_time(struct net_if *iface,
						    uint32_t time)
{
	UPDATE_STAT(iface, stats.pm.start_time = time);
}

static inline void net_stats_add_suspend_end_time(struct net_if *iface,
						  uint32_t time)
{
	uint32_t diff_time =
		k_cyc_to_ms_floor32(time - GET_STAT(iface, pm.start_time));

	UPDATE_STAT(iface, stats.pm.start_time = 0);
	UPDATE_STAT(iface, stats.pm.last_suspend_time = diff_time);
	UPDATE_STAT(iface, stats.pm.suspend_count++);
	UPDATE_STAT(iface, stats.pm.overall_suspend_time += diff_time);
}
#else
#define net_stats_add_suspend_start_time(iface, time)
#define net_stats_add_suspend_end_time(iface, time)
#endif

#if defined(CONFIG_NET_STATISTICS_PERIODIC_OUTPUT) \
	&& defined(CONFIG_NET_NATIVE)
/* A simple periodic statistic printer, used only in net core */
void net_print_statistics_all(void);
void net_print_statistics_iface(struct net_if *iface);
void net_print_statistics(void);
#else
#define net_print_statistics_all()
#define net_print_statistics_iface(iface)
#define net_print_statistics()
#endif

void net_stats_reset(struct net_if *iface);
#endif /* __NET_STATS_H__ */
