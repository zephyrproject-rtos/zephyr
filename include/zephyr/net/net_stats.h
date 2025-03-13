/** @file
 * @brief Network statistics
 *
 * Network statistics data. This should only be enabled when
 * debugging as it consumes memory.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_STATS_H_
#define ZEPHYR_INCLUDE_NET_NET_STATS_H_

#include <zephyr/types.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_mgmt.h>

#include <zephyr/net/prometheus/collector.h>
#include <zephyr/net/prometheus/counter.h>
#include <zephyr/net/prometheus/metric.h>
#include <zephyr/net/prometheus/gauge.h>
#include <zephyr/net/prometheus/histogram.h>
#include <zephyr/net/prometheus/summary.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network statistics library
 * @defgroup net_stats Network Statistics Library
 * @since 1.5
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

/**
 * @typedef net_stats_t
 * @brief Network statistics counter
 */
typedef uint32_t net_stats_t;

/**
 * @brief Number of bytes sent and received.
 */
struct net_stats_bytes {
	/** Number of bytes sent */
	net_stats_t sent;
	/** Number of bytes received */
	net_stats_t received;
};

/**
 * @brief Number of network packets sent and received.
 */
struct net_stats_pkts {
	/** Number of packets sent */
	net_stats_t tx;
	/** Number of packets received */
	net_stats_t rx;
};

/**
 * @brief IP layer statistics
 */
struct net_stats_ip {
	/** Number of received packets at the IP layer. */
	net_stats_t recv;

	/** Number of sent packets at the IP layer. */
	net_stats_t sent;

	/** Number of forwarded packets at the IP layer. */
	net_stats_t forwarded;

	/** Number of dropped packets at the IP layer. */
	net_stats_t drop;
};

/**
 * @brief IP layer error statistics
 */
struct net_stats_ip_errors {
	/** Number of packets dropped due to wrong IP version
	 * or header length.
	 */
	net_stats_t vhlerr;

	 /** Number of packets dropped due to wrong IP length, high byte. */
	net_stats_t hblenerr;

	/** Number of packets dropped due to wrong IP length, low byte. */
	net_stats_t lblenerr;

	/** Number of packets dropped because they were IP fragments. */
	net_stats_t fragerr;

	/** Number of packets dropped due to IP checksum errors. */
	net_stats_t chkerr;

	/** Number of packets dropped because they were neither ICMP,
	 * UDP nor TCP.
	 */
	net_stats_t protoerr;
};

/**
 * @brief ICMP statistics
 */
struct net_stats_icmp {
	/** Number of received ICMP packets. */
	net_stats_t recv;

	/** Number of sent ICMP packets. */
	net_stats_t sent;

	/** Number of dropped ICMP packets. */
	net_stats_t drop;

	/** Number of ICMP packets with a wrong type. */
	net_stats_t typeerr;

	/** Number of ICMP packets with a bad checksum. */
	net_stats_t chkerr;
};

/**
 * @brief TCP statistics
 */
struct net_stats_tcp {
	/** Amount of received and sent TCP application data. */
	struct net_stats_bytes bytes;

	/** Amount of retransmitted data. */
	net_stats_t resent;

	/** Number of dropped packets at the TCP layer. */
	net_stats_t drop;

	/** Number of received TCP segments. */
	net_stats_t recv;

	/** Number of sent TCP segments. */
	net_stats_t sent;

	/** Number of dropped TCP segments. */
	net_stats_t seg_drop;

	/** Number of TCP segments with a bad checksum. */
	net_stats_t chkerr;

	/** Number of received TCP segments with a bad ACK number. */
	net_stats_t ackerr;

	/** Number of received bad TCP RST (reset) segments. */
	net_stats_t rsterr;

	/** Number of received TCP RST (reset) segments. */
	net_stats_t rst;

	/** Number of retransmitted TCP segments. */
	net_stats_t rexmit;

	/** Number of dropped connection attempts because too few connections
	 * were available.
	 */
	net_stats_t conndrop;

	/** Number of connection attempts for closed ports, triggering a RST. */
	net_stats_t connrst;
};

/**
 * @brief UDP statistics
 */
struct net_stats_udp {
	/** Number of dropped UDP segments. */
	net_stats_t drop;

	/** Number of received UDP segments. */
	net_stats_t recv;

	/** Number of sent UDP segments. */
	net_stats_t sent;

	/** Number of UDP segments with a bad checksum. */
	net_stats_t chkerr;
};

/**
 * @brief IPv6 neighbor discovery statistics
 */
struct net_stats_ipv6_nd {
	/** Number of dropped IPv6 neighbor discovery packets. */
	net_stats_t drop;

	/** Number of received IPv6 neighbor discovery packets. */
	net_stats_t recv;

	/** Number of sent IPv6 neighbor discovery packets. */
	net_stats_t sent;
};

/**
 * @brief IPv6 Path MTU Discovery statistics
 */
struct net_stats_ipv6_pmtu {
	/** Number of dropped IPv6 PMTU packets. */
	net_stats_t drop;

	/** Number of received IPv6 PMTU packets. */
	net_stats_t recv;

	/** Number of sent IPv6 PMTU packets. */
	net_stats_t sent;
};

/**
 * @brief IPv4 Path MTU Discovery statistics
 */
struct net_stats_ipv4_pmtu {
	/** Number of dropped IPv4 PMTU packets. */
	net_stats_t drop;

	/** Number of received IPv4 PMTU packets. */
	net_stats_t recv;

	/** Number of sent IPv4 PMTU packets. */
	net_stats_t sent;
};

/**
 * @brief IPv6 multicast listener daemon statistics
 */
struct net_stats_ipv6_mld {
	/** Number of received IPv6 MLD queries. */
	net_stats_t recv;

	/** Number of sent IPv6 MLD reports. */
	net_stats_t sent;

	/** Number of dropped IPv6 MLD packets. */
	net_stats_t drop;
};

/**
 * @brief IPv4 IGMP daemon statistics
 */
struct net_stats_ipv4_igmp {
	/** Number of received IPv4 IGMP queries */
	net_stats_t recv;

	/** Number of sent IPv4 IGMP reports */
	net_stats_t sent;

	/** Number of dropped IPv4 IGMP packets */
	net_stats_t drop;
};

/**
 * @brief DNS statistics
 */
struct net_stats_dns {
	/** Number of received DNS queries */
	net_stats_t recv;

	/** Number of sent DNS responses */
	net_stats_t sent;

	/** Number of dropped DNS packets */
	net_stats_t drop;
};

/**
 * @brief Network packet transfer times for calculating average TX time
 */
struct net_stats_tx_time {
	/** Sum of network packet transfer times. */
	uint64_t sum;

	/** Number of network packets transferred. */
	net_stats_t count;
};

/**
 * @brief Network packet receive times for calculating average RX time
 */
struct net_stats_rx_time {
	/** Sum of network packet receive times. */
	uint64_t sum;

	/** Number of network packets received. */
	net_stats_t count;
};

/** @cond INTERNAL_HIDDEN */

#if NET_TC_TX_COUNT == 0
#define NET_TC_TX_STATS_COUNT 1
#else
#define NET_TC_TX_STATS_COUNT NET_TC_TX_COUNT
#endif

#if NET_TC_RX_COUNT == 0
#define NET_TC_RX_STATS_COUNT 1
#else
#define NET_TC_RX_STATS_COUNT NET_TC_RX_COUNT
#endif

/** @endcond */

/**
 * @brief Traffic class statistics
 */
struct net_stats_tc {
	/** TX statistics for each traffic class */
	struct {
		/** Helper for calculating average TX time statistics */
		struct net_stats_tx_time tx_time;
#if defined(CONFIG_NET_PKT_TXTIME_STATS_DETAIL)
		/** Detailed TX time statistics inside network stack */
		struct net_stats_tx_time
				tx_time_detail[NET_PKT_DETAIL_STATS_COUNT];
#endif
		/** Number of packets sent for this traffic class */
		net_stats_t pkts;
		/** Number of packets dropped for this traffic class */
		net_stats_t dropped;
		/** Number of bytes sent for this traffic class */
		net_stats_t bytes;
		/** Priority of this traffic class */
		uint8_t priority;
	} sent[NET_TC_TX_STATS_COUNT];

	/** RX statistics for each traffic class */
	struct {
		/** Helper for calculating average RX time statistics */
		struct net_stats_rx_time rx_time;
#if defined(CONFIG_NET_PKT_RXTIME_STATS_DETAIL)
		/** Detailed RX time statistics inside network stack */
		struct net_stats_rx_time
				rx_time_detail[NET_PKT_DETAIL_STATS_COUNT];
#endif
		/** Number of packets received for this traffic class */
		net_stats_t pkts;
		/** Number of packets dropped for this traffic class */
		net_stats_t dropped;
		/** Number of bytes received for this traffic class */
		net_stats_t bytes;
		/** Priority of this traffic class */
		uint8_t priority;
	} recv[NET_TC_RX_STATS_COUNT];
};


/**
 * @brief Power management statistics
 */
struct net_stats_pm {
	/** Total suspend time */
	uint64_t overall_suspend_time;
	/** How many times we were suspended */
	net_stats_t suspend_count;
	/** How long the last suspend took */
	uint32_t last_suspend_time;
	/** Network interface last suspend start time */
	uint32_t start_time;
};


/**
 * @brief All network statistics in one struct.
 */
struct net_stats {
	/** Count of malformed packets or packets we do not have handler for */
	net_stats_t processing_error;

	/**
	 * This calculates amount of data transferred through all the
	 * network interfaces.
	 */
	struct net_stats_bytes bytes;

	/** IP layer errors */
	struct net_stats_ip_errors ip_errors;

#if defined(CONFIG_NET_STATISTICS_IPV6)
	/** IPv6 statistics */
	struct net_stats_ip ipv6;
#endif

#if defined(CONFIG_NET_STATISTICS_IPV4)
	/** IPv4 statistics */
	struct net_stats_ip ipv4;
#endif

#if defined(CONFIG_NET_STATISTICS_ICMP)
	/** ICMP statistics */
	struct net_stats_icmp icmp;
#endif

#if defined(CONFIG_NET_STATISTICS_TCP)
	/** TCP statistics */
	struct net_stats_tcp tcp;
#endif

#if defined(CONFIG_NET_STATISTICS_UDP)
	/** UDP statistics */
	struct net_stats_udp udp;
#endif

#if defined(CONFIG_NET_STATISTICS_IPV6_ND)
	/** IPv6 neighbor discovery statistics */
	struct net_stats_ipv6_nd ipv6_nd;
#endif

#if defined(CONFIG_NET_STATISTICS_IPV6_PMTU)
	/** IPv6 Path MTU Discovery statistics */
	struct net_stats_ipv6_pmtu ipv6_pmtu;
#endif

#if defined(CONFIG_NET_STATISTICS_IPV4_PMTU)
	/** IPv4 Path MTU Discovery statistics */
	struct net_stats_ipv4_pmtu ipv4_pmtu;
#endif

#if defined(CONFIG_NET_STATISTICS_MLD)
	/** IPv6 MLD statistics */
	struct net_stats_ipv6_mld ipv6_mld;
#endif

#if defined(CONFIG_NET_STATISTICS_IGMP)
	/** IPv4 IGMP statistics */
	struct net_stats_ipv4_igmp ipv4_igmp;
#endif

#if defined(CONFIG_NET_STATISTICS_DNS)
	/** DNS statistics */
	struct net_stats_dns dns;
#endif

#if NET_TC_COUNT > 1
	/** Traffic class statistics */
	struct net_stats_tc tc;
#endif

#if defined(CONFIG_NET_PKT_TXTIME_STATS)
	/** Network packet TX time statistics */
	struct net_stats_tx_time tx_time;
#endif

#if defined(CONFIG_NET_PKT_RXTIME_STATS)
	/** Network packet RX time statistics */
	struct net_stats_rx_time rx_time;
#endif

#if defined(CONFIG_NET_PKT_TXTIME_STATS_DETAIL)
	/** Network packet TX time detail statistics */
	struct net_stats_tx_time tx_time_detail[NET_PKT_DETAIL_STATS_COUNT];
#endif
#if defined(CONFIG_NET_PKT_RXTIME_STATS_DETAIL)
	/** Network packet RX time detail statistics */
	struct net_stats_rx_time rx_time_detail[NET_PKT_DETAIL_STATS_COUNT];
#endif

#if defined(CONFIG_NET_STATISTICS_POWER_MANAGEMENT)
	/** Power management statistics */
	struct net_stats_pm pm;
#endif
};

/**
 * @brief Ethernet error statistics
 */
struct net_stats_eth_errors {
	/** Number of RX length errors */
	net_stats_t rx_length_errors;

	/** Number of RX overrun errors */
	net_stats_t rx_over_errors;

	/** Number of RX CRC errors */
	net_stats_t rx_crc_errors;

	/** Number of RX frame errors */
	net_stats_t rx_frame_errors;

	/** Number of RX net_pkt allocation errors */
	net_stats_t rx_no_buffer_count;

	/** Number of RX missed errors */
	net_stats_t rx_missed_errors;

	/** Number of RX long length errors */
	net_stats_t rx_long_length_errors;

	/** Number of RX short length errors */
	net_stats_t rx_short_length_errors;

	/** Number of RX buffer align errors */
	net_stats_t rx_align_errors;

	/** Number of RX DMA failed errors */
	net_stats_t rx_dma_failed;

	/** Number of RX net_buf allocation errors */
	net_stats_t rx_buf_alloc_failed;

	/** Number of TX aborted errors */
	net_stats_t tx_aborted_errors;

	/** Number of TX carrier errors */
	net_stats_t tx_carrier_errors;

	/** Number of TX FIFO errors */
	net_stats_t tx_fifo_errors;

	/** Number of TX heartbeat errors */
	net_stats_t tx_heartbeat_errors;

	/** Number of TX window errors */
	net_stats_t tx_window_errors;

	/** Number of TX DMA failed errors */
	net_stats_t tx_dma_failed;

	/** Number of uncorrected ECC errors */
	net_stats_t uncorr_ecc_errors;

	/** Number of corrected ECC errors */
	net_stats_t corr_ecc_errors;
};

/**
 * @brief Ethernet flow control statistics
 */
struct net_stats_eth_flow {
	/** Number of RX XON flow control */
	net_stats_t rx_flow_control_xon;

	/** Number of RX XOFF flow control */
	net_stats_t rx_flow_control_xoff;

	/** Number of TX XON flow control */
	net_stats_t tx_flow_control_xon;

	/** Number of TX XOFF flow control */
	net_stats_t tx_flow_control_xoff;
};

/**
 * @brief Ethernet checksum statistics
 */
struct net_stats_eth_csum {
	/** Number of good RX checksum offloading */
	net_stats_t rx_csum_offload_good;

	/** Number of failed RX checksum offloading */
	net_stats_t rx_csum_offload_errors;
};

/**
 * @brief Ethernet hardware timestamp statistics
 */
struct net_stats_eth_hw_timestamp {
	/** Number of RX hardware timestamp cleared */
	net_stats_t rx_hwtstamp_cleared;

	/** Number of RX hardware timestamp timeout */
	net_stats_t tx_hwtstamp_timeouts;

	/** Number of RX hardware timestamp skipped */
	net_stats_t tx_hwtstamp_skipped;
};

#ifdef CONFIG_NET_STATISTICS_ETHERNET_VENDOR
/**
 * @brief Ethernet vendor specific statistics
 */
struct net_stats_eth_vendor {
	const char * const key; /**< Key name of vendor statistics */
	uint32_t value;         /**< Value of the statistics key */
};
#endif

/**
 * @brief All Ethernet specific statistics
 */
struct net_stats_eth {
	/** Total number of bytes received and sent */
	struct net_stats_bytes bytes;

	/** Total number of packets received and sent */
	struct net_stats_pkts pkts;

	/** Total number of broadcast packets received and sent */
	struct net_stats_pkts broadcast;

	/** Total number of multicast packets received and sent */
	struct net_stats_pkts multicast;

	/** Total number of errors in RX and TX */
	struct net_stats_pkts errors;

	/** Total number of errors in RX and TX */
	struct net_stats_eth_errors error_details;

	/** Total number of flow control errors in RX and TX */
	struct net_stats_eth_flow flow_control;

	/** Total number of checksum errors in RX and TX */
	struct net_stats_eth_csum csum;

	/** Total number of hardware timestamp errors in RX and TX */
	struct net_stats_eth_hw_timestamp hw_timestamp;

	/** Total number of collisions */
	net_stats_t collisions;

	/** Total number of dropped TX packets */
	net_stats_t tx_dropped;

	/** Total number of TX timeout errors */
	net_stats_t tx_timeout_count;

	/** Total number of TX queue restarts */
	net_stats_t tx_restart_queue;

	/** Total number of RX unknown protocol packets */
	net_stats_t unknown_protocol;

#ifdef CONFIG_NET_STATISTICS_ETHERNET_VENDOR
	/** Array is terminated with an entry containing a NULL key */
	struct net_stats_eth_vendor *vendor;
#endif
};

/**
 * @brief All PPP specific statistics
 */
struct net_stats_ppp {
	/** Total number of bytes received and sent */
	struct net_stats_bytes bytes;

	/** Total number of packets received and sent */
	struct net_stats_pkts pkts;

	/** Number of received and dropped PPP frames. */
	net_stats_t drop;

	/** Number of received PPP frames with a bad checksum. */
	net_stats_t chkerr;
};

/**
 * @brief All Wi-Fi management statistics
 */
struct net_stats_sta_mgmt {
	/** Number of received beacons */
	net_stats_t beacons_rx;

	/** Number of missed beacons */
	net_stats_t beacons_miss;
};

/**
 * @brief All Wi-Fi specific statistics
 */
struct net_stats_wifi {
	/** Total number of beacon errors */
	struct net_stats_sta_mgmt sta_mgmt;

	/** Total number of bytes received and sent */
	struct net_stats_bytes bytes;

	/** Total number of packets received and sent */
	struct net_stats_pkts pkts;

	/** Total number of broadcast packets received and sent */
	struct net_stats_pkts broadcast;

	/** Total number of multicast packets received and sent */
	struct net_stats_pkts multicast;

	/** Total number of errors in RX and TX */
	struct net_stats_pkts errors;

	/** Total number of unicast packets received and sent */
	struct net_stats_pkts unicast;

	/** Total number of dropped packets at received and sent*/
	net_stats_t overrun_count;
};

#if defined(CONFIG_NET_STATISTICS_USER_API)
/* Management part definitions */

/** @cond INTERNAL_HIDDEN */

#define _NET_STATS_LAYER	NET_MGMT_LAYER_L3
#define _NET_STATS_CODE		0x101
#define _NET_STATS_BASE		(NET_MGMT_LAYER(_NET_STATS_LAYER) |	\
				 NET_MGMT_LAYER_CODE(_NET_STATS_CODE))

enum net_request_stats_cmd {
	NET_REQUEST_STATS_CMD_GET_ALL = 1,
	NET_REQUEST_STATS_CMD_GET_PROCESSING_ERROR,
	NET_REQUEST_STATS_CMD_GET_BYTES,
	NET_REQUEST_STATS_CMD_GET_IP_ERRORS,
	NET_REQUEST_STATS_CMD_GET_IPV4,
	NET_REQUEST_STATS_CMD_GET_IPV6,
	NET_REQUEST_STATS_CMD_GET_IPV6_ND,
	NET_REQUEST_STATS_CMD_GET_IPV6_PMTU,
	NET_REQUEST_STATS_CMD_GET_IPV4_PMTU,
	NET_REQUEST_STATS_CMD_GET_ICMP,
	NET_REQUEST_STATS_CMD_GET_UDP,
	NET_REQUEST_STATS_CMD_GET_TCP,
	NET_REQUEST_STATS_CMD_GET_ETHERNET,
	NET_REQUEST_STATS_CMD_GET_PPP,
	NET_REQUEST_STATS_CMD_GET_PM,
	NET_REQUEST_STATS_CMD_GET_WIFI,
	NET_REQUEST_STATS_CMD_RESET_WIFI,
};

/** @endcond */

/** Request all network statistics */
#define NET_REQUEST_STATS_GET_ALL				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_ALL)

/** Request all processing error statistics */
#define NET_REQUEST_STATS_GET_PROCESSING_ERROR				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_PROCESSING_ERROR)

/** Request number of received and sent bytes */
#define NET_REQUEST_STATS_GET_BYTES				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_BYTES)

/** Request IP error statistics */
#define NET_REQUEST_STATS_GET_IP_ERRORS				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_IP_ERRORS)

/** @cond INTERNAL_HIDDEN */

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_ALL);
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_PROCESSING_ERROR);
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_BYTES);
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IP_ERRORS);

/** @endcond */

#if defined(CONFIG_NET_STATISTICS_IPV4)
/** Request IPv4 statistics */
#define NET_REQUEST_STATS_GET_IPV4				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_IPV4)

/** @cond INTERNAL_HIDDEN */
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IPV4);
/** @endcond */
#endif /* CONFIG_NET_STATISTICS_IPV4 */

#if defined(CONFIG_NET_STATISTICS_IPV6)
/** Request IPv6 statistics */
#define NET_REQUEST_STATS_GET_IPV6				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_IPV6)

/** @cond INTERNAL_HIDDEN */
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IPV6);
/** @endcond */
#endif /* CONFIG_NET_STATISTICS_IPV6 */

#if defined(CONFIG_NET_STATISTICS_IPV6_ND)
/** Request IPv6 neighbor discovery statistics */
#define NET_REQUEST_STATS_GET_IPV6_ND				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_IPV6_ND)

/** @cond INTERNAL_HIDDEN */
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IPV6_ND);
/** @endcond */
#endif /* CONFIG_NET_STATISTICS_IPV6_ND */

#if defined(CONFIG_NET_STATISTICS_IPV6_PMTU)
/** Request IPv6 Path MTU Discovery statistics */
#define NET_REQUEST_STATS_GET_IPV6_PMTU				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_IPV6_PMTU)

/** @cond INTERNAL_HIDDEN */
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IPV6_PMTU);
/** @endcond */
#endif /* CONFIG_NET_STATISTICS_IPV6_PMTU */

#if defined(CONFIG_NET_STATISTICS_IPV4_PMTU)
/** Request IPv4 Path MTU Discovery statistics */
#define NET_REQUEST_STATS_GET_IPV4_PMTU				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_IPV4_PMTU)

/** @cond INTERNAL_HIDDEN */
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IPV4_PMTU);
/** @endcond */
#endif /* CONFIG_NET_STATISTICS_IPV4_PMTU */

#if defined(CONFIG_NET_STATISTICS_ICMP)
/** Request ICMPv4 and ICMPv6 statistics */
#define NET_REQUEST_STATS_GET_ICMP				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_ICMP)

/** @cond INTERNAL_HIDDEN */
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_ICMP);
/** @endcond */
#endif /* CONFIG_NET_STATISTICS_ICMP */

#if defined(CONFIG_NET_STATISTICS_UDP)
/** Request UDP statistics */
#define NET_REQUEST_STATS_GET_UDP				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_UDP)

/** @cond INTERNAL_HIDDEN */
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_UDP);
/** @endcond */
#endif /* CONFIG_NET_STATISTICS_UDP */

#if defined(CONFIG_NET_STATISTICS_TCP)
/** Request TCP statistics */
#define NET_REQUEST_STATS_GET_TCP				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_TCP)

/** @cond INTERNAL_HIDDEN */
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_TCP);
/** @endcond */
#endif /* CONFIG_NET_STATISTICS_TCP */

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
/** Request Ethernet statistics */
#define NET_REQUEST_STATS_GET_ETHERNET				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_ETHERNET)

/** @cond INTERNAL_HIDDEN */
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_ETHERNET);
/** @endcond */
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

#if defined(CONFIG_NET_STATISTICS_PPP)
/** Request PPP statistics */
#define NET_REQUEST_STATS_GET_PPP				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_PPP)

/** @cond INTERNAL_HIDDEN */
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_PPP);
/** @endcond */
#endif /* CONFIG_NET_STATISTICS_PPP */

#endif /* CONFIG_NET_STATISTICS_USER_API */

#if defined(CONFIG_NET_STATISTICS_POWER_MANAGEMENT)
/** Request network power management statistics */
#define NET_REQUEST_STATS_GET_PM				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_PM)

/** @cond INTERNAL_HIDDEN */
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_PM);
/** @endcond */
#endif /* CONFIG_NET_STATISTICS_POWER_MANAGEMENT */

#if defined(CONFIG_NET_STATISTICS_WIFI)
/** Request Wi-Fi statistics */
#define NET_REQUEST_STATS_GET_WIFI				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_WIFI)

/** @cond INTERNAL_HIDDEN */
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_WIFI);
/** @endcond */

/** Reset Wi-Fi statistics*/
#define NET_REQUEST_STATS_RESET_WIFI                              \
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_RESET_WIFI)

/** @cond INTERNAL_HIDDEN */
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_RESET_WIFI);
/** @endcond */
#endif /* CONFIG_NET_STATISTICS_WIFI */

#define NET_STATS_GET_METRIC_NAME(_name) _name
#define NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx) net_stats_##dev_id##_##sfx##_collector
#define NET_STATS_GET_VAR(dev_id, sfx, var) zephyr_net_##var
#define NET_STATS_GET_INSTANCE(dev_id, sfx, _not_used) STRINGIFY(_##dev_id##_##sfx)

/* The label value is set to be the network interface name. Note that we skip
 * the first character (_) when setting the label value. This can be changed
 * if there is a way to token paste the instance name without the prefix character.
 * Note also that the below macros have some parameters that are not used atm.
 */
#define NET_STATS_PROMETHEUS_COUNTER_DEFINE(_desc, _labelval, _not_used,	\
					    _collector, _name, _stat_var_ptr)	\
	static PROMETHEUS_COUNTER_DEFINE(					\
		NET_STATS_GET_METRIC_NAME(_name),				\
		_desc, ({ .key = "nic", .value = &_labelval[1] }),		\
		&(_collector), _stat_var_ptr)

#define NET_STATS_PROMETHEUS_GAUGE_DEFINE(_desc,  _labelval, _not_used,		\
					  _collector, _name, _stat_var_ptr)	\
	static PROMETHEUS_GAUGE_DEFINE(						\
		NET_STATS_GET_METRIC_NAME(_name),				\
		_desc, ({ .key = "nic", .value = &_labelval[1] }),		\
		&(_collector), _stat_var_ptr)

#define NET_STATS_PROMETHEUS_SUMMARY_DEFINE(_desc,  _labelval, _not_used,	\
					    _collector, _name, _stat_var_ptr)	\
	static PROMETHEUS_SUMMARY_DEFINE(					\
		NET_STATS_GET_METRIC_NAME(_name),				\
		_desc, ({ .key = "nic", .value = &_labelval[1] }),		\
		&(_collector), _stat_var_ptr)

#define NET_STATS_PROMETHEUS_HISTOGRAM_DEFINE(_desc, _labelval, _not_used,	\
					      _collector, _name, _stat_var_ptr)	\
	static PROMETHEUS_HISTOGRAM_DEFINE(					\
		NET_STATS_GET_METRIC_NAME(_name),				\
		_desc, ({ .key = "nic", .value = &_labelval[1] }),		\
		&(_collector), _stat_var_ptr)

/* IPv6 layer statistics */
#if defined(CONFIG_NET_STATISTICS_IPV6)
#define NET_STATS_PROMETHEUS_IPV6(iface, dev_id, sfx)			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv6 packets sent",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv6_sent),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv6_sent),		\
		&(iface)->stats.ipv6.sent);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv6 packets received",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv6_recv),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv6_recv),		\
		&(iface)->stats.ipv6.recv);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv6 packets dropped",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv6_drop),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv6_drop),		\
		&(iface)->stats.ipv6.drop);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv6 packets forwarded",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv6_forward),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv6_forwarded),		\
		&(iface)->stats.ipv6.forwarded)
#else
#define NET_STATS_PROMETHEUS_IPV6(iface, dev_id, sfx)
#endif

/* IPv4 layer statistics */
#if defined(CONFIG_NET_STATISTICS_IPV4)
#define NET_STATS_PROMETHEUS_IPV4(iface, dev_id, sfx)			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv4 packets sent",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv4_sent),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv4_sent),		\
		&(iface)->stats.ipv4.sent);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv4 packets received",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv4_recv),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv4_recv),		\
		&(iface)->stats.ipv4.recv);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv4 packets dropped",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv4_drop),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv4_drop),		\
		&(iface)->stats.ipv4.drop);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv4 packets forwarded",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv4_forwarded),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv4_forwarded),		\
		&(iface)->stats.ipv4.forwarded)
#else
#define NET_STATS_PROMETHEUS_IPV4(iface, dev_id, sfx)
#endif

/* ICMP layer statistics */
#if defined(CONFIG_NET_STATISTICS_ICMP)
#define NET_STATS_PROMETHEUS_ICMP(iface, dev_id, sfx)			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"ICMP packets sent",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, icmp_sent),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, icmp_sent),		\
		&(iface)->stats.icmp.sent);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"ICMP packets received",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, icmp_recv),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, icmp_recv),		\
		&(iface)->stats.icmp.recv);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"ICMP packets dropped",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, icmp_drop),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, icmp_drop),		\
		&(iface)->stats.icmp.drop);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"ICMP packets checksum error",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, icmp_chkerr),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, icmp_chkerr),		\
		&(iface)->stats.icmp.chkerr);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"ICMP packets type error",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, icmp_typeerr),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, icmp_typeerr),		\
		&(iface)->stats.icmp.typeerr)
#else
#define NET_STATS_PROMETHEUS_ICMP(iface, dev_id, sfx)
#endif

/* UDP layer statistics */
#if defined(CONFIG_NET_STATISTICS_UDP)
#define NET_STATS_PROMETHEUS_UDP(iface, dev_id, sfx)			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"UDP packets sent",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, udp_sent),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, udp_sent),		\
		&(iface)->stats.udp.sent);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"UDP packets received",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, udp_recv),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, udp_recv),		\
		&(iface)->stats.udp.recv);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"UDP packets dropped",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, udp_drop),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, udp_drop),		\
		&(iface)->stats.udp.drop);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"UDP packets checksum error",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, udp_chkerr),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, udp_chkerr),		\
		&(iface)->stats.udp.chkerr)
#else
#define NET_STATS_PROMETHEUS_UDP(iface, dev_id, sfx)
#endif

/* TCP layer statistics */
#if defined(CONFIG_NET_STATISTICS_TCP)
#define NET_STATS_PROMETHEUS_TCP(iface, dev_id, sfx)			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"TCP bytes sent",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, tcp_bytes_sent),	\
		"byte_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, tcp_bytes_sent),		\
		&(iface)->stats.tcp.bytes.sent);			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"TCP bytes received",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, tcp_bytes_recv),	\
		"byte_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, tcp_bytes_recv),		\
		&(iface)->stats.tcp.bytes.received);			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"TCP bytes resent",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, tcp_bytes_resent),	\
		"byte_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, tcp_bytes_resent),	\
		&(iface)->stats.tcp.resent);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"TCP packets sent",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, tcp_sent),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, tcp_sent),		\
		&(iface)->stats.tcp.sent);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"TCP packets received",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, tcp_recv),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, tcp_recv),		\
		&(iface)->stats.tcp.recv);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"TCP packets dropped",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, tcp_drop),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, tcp_drop),		\
		&(iface)->stats.tcp.drop);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"TCP packets checksum error",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, tcp_chkerr),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, tcp_chkerr),		\
		&(iface)->stats.tcp.chkerr);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"TCP packets ack error",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, tcp_ackerr),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, tcp_ackerr),		\
		&(iface)->stats.tcp.ackerr);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"TCP packets reset error",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, tcp_rsterr),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, tcp_rsterr),		\
		&(iface)->stats.tcp.rsterr);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"TCP packets retransmitted",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, tcp_rexmit),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, tcp_rexmit),		\
		&(iface)->stats.tcp.rexmit);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"TCP reset received",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, tcp_rst_recv),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, tcp_rst),		\
		&(iface)->stats.tcp.rst);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"TCP connection drop",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, tcp_conndrop),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, tcp_conndrop),		\
		&(iface)->stats.tcp.conndrop);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"TCP connection reset",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, tcp_connrst),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, tcp_connrst),		\
		&(iface)->stats.tcp.connrst)
#else
#define NET_STATS_PROMETHEUS_TCP(iface, dev_id, sfx)
#endif

/* IPv6 Neighbor Discovery statistics */
#if defined(CONFIG_NET_STATISTICS_IPV6_ND)
#define NET_STATS_PROMETHEUS_IPV6_ND(iface, dev_id, sfx)		\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv6 ND packets sent",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv6_nd_sent),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv6_nd_sent),		\
		&(iface)->stats.ipv6_nd.sent);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv6 ND packets received",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv6_nd_recv),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv6_nd_recv),		\
		&(iface)->stats.ipv6_nd.recv);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv6 ND packets dropped",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv6_nd_drop),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv6_nd_drop),		\
		&(iface)->stats.ipv6_nd.drop)
#else
#define NET_STATS_PROMETHEUS_IPV6_ND(iface, dev_id, sfx)
#endif

/* IPv6 Path MTU Discovery statistics */
#if defined(CONFIG_NET_STATISTICS_IPV6_PMTU)
#define NET_STATS_PROMETHEUS_IPV6_PMTU(iface, dev_id, sfx)		\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv6 PMTU packets sent",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv6_pmtu_sent),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv6_pmtu_sent),		\
		&(iface)->stats.ipv6_pmtu.sent);			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv6 PMTU packets received",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv6_pmtu_recv),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv6_pmtu_recv),		\
		&(iface)->stats.ipv6_pmtu.recv);			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv6 PMTU packets dropped",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv6_pmtu_drop),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv6_pmtu_drop),		\
		&(iface)->stats.ipv6_pmtu.drop)
#else
#define NET_STATS_PROMETHEUS_IPV6_PMTU(iface, dev_id, sfx)
#endif

/* IPv4 Path MTU Discovery statistics */
#if defined(CONFIG_NET_STATISTICS_IPV4_PMTU)
#define NET_STATS_PROMETHEUS_IPV4_PMTU(iface, dev_id, sfx)		\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv4 PMTU packets sent",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv4_pmtu_sent),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv4_pmtu_sent),		\
		&(iface)->stats.ipv4_pmtu.sent);			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv4 PMTU packets received",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv4_pmtu_recv),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv4_pmtu_recv),		\
		&(iface)->stats.ipv4_pmtu.recv);			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv4 PMTU packets dropped",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv4_pmtu_drop),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv4_pmtu_drop),		\
		&(iface)->stats.ipv4_pmtu.drop)
#else
#define NET_STATS_PROMETHEUS_IPV4_PMTU(iface, dev_id, sfx)
#endif

/* IPv6 Multicast Listener Discovery statistics */
#if defined(CONFIG_NET_STATISTICS_MLD)
#define NET_STATS_PROMETHEUS_MLD(iface, dev_id, sfx)			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv6 MLD packets sent",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv6_mld_sent),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv6_mld_sent),		\
		&(iface)->stats.ipv6_mld.sent);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv6 MLD packets received",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv6_mld_recv),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv6_mld_recv),		\
		&(iface)->stats.ipv6_mld.recv);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv6 MLD packets dropped",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv6_mld_drop),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv6_mld_drop),		\
		&(iface)->stats.ipv6_mld.drop)
#else
#define NET_STATS_PROMETHEUS_MLD(iface, dev_id, sfx)
#endif

/* IPv4 IGMP statistics */
#if defined(CONFIG_NET_STATISTICS_IGMP)
#define NET_STATS_PROMETHEUS_IGMP(iface, dev_id, sfx)			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv4 IGMP packets sent",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv4_igmp_sent),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv4_igmp_sent),		\
		&(iface)->stats.ipv4_igmp.sent);			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv4 IGMP packets received",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv4_igmp_recv),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv4_igmp_recv),		\
		&(iface)->stats.ipv4_igmp.recv);			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IPv4 IGMP packets dropped",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ipv4_igmp_drop),	\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ipv4_igmp_drop),		\
		&(iface)->stats.ipv4_igmp.drop)
#else
#define NET_STATS_PROMETHEUS_IGMP(iface, dev_id, sfx)
#endif

/* DNS statistics */
#if defined(CONFIG_NET_STATISTICS_DNS)
#define NET_STATS_PROMETHEUS_DNS(iface, dev_id, sfx)			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"DNS packets sent",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, dns_sent),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, dns_sent),		\
		&(iface)->stats.dns.sent);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"DNS packets received",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, dns_recv),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, dns_recv),		\
		&(iface)->stats.dns.recv);				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"DNS packets dropped",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, dns_drop),		\
		"packet_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, dns_drop),		\
		&(iface)->stats.dns.drop)
#else
#define NET_STATS_PROMETHEUS_DNS(iface, dev_id, sfx)
#endif

/* TX time statistics */
#if defined(CONFIG_NET_PKT_TXTIME_STATS)
#define NET_STATS_PROMETHEUS_TX_TIME(iface, dev_id, sfx)		\
	NET_STATS_PROMETHEUS_SUMMARY_DEFINE(				\
		"TX time in microseconds",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, tx_time),		\
		"time",							\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, tx_time),		\
		&(iface)->stats.tx_time)
#else
#define NET_STATS_PROMETHEUS_TX_TIME(iface, dev_id, sfx)
#endif

/* RX time statistics */
#if defined(CONFIG_NET_PKT_RXTIME_STATS)
#define NET_STATS_PROMETHEUS_RX_TIME(iface, dev_id, sfx)		\
	NET_STATS_PROMETHEUS_SUMMARY_DEFINE(				\
		"RX time in microseconds",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, rx_time),		\
		"time",							\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, rx_time),		\
		&(iface)->stats.rx_time)
#else
#define NET_STATS_PROMETHEUS_RX_TIME(iface, dev_id, sfx)
#endif

/* Per network interface statistics via Prometheus */
#define NET_STATS_PROMETHEUS(iface, dev_id, sfx)			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"Processing error",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, process_error),	\
		"error_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, processing_error),	\
		&(iface)->stats.processing_error);			\
	/* IP layer error statistics */					\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IP proto error",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ip_proto_error),	\
		"error_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ip_errors_protoerr),	\
		&(iface)->stats.ip_errors.protoerr);			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IP version/header len error",				\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ip_vhl_error),	\
		"error_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ip_errors_vhlerr),	\
		&(iface)->stats.ip_errors.vhlerr);			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IP header len error (high byte)",			\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ip_hblen_error),	\
		"error_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ip_errors_hblenerr),	\
		&(iface)->stats.ip_errors.hblenerr);			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IP header len error (low byte)",			\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ip_lblen_error),	\
		"error_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ip_errors_lblenerr),	\
		&(iface)->stats.ip_errors.lblenerr);			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IP fragment error",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ip_frag_error),	\
		"error_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ip_errors_fragerr),	\
		&(iface)->stats.ip_errors.fragerr);			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"IP checksum error",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, ip_chk_error),	\
		"error_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, ip_errors_chkerr),	\
		&(iface)->stats.ip_errors.chkerr);			\
	/* General network statistics */				\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"Bytes received",					\
		NET_STATS_GET_INSTANCE(dev_id, sfx, bytes_recv),	\
		"byte_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, bytes_recv),		\
		&(iface)->stats.bytes.received);			\
	NET_STATS_PROMETHEUS_COUNTER_DEFINE(				\
		"Bytes sent",						\
		NET_STATS_GET_INSTANCE(dev_id, sfx, bytes_sent),	\
		"byte_count",						\
		NET_STATS_GET_COLLECTOR_NAME(dev_id, sfx),		\
		NET_STATS_GET_VAR(dev_id, sfx, bytes_sent),		\
		&(iface)->stats.bytes.sent);				\
	NET_STATS_PROMETHEUS_IPV6(iface, dev_id, sfx);			\
	NET_STATS_PROMETHEUS_IPV4(iface, dev_id, sfx);			\
	NET_STATS_PROMETHEUS_ICMP(iface, dev_id, sfx);			\
	NET_STATS_PROMETHEUS_UDP(iface, dev_id, sfx);			\
	NET_STATS_PROMETHEUS_TCP(iface, dev_id, sfx);			\
	NET_STATS_PROMETHEUS_IPV6_ND(iface, dev_id, sfx);		\
	NET_STATS_PROMETHEUS_IPV6_PMTU(iface, dev_id, sfx);		\
	NET_STATS_PROMETHEUS_IPV4_PMTU(iface, dev_id, sfx);		\
	NET_STATS_PROMETHEUS_MLD(iface, dev_id, sfx);			\
	NET_STATS_PROMETHEUS_IGMP(iface, dev_id, sfx);			\
	NET_STATS_PROMETHEUS_DNS(iface, dev_id, sfx);			\
	NET_STATS_PROMETHEUS_TX_TIME(iface, dev_id, sfx);		\
	NET_STATS_PROMETHEUS_RX_TIME(iface, dev_id, sfx)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_STATS_H_ */
