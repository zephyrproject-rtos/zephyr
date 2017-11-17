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

#ifndef __NET_STATS_H
#define __NET_STATS_H

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network statistics library
 * @defgroup net_stats Network Statistics Library
 * @ingroup networking
 * @{
 */

typedef u32_t net_stats_t;

struct net_stats_bytes {
	u32_t sent;
	u32_t received;
};

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

struct net_stats_tcp {
	/** Amount of received and sent TCP application data. */
	struct net_stats_bytes bytes;

	/** Amount of retransmitted data. */
	net_stats_t resent;

	/** Number of recived TCP segments. */
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

struct net_stats_udp {
	/** Number of dropped UDP segments. */
	net_stats_t drop;

	/** Number of recived UDP segments. */
	net_stats_t recv;

	/** Number of sent UDP segments. */
	net_stats_t sent;

	/** Number of UDP segments with a bad checksum. */
	net_stats_t chkerr;
};

struct net_stats_ipv6_nd {
	net_stats_t drop;
	net_stats_t recv;
	net_stats_t sent;
};

struct net_stats_rpl_dis {
	/** Number of received DIS packets. */
	net_stats_t recv;

	/** Number of sent DIS packets. */
	net_stats_t sent;

	/** Number of dropped DIS packets. */
	net_stats_t drop;
};

struct net_stats_rpl_dio {
	/** Number of received DIO packets. */
	net_stats_t recv;

	/** Number of sent DIO packets. */
	net_stats_t sent;

	/** Number of dropped DIO packets. */
	net_stats_t drop;

	/** Number of DIO intervals. */
	net_stats_t interval;
};

struct net_stats_rpl_dao {
	/** Number of received DAO packets. */
	net_stats_t recv;

	/** Number of sent DAO packets. */
	net_stats_t sent;

	/** Number of dropped DAO packets. */
	net_stats_t drop;

	/** Number of forwarded DAO packets. */
	net_stats_t forwarded;
};

struct net_stats_rpl_dao_ack {
	/** Number of received DAO-ACK packets. */
	net_stats_t recv;

	/** Number of sent DAO-ACK packets. */
	net_stats_t sent;

	/** Number of dropped DAO-ACK packets. */
	net_stats_t drop;
};

struct net_stats_rpl {
	u16_t mem_overflows;
	u16_t local_repairs;
	u16_t global_repairs;
	u16_t malformed_msgs;
	u16_t resets;
	u16_t parent_switch;
	u16_t forward_errors;
	u16_t loop_errors;
	u16_t loop_warnings;
	u16_t root_repairs;

	struct net_stats_rpl_dis dis;
	struct net_stats_rpl_dio dio;
	struct net_stats_rpl_dao dao;
	struct net_stats_rpl_dao_ack dao_ack;
};

struct net_stats_ipv6_mld {
	/** Number of received IPv6 MLD queries */
	net_stats_t recv;

	/** Number of sent IPv6 MLD reports */
	net_stats_t sent;

	/** Number of dropped IPv6 MLD packets */
	net_stats_t drop;
};

struct net_stats {
	net_stats_t processing_error;

	/*
	 * This calculates amount of data transferred through all the
	 * network interfaces.
	 */
	struct net_stats_bytes bytes;

	struct net_stats_ip_errors ip_errors;

#if defined(CONFIG_NET_STATISTICS_IPV6)
	struct net_stats_ip ipv6;
#endif

#if defined(CONFIG_NET_STATISTICS_IPV4)
	struct net_stats_ip ipv4;
#endif

#if defined(CONFIG_NET_STATISTICS_ICMP)
	struct net_stats_icmp icmp;
#endif

#if defined(CONFIG_NET_STATISTICS_TCP)
	struct net_stats_tcp tcp;
#endif

#if defined(CONFIG_NET_STATISTICS_UDP)
	struct net_stats_udp udp;
#endif

#if defined(CONFIG_NET_STATISTICS_IPV6_ND)
	struct net_stats_ipv6_nd ipv6_nd;
#endif

#if defined(CONFIG_NET_STATISTICS_RPL)
	struct net_stats_rpl rpl;
#endif

#if defined(CONFIG_NET_IPV6_MLD)
	struct net_stats_ipv6_mld ipv6_mld;
#endif
};

#if defined(CONFIG_NET_STATISTICS_USER_API)
/* Management part definitions */

#include <net/net_mgmt.h>

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
	NET_REQUEST_STATS_CMD_GET_RPL,
};

#define NET_REQUEST_STATS_GET_ALL				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_ALL)

//NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_ALL);

#define NET_REQUEST_STATS_GET_PROCESSING_ERROR				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_PROCESSING_ERROR)

//NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_PROCESSING_ERROR);

#define NET_REQUEST_STATS_GET_BYTES				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_BYTES)

//NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_BYTES);

#define NET_REQUEST_STATS_GET_IP_ERRORS				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_IP_ERRORS)

//NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_IP_ERRORS);

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

#if defined(CONFIG_NET_STATISTICS_RPL)
#define NET_REQUEST_STATS_GET_RPL				\
	(_NET_STATS_BASE | NET_REQUEST_STATS_CMD_GET_RPL)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_STATS_GET_RPL);
#endif /* CONFIG_NET_STATISTICS_RPL */

#endif /* CONFIG_NET_STATISTICS_USER_API */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __NET_STATS_H */
