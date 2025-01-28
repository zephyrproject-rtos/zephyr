/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <zephyr/net/net_stats.h>

#include "net_shell_private.h"

#include "../ip/net_stats.h"

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

	PR("Checksum offload : RX good %u errors %u\n",
	   data->csum.rx_csum_offload_good,
	   data->csum.rx_csum_offload_errors);
	PR("Flow control     : RX xon %u xoff %u TX xon %u xoff %u\n",
	   data->flow_control.rx_flow_control_xon,
	   data->flow_control.rx_flow_control_xoff,
	   data->flow_control.tx_flow_control_xon,
	   data->flow_control.tx_flow_control_xoff);
	PR("ECC errors       : uncorrected %u corrected %u\n",
	   data->error_details.uncorr_ecc_errors,
	   data->error_details.corr_ecc_errors);
	PR("HW timestamp     : RX cleared %u TX timeout %u skipped %u\n",
	   data->hw_timestamp.rx_hwtstamp_cleared,
	   data->hw_timestamp.tx_hwtstamp_timeouts,
	   data->hw_timestamp.tx_hwtstamp_skipped);

	PR("RX errors : %5s %5s %5s %5s %5s %5s %5s %5s %5s %5s %5s\n",
	   "Len", "Over", "CRC", "Frame", "NoBuf", "Miss", "Long", "Short",
	   "Align", "DMA", "Alloc");
	PR("            %5u %5u %5u %5u %5u %5u %5u %5u %5u %5u %5u\n",
	   data->error_details.rx_length_errors,
	   data->error_details.rx_over_errors,
	   data->error_details.rx_crc_errors,
	   data->error_details.rx_frame_errors,
	   data->error_details.rx_no_buffer_count,
	   data->error_details.rx_missed_errors,
	   data->error_details.rx_long_length_errors,
	   data->error_details.rx_short_length_errors,
	   data->error_details.rx_align_errors,
	   data->error_details.rx_dma_failed,
	   data->error_details.rx_buf_alloc_failed);
	PR("TX errors : %5s %8s %5s %10s %7s %5s\n",
	   "Abort", "Carrier", "Fifo", "Heartbeat", "Window", "DMA");
	PR("            %5u %8u %5u %10u %7u %5u\n",
	   data->error_details.tx_aborted_errors,
	   data->error_details.tx_carrier_errors,
	   data->error_details.tx_fifo_errors,
	   data->error_details.tx_heartbeat_errors,
	   data->error_details.tx_window_errors,
	   data->error_details.tx_dma_failed);

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

#else /* CONFIG_NET_PKT_TXTIME_STATS_DETAIL || CONFIG_NET_PKT_RXTIME_STATS_DETAIL */

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
#endif /* CONFIG_NET_PKT_TXTIME_STATS_DETAIL || CONFIG_NET_PKT_RXTIME_STATS_DETAIL */

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
		PR("Avg %s net_pkt (%u) time %" PRIu32 " us%s\n", "TX", count,
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
	PR("TC  Priority\tRecv pkts\tDrop pkts\tbytes\ttime\n");

	for (i = 0; i < NET_TC_RX_COUNT; i++) {
		net_stats_t count = GET_STAT(iface,
					     tc.recv[i].rx_time.count);
		if (count == 0) {
			PR("[%d] %s (%d)\t%d\t%d\t\t%d\t-\n", i,
			   priority2str(GET_STAT(iface, tc.recv[i].priority)),
			   GET_STAT(iface, tc.recv[i].priority),
			   GET_STAT(iface, tc.recv[i].pkts),
			   GET_STAT(iface, tc.recv[i].dropped),
			   GET_STAT(iface, tc.recv[i].bytes));
		} else {
			PR("[%d] %s (%d)\t%d\t%d\t\t%d\t%u us%s\n", i,
			   priority2str(GET_STAT(iface, tc.recv[i].priority)),
			   GET_STAT(iface, tc.recv[i].priority),
			   GET_STAT(iface, tc.recv[i].pkts),
			   GET_STAT(iface, tc.recv[i].dropped),
			   GET_STAT(iface, tc.recv[i].bytes),
			   (uint32_t)(GET_STAT(iface,
					    tc.recv[i].rx_time.sum) /
				   (uint64_t)count),
			   get_net_pkt_tc_stats_detail(iface, i, false));
		}
	}
#else
	PR("TC  Priority\tRecv pkts\tDrop pkts\tbytes\n");

	for (i = 0; i < NET_TC_RX_COUNT; i++) {
		PR("[%d] %s (%d)\t%d\t%d\t\t%d\n", i,
		   priority2str(GET_STAT(iface, tc.recv[i].priority)),
		   GET_STAT(iface, tc.recv[i].priority),
		   GET_STAT(iface, tc.recv[i].pkts),
		   GET_STAT(iface, tc.recv[i].dropped),
		   GET_STAT(iface, tc.recv[i].bytes));
	}
#endif /* CONFIG_NET_PKT_RXTIME_STATS */
#else
	ARG_UNUSED(sh);

#if defined(CONFIG_NET_PKT_RXTIME_STATS)
	net_stats_t count = GET_STAT(iface, rx_time.count);

	if (count != 0) {
		PR("Avg %s net_pkt (%u) time %" PRIu32 " us%s\n", "RX", count,
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
#if defined(CONFIG_NET_STATISTICS_IPV6_PMTU)
	PR("IPv6 PMTU recv %d\tsent\t%d\tdrop\t%d\n",
	   GET_STAT(iface, ipv6_pmtu.recv),
	   GET_STAT(iface, ipv6_pmtu.sent),
	   GET_STAT(iface, ipv6_pmtu.drop));
#endif /* CONFIG_NET_STATISTICS_IPV6_PMTU */
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

#if defined(CONFIG_NET_STATISTICS_IPV4_PMTU)
	PR("IPv4 PMTU recv %d\tsent\t%d\tdrop\t%d\n",
	   GET_STAT(iface, ipv4_pmtu.recv),
	   GET_STAT(iface, ipv4_pmtu.sent),
	   GET_STAT(iface, ipv4_pmtu.drop));
#endif /* CONFIG_NET_STATISTICS_IPV4_PMTU */

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
#if defined(CONFIG_NET_STATISTICS_DNS)
	PR("DNS recv       %d\tsent\t%d\tdrop\t%d\n",
	   GET_STAT(iface, dns.recv),
	   GET_STAT(iface, dns.sent),
	   GET_STAT(iface, dns.drop));
#endif /* CONFIG_NET_STATISTICS_DNS */

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

#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
static void net_shell_print_statistics_all(struct net_shell_user_data *data)
{
	net_if_foreach(net_shell_print_statistics, data);
}
#endif

int cmd_net_stats_all(const struct shell *sh, size_t argc, char *argv[])
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

int cmd_net_stats_iface(const struct shell *sh, size_t argc, char *argv[])
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

#if defined(CONFIG_NET_SHELL_DYN_CMD_COMPLETION)

#include "iface_dynamic.h"

#endif /* CONFIG_NET_SHELL_DYN_CMD_COMPLETION */

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_stats,
	SHELL_CMD(all, NULL,
		  "Show network statistics for all network interfaces.",
		  cmd_net_stats_all),
	SHELL_CMD(iface, IFACE_DYN_CMD,
		  "'net stats <index>' shows network statistics for "
		  "one specific network interface.",
		  cmd_net_stats_iface),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), stats, &net_cmd_stats,
		 "Show network statistics.",
		 cmd_net_stats, 1, 1);
