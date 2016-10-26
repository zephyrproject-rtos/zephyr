/** @file
 * @brief Network statistics
 *
 * Network statistics data. This should only be enabled when
 * debugging as it consumes memory.
 */

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

#ifndef __NET_STATS_H
#define __NET_STATS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NET_STATISTICS)
#define NET_STATS(s) s
extern struct net_stats net_stats;
#else
#define NET_STATS(s)
#endif

typedef uint32_t net_stats_t;

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
	/** Number of packets dropped due to wrong IP version or header length. */
	net_stats_t vhlerr;

	 /** Number of packets dropped due to wrong IP length, high byte. */
	net_stats_t hblenerr;

	/** Number of packets dropped due to wrong IP length, low byte. */
	net_stats_t lblenerr;

	/** Number of packets dropped because they were IP fragments. */
	net_stats_t fragerr;

	/** Number of packets dropped due to IP checksum errors. */
	net_stats_t chkerr;

	/** Number of packets dropped because they were neither ICMP, UDP nor TCP. */
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
	/** Number of recived TCP segments. */
	net_stats_t recv;

	/** Number of sent TCP segments. */
	net_stats_t sent;

	/** Number of dropped TCP segments. */
	net_stats_t drop;

	/** Number of TCP segments with a bad checksum. */
	net_stats_t chkerr;

	/** Number of TCP segments with a bad ACK number. */
	net_stats_t ackerr;

	/** Number of received TCP RST (reset) segments. */
	net_stats_t rst;

	/** Number of retransmitted TCP segments. */
	net_stats_t rexmit;

	/** Number of dropped SYNs because too few connections were available. */
	net_stats_t syndrop;

	/** Number of SYNs for closed ports, triggering a RST. */
	net_stats_t synrst;
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

struct net_stats {
	net_stats_t processing_error;

	struct net_stats_ip_errors ip_errors;

#if defined(CONFIG_NET_IPV6)
	struct net_stats_ip ipv6;
#define NET_STATS_IPV6(s) NET_STATS(s)
#else
#define NET_STATS_IPV6(s)
#endif

#if defined(CONFIG_NET_IPV4)
	struct net_stats_ip ipv4;
#define NET_STATS_IPV4(s) NET_STATS(s)
#else
#define NET_STATS_IPV4(s)
#endif

	struct net_stats_icmp icmp;

#if defined(CONFIG_NET_TCP)
	struct net_stats_tcp tcp;
#define NET_STATS_TCP(s) NET_STATS(s)
#else
#define NET_STATS_TCP(s)
#endif

#if defined (CONFIG_NET_UDP)
	struct net_stats_udp udp;
#define NET_STATS_UDP(s) NET_STATS(s)
#else
#define NET_STATS_UDP(s)
#endif

#if defined(CONFIG_NET_IPV6_ND)
	struct net_stats_ipv6_nd ipv6_nd;
#define NET_STATS_IPV6_ND(s) NET_STATS(s)
#else
#define NET_STATS_IPV6_ND(s)
#endif

#if defined(CONFIG_NET_RPL_STATS)
	struct {
		uint16_t mem_overflows;
		uint16_t local_repairs;
		uint16_t global_repairs;
		uint16_t malformed_msgs;
		uint16_t resets;
		uint16_t parent_switch;
		uint16_t forward_errors;
		uint16_t loop_errors;
		uint16_t loop_warnings;
		uint16_t root_repairs;

		struct net_stats_rpl_dis dis;
		struct net_stats_rpl_dio dio;
		struct net_stats_rpl_dao dao;
		struct net_stats_rpl_dao_ack dao_ack;
	} rpl;
#define NET_STATS_RPL(s) NET_STATS(s)
#define NET_STATS_RPL_DIS(s) NET_STATS(s)
#else
#define NET_STATS_RPL(s)
#define NET_STATS_RPL_DIS(s)
#endif /* CONFIG_NET_RPL_STATS */
};

#ifdef __cplusplus
}
#endif

#endif /* __NET_STATS_H */
