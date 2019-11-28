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
#include <net/net_core.h>
#include <net/net_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network statistics library
 * @defgroup net_stats Network Statistics Library
 * @ingroup networking
 * @{
 */

/**
 * @typedef net_stats_t
 * @brief Network statistics counter
 */
typedef u32_t net_stats_t;

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

	/** Number of received TCP segments. */
	net_stats_t recv;

	/** Number of sent TCP segments. */
	net_stats_t sent;

	/** Number of dropped TCP segments. */
	net_stats_t drop;

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
	net_stats_t drop;
	net_stats_t recv;
	net_stats_t sent;
};

/**
 * @brief IPv6 multicast listener daemon statistics
 */
struct net_stats_ipv6_mld {
	/** Number of received IPv6 MLD queries */
	net_stats_t recv;

	/** Number of sent IPv6 MLD reports */
	net_stats_t sent;

	/** Number of dropped IPv6 MLD packets */
	net_stats_t drop;
};

/**
 * @brief Network packet transfer times for calculating average TX time
 */
struct net_stats_tx_time {
	u64_t sum;
	net_stats_t count;
};

/**
 * @brief Network packet receive times for calculating average RX time
 */
struct net_stats_rx_time {
	u64_t sum;
	net_stats_t count;
};

/**
 * @brief Traffic class statistics
 */
struct net_stats_tc {
	struct {
		struct net_stats_tx_time tx_time;
		net_stats_t pkts;
		net_stats_t bytes;
		u8_t priority;
	} sent[NET_TC_TX_COUNT];

	struct {
		struct net_stats_rx_time rx_time;
		net_stats_t pkts;
		net_stats_t bytes;
		u8_t priority;
	} recv[NET_TC_RX_COUNT];
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

#if defined(CONFIG_NET_STATISTICS_MLD)
	/** IPv6 MLD statistics */
	struct net_stats_ipv6_mld ipv6_mld;
#endif

#if NET_TC_COUNT > 1
	/** Traffic class statistics */
	struct net_stats_tc tc;
#endif

#if defined(CONFIG_NET_CONTEXT_TIMESTAMP) && \
	defined(CONFIG_NET_PKT_TXTIME_STATS)
#error \
"Cannot define both CONFIG_NET_CONTEXT_TIMESTAMP and CONFIG_NET_PKT_TXTIME_STATS"
#endif
#if defined(CONFIG_NET_CONTEXT_TIMESTAMP) || \
					defined(CONFIG_NET_PKT_TXTIME_STATS)
	/** Network packet TX time statistics */
	struct net_stats_tx_time tx_time;
#endif

#if defined(CONFIG_NET_PKT_RXTIME_STATS)
	/** Network packet RX time statistics */
	struct net_stats_rx_time rx_time;
#endif
};

/**
 * @brief Ethernet error statistics
 */
struct net_stats_eth_errors {
	net_stats_t rx_length_errors;
	net_stats_t rx_over_errors;
	net_stats_t rx_crc_errors;
	net_stats_t rx_frame_errors;
	net_stats_t rx_no_buffer_count;
	net_stats_t rx_missed_errors;
	net_stats_t rx_long_length_errors;
	net_stats_t rx_short_length_errors;
	net_stats_t rx_align_errors;
	net_stats_t rx_dma_failed;
	net_stats_t rx_buf_alloc_failed;

	net_stats_t tx_aborted_errors;
	net_stats_t tx_carrier_errors;
	net_stats_t tx_fifo_errors;
	net_stats_t tx_heartbeat_errors;
	net_stats_t tx_window_errors;
	net_stats_t tx_dma_failed;

	net_stats_t uncorr_ecc_errors;
	net_stats_t corr_ecc_errors;
};

/**
 * @brief Ethernet flow control statistics
 */
struct net_stats_eth_flow {
	net_stats_t rx_flow_control_xon;
	net_stats_t rx_flow_control_xoff;
	net_stats_t tx_flow_control_xon;
	net_stats_t tx_flow_control_xoff;
};

/**
 * @brief Ethernet checksum statistics
 */
struct net_stats_eth_csum {
	net_stats_t rx_csum_offload_good;
	net_stats_t rx_csum_offload_errors;
};

/**
 * @brief Ethernet hardware timestamp statistics
 */
struct net_stats_eth_hw_timestamp {
	net_stats_t rx_hwtstamp_cleared;
	net_stats_t tx_hwtstamp_timeouts;
	net_stats_t tx_hwtstamp_skipped;
};

#ifdef CONFIG_NET_STATISTICS_ETHERNET_VENDOR
/**
 * @brief Ethernet vendor specific statistics
 */
struct net_stats_eth_vendor {
	const char * const key;
	u32_t value;
};
#endif

/**
 * @brief All Ethernet specific statistics
 */
struct net_stats_eth {
	struct net_stats_bytes bytes;
	struct net_stats_pkts pkts;
	struct net_stats_pkts broadcast;
	struct net_stats_pkts multicast;
	struct net_stats_pkts errors;
	struct net_stats_eth_errors error_details;
	struct net_stats_eth_flow flow_control;
	struct net_stats_eth_csum csum;
	struct net_stats_eth_hw_timestamp hw_timestamp;
	net_stats_t collisions;
	net_stats_t tx_dropped;
	net_stats_t tx_timeout_count;
	net_stats_t tx_restart_queue;
#ifdef CONFIG_NET_STATISTICS_ETHERNET_VENDOR
	/** Array is terminated with an entry containing a NULL key */
	struct net_stats_eth_vendor *vendor;
#endif
};

/**
 * @brief All PPP specific statistics
 */
struct net_stats_ppp {
	struct net_stats_bytes bytes;
	struct net_stats_pkts pkts;

	/** Number of received and dropped PPP frames. */
	net_stats_t drop;

	/** Number of received PPP frames with a bad checksum. */
	net_stats_t chkerr;
};

#if defined(CONFIG_NET_STATISTICS_USER_API)
/* Management part definitions */

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
	NET_REQUEST_STATS_CMD_GET_ICMP,
	NET_REQUEST_STATS_CMD_GET_UDP,
	NET_REQUEST_STATS_CMD_GET_TCP,
	NET_REQUEST_STATS_CMD_GET_ETHERNET,
	NET_REQUEST_STATS_CMD_GET_PPP,
};

#define NET_REQUEST_STATS_GET_ALL				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_ALL)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_ALL);

#define NET_REQUEST_STATS_GET_PROCESSING_ERROR				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_PROCESSING_ERROR)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_PROCESSING_ERROR);

#define NET_REQUEST_STATS_GET_BYTES				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_BYTES)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_BYTES);

#define NET_REQUEST_STATS_GET_IP_ERRORS				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_IP_ERRORS)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IP_ERRORS);

#if defined(CONFIG_NET_STATISTICS_IPV4)
#define NET_REQUEST_STATS_GET_IPV4				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_IPV4)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IPV4);
#endif /* CONFIG_NET_STATISTICS_IPV4 */

#if defined(CONFIG_NET_STATISTICS_IPV6)
#define NET_REQUEST_STATS_GET_IPV6				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_IPV6)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IPV6);
#endif /* CONFIG_NET_STATISTICS_IPV6 */

#if defined(CONFIG_NET_STATISTICS_IPV6_ND)
#define NET_REQUEST_STATS_GET_IPV6_ND				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_IPV6_ND)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IPV6_ND);
#endif /* CONFIG_NET_STATISTICS_IPV6_ND */

#if defined(CONFIG_NET_STATISTICS_ICMP)
#define NET_REQUEST_STATS_GET_ICMP				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_ICMP)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_ICMP);
#endif /* CONFIG_NET_STATISTICS_ICMP */

#if defined(CONFIG_NET_STATISTICS_UDP)
#define NET_REQUEST_STATS_GET_UDP				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_UDP)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_UDP);
#endif /* CONFIG_NET_STATISTICS_UDP */

#if defined(CONFIG_NET_STATISTICS_TCP)
#define NET_REQUEST_STATS_GET_TCP				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_TCP)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_TCP);
#endif /* CONFIG_NET_STATISTICS_TCP */

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
#define NET_REQUEST_STATS_GET_ETHERNET				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_ETHERNET)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_ETHERNET);
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

#if defined(CONFIG_NET_STATISTICS_PPP)
#define NET_REQUEST_STATS_GET_PPP				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_PPP)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_PPP);
#endif /* CONFIG_NET_STATISTICS_PPP */

#endif /* CONFIG_NET_STATISTICS_USER_API */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_STATS_H_ */
