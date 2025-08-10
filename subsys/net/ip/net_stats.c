/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_STATISTICS_PERIODIC_OUTPUT)
#define NET_LOG_LEVEL LOG_LEVEL_INF
#else
#define NET_LOG_LEVEL CONFIG_NET_STATISTICS_LOG_LEVEL
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_stats, NET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/prometheus/collector.h>
#include <zephyr/net/prometheus/counter.h>
#include <zephyr/net/prometheus/gauge.h>
#include <zephyr/net/prometheus/histogram.h>
#include <zephyr/net/prometheus/summary.h>

#include "net_stats.h"
#include "net_private.h"

/* Global network statistics.
 *
 * The variable needs to be global so that the GET_STAT() macro can access it
 * from net_shell.c
 */
struct net_stats net_stats = { 0 };

#if defined(CONFIG_NET_STATISTICS_PERIODIC_OUTPUT)

#define PRINT_STATISTICS_INTERVAL (30 * MSEC_PER_SEC)

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

static inline int64_t cmp_val(uint64_t val1, uint64_t val2)
{
	return (int64_t)(val1 - val2);
}

static inline void stats(struct net_if *iface)
{
	static uint64_t next_print;
	uint64_t curr = k_uptime_get();
	int64_t cmp = cmp_val(curr, next_print);
	int i;

	if (!next_print || (abs(cmp) > PRINT_STATISTICS_INTERVAL)) {
		if (iface) {
			NET_INFO("Interface %p [%d]", iface,
				 net_if_get_by_iface(iface));
		} else {
			NET_INFO("Global statistics:");
		}

#if defined(CONFIG_NET_STATISTICS_IPV6)
		NET_INFO("IPv6 recv      %u\tsent\t%u\tdrop\t%u\tforwarded\t%u",
			 GET_STAT(iface, ipv6.recv),
			 GET_STAT(iface, ipv6.sent),
			 GET_STAT(iface, ipv6.drop),
			 GET_STAT(iface, ipv6.forwarded));
#if defined(CONFIG_NET_STATISTICS_IPV6_ND)
		NET_INFO("IPv6 ND recv   %u\tsent\t%u\tdrop\t%u",
			 GET_STAT(iface, ipv6_nd.recv),
			 GET_STAT(iface, ipv6_nd.sent),
			 GET_STAT(iface, ipv6_nd.drop));
#endif /* CONFIG_NET_STATISTICS_IPV6_ND */
#if defined(CONFIG_NET_STATISTICS_IPV6_PMTU)
		NET_INFO("IPv6 PMTU recv %u\tsent\t%u\tdrop\t%u",
			 GET_STAT(iface, ipv6_pmtu.recv),
			 GET_STAT(iface, ipv6_pmtu.sent),
			 GET_STAT(iface, ipv6_pmtu.drop));
#endif /* CONFIG_NET_STATISTICS_IPV6_PMTU */
#if defined(CONFIG_NET_STATISTICS_MLD)
		NET_INFO("IPv6 MLD recv  %u\tsent\t%u\tdrop\t%u",
			 GET_STAT(iface, ipv6_mld.recv),
			 GET_STAT(iface, ipv6_mld.sent),
			 GET_STAT(iface, ipv6_mld.drop));
#endif /* CONFIG_NET_STATISTICS_MLD */
#endif /* CONFIG_NET_STATISTICS_IPV6 */

#if defined(CONFIG_NET_STATISTICS_IPV4)
		NET_INFO("IPv4 recv      %u\tsent\t%u\tdrop\t%u\tforwarded\t%u",
			 GET_STAT(iface, ipv4.recv),
			 GET_STAT(iface, ipv4.sent),
			 GET_STAT(iface, ipv4.drop),
			 GET_STAT(iface, ipv4.forwarded));
#endif /* CONFIG_NET_STATISTICS_IPV4 */

		NET_INFO("IP vhlerr      %u\thblener\t%u\tlblener\t%u",
			 GET_STAT(iface, ip_errors.vhlerr),
			 GET_STAT(iface, ip_errors.hblenerr),
			 GET_STAT(iface, ip_errors.lblenerr));
		NET_INFO("IP fragerr     %u\tchkerr\t%u\tprotoer\t%u",
			 GET_STAT(iface, ip_errors.fragerr),
			 GET_STAT(iface, ip_errors.chkerr),
			 GET_STAT(iface, ip_errors.protoerr));

#if defined(CONFIG_NET_STATISTICS_IPV4_PMTU)
		NET_INFO("IPv4 PMTU recv %u\tsent\t%u\tdrop\t%u",
			 GET_STAT(iface, ipv4_pmtu.recv),
			 GET_STAT(iface, ipv4_pmtu.sent),
			 GET_STAT(iface, ipv4_pmtu.drop));
#endif /* CONFIG_NET_STATISTICS_IPV4_PMTU */

		NET_INFO("ICMP recv      %u\tsent\t%u\tdrop\t%u",
			 GET_STAT(iface, icmp.recv),
			 GET_STAT(iface, icmp.sent),
			 GET_STAT(iface, icmp.drop));
		NET_INFO("ICMP typeer    %u\tchkerr\t%u",
			 GET_STAT(iface, icmp.typeerr),
			 GET_STAT(iface, icmp.chkerr));

#if defined(CONFIG_NET_STATISTICS_UDP)
		NET_INFO("UDP recv       %u\tsent\t%u\tdrop\t%u",
			 GET_STAT(iface, udp.recv),
			 GET_STAT(iface, udp.sent),
			 GET_STAT(iface, udp.drop));
		NET_INFO("UDP chkerr     %u",
			 GET_STAT(iface, udp.chkerr));
#endif

#if defined(CONFIG_NET_STATISTICS_TCP)
		NET_INFO("TCP bytes recv %llu\tsent\t%llu",
			 GET_STAT(iface, tcp.bytes.received),
			 GET_STAT(iface, tcp.bytes.sent));
		NET_INFO("TCP seg recv   %u\tsent\t%u\tdrop\t%u",
			 GET_STAT(iface, tcp.recv),
			 GET_STAT(iface, tcp.sent),
			 GET_STAT(iface, tcp.drop));
		NET_INFO("TCP seg resent %u\tchkerr\t%u\tackerr\t%u",
			 GET_STAT(iface, tcp.resent),
			 GET_STAT(iface, tcp.chkerr),
			 GET_STAT(iface, tcp.ackerr));
		NET_INFO("TCP seg rsterr %u\trst\t%u\tre-xmit\t%u",
			 GET_STAT(iface, tcp.rsterr),
			 GET_STAT(iface, tcp.rst),
			 GET_STAT(iface, tcp.rexmit));
		NET_INFO("TCP conn drop  %u\tconnrst\t%u",
			 GET_STAT(iface, tcp.conndrop),
			 GET_STAT(iface, tcp.connrst));
#endif

		NET_INFO("Bytes received %llu", GET_STAT(iface, bytes.received));
		NET_INFO("Bytes sent     %llu", GET_STAT(iface, bytes.sent));
		NET_INFO("Processing err %u",
			 GET_STAT(iface, processing_error));

#if NET_TC_COUNT > 1
#if NET_TC_TX_COUNT > 1
		NET_INFO("TX traffic class statistics:");
		NET_INFO("TC  Priority\tSent pkts\tbytes");

		for (i = 0; i < NET_TC_TX_COUNT; i++) {
			NET_INFO("[%d] %s (%u)\t%u\t\t%llu", i,
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
			NET_INFO("[%d] %s (%u)\t%u\t\t%llu", i,
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

#if defined(CONFIG_NET_STATISTICS_POWER_MANAGEMENT)
		NET_INFO("Power management statistics:");
		NET_INFO("Last suspend time: %u ms",
			 GET_STAT(iface, pm.last_suspend_time));
		NET_INFO("Got suspended %u times",
			 GET_STAT(iface, pm.suspend_count));
		NET_INFO("Average suspend time: %u ms",
			 (uint32_t)(GET_STAT(iface, pm.overall_suspend_time) /
				 GET_STAT(iface, pm.suspend_count)));
		NET_INFO("Total suspended time: %llu ms",
			 GET_STAT(iface, pm.overall_suspend_time));
#endif
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

static int net_stats_get(uint32_t mgmt_request, struct net_if *iface,
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
#if defined(CONFIG_NET_STATISTICS_IPV6_PMTU)
	case NET_REQUEST_STATS_CMD_GET_IPV6_PMTU:
		len_chk = sizeof(struct net_stats_ipv6_pmtu);
		src = GET_STAT_ADDR(iface, ipv6_pmtu);
		break;
#endif
#if defined(CONFIG_NET_STATISTICS_IPV4_PMTU)
	case NET_REQUEST_STATS_CMD_GET_IPV4_PMTU:
		len_chk = sizeof(struct net_stats_ipv4_pmtu);
		src = GET_STAT_ADDR(iface, ipv4_pmtu);
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
#if defined(CONFIG_NET_STATISTICS_POWER_MANAGEMENT)
	case NET_REQUEST_STATS_GET_PM:
		len_chk = sizeof(struct net_stats_pm);
		src = GET_STAT_ADDR(iface, pm);
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

#if defined(CONFIG_NET_STATISTICS_IPV6_PMTU)
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IPV6_PMTU,
				  net_stats_get);
#endif

#if defined(CONFIG_NET_STATISTICS_IPV4_PMTU)
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IPV4_PMTU,
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

#if defined(CONFIG_NET_STATISTICS_POWER_MANAGEMENT)
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_PM,
				  net_stats_get);
#endif

#endif /* CONFIG_NET_STATISTICS_USER_API */

void net_stats_reset(struct net_if *iface)
{
	if (iface) {
		net_if_stats_reset(iface);
		return;
	}

	net_if_stats_reset_all();
	memset(&net_stats, 0, sizeof(net_stats));
}

#if defined(CONFIG_NET_STATISTICS_VIA_PROMETHEUS)
static void register_prometheus_metrics(struct net_if *iface)
{
	int total_count = 0;

	/* Find the correct collector for this interface */
	STRUCT_SECTION_FOREACH(prometheus_collector, entry) {
		if (entry->user_data == (void *)iface) {
			iface->collector = entry;
			break;
		}
	}

	if (iface->collector == NULL) {
		NET_DBG("No collector found for interface %d",
			net_if_get_by_iface(iface));
		return;
	}

	STRUCT_SECTION_FOREACH(prometheus_counter, entry) {
		if (entry->base.collector != iface->collector) {
			continue;
		}

		prometheus_collector_register_metric(iface->collector,
						     &entry->base);
		total_count++;
	}

	STRUCT_SECTION_FOREACH(prometheus_gauge, entry) {
		if (entry->base.collector != iface->collector) {
			continue;
		}

		prometheus_collector_register_metric(iface->collector,
						     &entry->base);
		total_count++;
	}

	STRUCT_SECTION_FOREACH(prometheus_summary, entry) {
		if (entry->base.collector != iface->collector) {
			continue;
		}

		prometheus_collector_register_metric(iface->collector,
						     &entry->base);
		total_count++;
	}

	STRUCT_SECTION_FOREACH(prometheus_histogram, entry) {
		if (entry->base.collector != iface->collector) {
			continue;
		}

		prometheus_collector_register_metric(iface->collector,
						     &entry->base);
		total_count++;
	}

	NET_DBG("Registered %d metrics for interface %d", total_count,
		net_if_get_by_iface(iface));
}

/* Do not update metrics one by one as that would require searching
 * each individual metric from the collector. Instead, let the
 * Prometheus API scrape the data from net_stats stored in net_if when
 * needed.
 */
int net_stats_prometheus_scrape(struct prometheus_collector *collector,
				struct prometheus_metric *metric,
				void *user_data)
{
	struct net_if *iface = user_data;
	net_stats_t value;

	if (!iface) {
		return -EINVAL;
	}

	if (iface->collector != collector) {
		return -EINVAL;
	}

	/* Update the metrics */
	if (metric->type == PROMETHEUS_COUNTER) {
		struct prometheus_counter *counter =
			CONTAINER_OF(metric, struct prometheus_counter, base);

		if (counter->user_data == NULL) {
			return -EAGAIN;
		}

		value = *((net_stats_t *)counter->user_data);

		prometheus_counter_set(counter, (uint64_t)value);

	} else if (metric->type == PROMETHEUS_GAUGE) {
		struct prometheus_gauge *gauge =
			CONTAINER_OF(metric, struct prometheus_gauge, base);

		if (gauge->user_data == NULL) {
			return -EAGAIN;
		}

		value = *((net_stats_t *)gauge->user_data);

		prometheus_gauge_set(gauge, (double)value);

	} else if (metric->type == PROMETHEUS_HISTOGRAM) {
		struct prometheus_histogram *histogram =
			CONTAINER_OF(metric, struct prometheus_histogram, base);

		if (histogram->user_data == NULL) {
			return -EAGAIN;
		}

	} else if (metric->type == PROMETHEUS_SUMMARY) {
		struct prometheus_summary *summary =
			CONTAINER_OF(metric, struct prometheus_summary, base);

		if (summary->user_data == NULL) {
			return -EAGAIN;
		}

		if (IS_ENABLED(CONFIG_NET_PKT_TXTIME_STATS) &&
		    strstr(metric->name, "_tx_time_summary") == 0) {
			IF_ENABLED(CONFIG_NET_PKT_TXTIME_STATS,
				   (struct net_stats_tx_time *tx_time =
				    (struct net_stats_tx_time *)summary->user_data;

				    prometheus_summary_observe_set(
					    summary,
					    (double)tx_time->sum,
					    (unsigned long)tx_time->count)));
		} else if (IS_ENABLED(CONFIG_NET_PKT_RXTIME_STATS) &&
			   strstr(metric->name, "_rx_time_summary") == 0) {
			IF_ENABLED(CONFIG_NET_PKT_RXTIME_STATS,
				   (struct net_stats_rx_time *rx_time =
				    (struct net_stats_rx_time *)summary->user_data;

				    prometheus_summary_observe_set(
					    summary,
					    (double)rx_time->sum,
					    (unsigned long)rx_time->count)));
		}
	} else {
		NET_DBG("Unknown metric type %d", metric->type);
	}

	return 0;
}

void net_stats_prometheus_init(struct net_if *iface)
{
	register_prometheus_metrics(iface);
}
#endif /* CONFIG_NET_STATISTICS_VIA_PROMETHEUS */
