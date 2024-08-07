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

#if defined(CONFIG_NET_STATISTICS_MLD)
	/** IPv6 MLD statistics */
	struct net_stats_ipv6_mld ipv6_mld;
#endif

#if defined(CONFIG_NET_STATISTICS_IGMP)
	/** IPv4 IGMP statistics */
	struct net_stats_ipv4_igmp ipv4_igmp;
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

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_STATS_H_ */
