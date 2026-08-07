/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal IPv4 NAT data structures.
 *
 * Private header for NAT connection tracking and iptable rule internals.
 * Not exposed to applications.
 */

#ifndef SUBSYS_NET_IP_IPV4_NAT_INTERNAL_H_
#define SUBSYS_NET_IP_IPV4_NAT_INTERNAL_H_

#include <zephyr/net/ipv4_nat.h>
#include <zephyr/sys/slist.h>

/**
 * @brief Direction indices for a NAT connection tuple.
 *
 * Used for accessing tuple, interface, etc. by direction origin or reply.
 */
enum {
	/** Traffic in the original direction. */
	NET_NAT_DIR_ORIGIN,
	/** Reply direction of the connection. */
	NET_NAT_DIR_REPLY,
	/** Number of directions; must be last. */
	NET_NAT_DIR_NUM
};

/**
 * @brief A tuple uniquely identifying a connection.
 *
 * Stores source/destination IPv4 address/port/protocol for use with NAT.
 */
struct net_conn_tuple {
	/** Source IPv4 address. */
	uint8_t src[NET_IPV4_ADDR_SIZE];
	/** Destination IPv4 address. */
	uint8_t dst[NET_IPV4_ADDR_SIZE];
	/** Protocol number (e.g., IPPROTO_TCP, IPPROTO_UDP, IPPROTO_ICMP). */
	enum net_ip_protocol proto;
	union {
		struct {
			/** TCP source port. */
			uint16_t src_port;
			/** TCP destination port. */
			uint16_t dst_port;
		} tcp; /**< Used if proto is IPPROTO_TCP. */

		struct {
			/** UDP source port. */
			uint16_t src_port;
			/** UDP destination port. */
			uint16_t dst_port;
		} udp; /**< Used if proto is IPPROTO_UDP. */

		struct {
			/** ICMP echo identifier. */
			uint16_t identifier;
		} icmp; /**< Used if proto is IPPROTO_ICMP. */
	};
};

/**
 * @brief NAT table entry for a single connection.
 *
 * Manages state, timeout, interface, and connection tuples for both directions.
 */
struct net_nat4_entry {
	/** Connection tuple for each direction. */
	struct net_conn_tuple tuple[NET_NAT_DIR_NUM];
	/** Connection state (implementation-defined). */
	int state;
	/** Input/output network interfaces. */
	struct net_if *iface[NET_NAT_DIR_NUM];
	/** Unreplied connection timeout configuration (seconds). */
	int unreply_timeout;
	/** Replied connection timeout configuration (seconds). */
	int reply_timeout;
	/** Timeout end timepoint. */
	k_timepoint_t end;
};

/**
 * @brief Represents an iptable rule for IPv4 NAT or forwarding.
 *
 * Captures all configuration for rule matching, policy, and timeouts.
 */
struct net_iptable_rule {
	/** Node for including rule in a singly-linked list. */
	sys_snode_t node;
	/** Input/output network interfaces. */
	struct net_if *iface[NET_NAT_DIR_NUM];
	/** Source IPv4 address. */
	uint8_t src[NET_IPV4_ADDR_SIZE];
	/** Source IPv4 address mask. */
	uint8_t src_mask[NET_IPV4_ADDR_SIZE];
	/** Destination IPv4 address. */
	uint8_t dst[NET_IPV4_ADDR_SIZE];
	/** Destination IPv4 address mask. */
	uint8_t dst_mask[NET_IPV4_ADDR_SIZE];
	/** Protocol number to match. */
	enum net_ip_protocol proto;
	/**
	 * Rule match priority (0-255).
	 * Higher priority rules are matched first; for equal priority,
	 * rules added earlier are preferred.
	 */
	uint8_t priority;
	/** Unreplied timeout in seconds; 0: default, -1: never expires. */
	int unreply_timeout;
	/** Replied timeout in seconds; 0: default, -1: never expires. */
	int reply_timeout;
	/** Index in rule pool. */
	uint8_t idx;
	/** Rule usage flag. */
	uint8_t used;
};

/**
 * @brief NAT and iptable state management structure.
 *
 * Stores NAT connection entries and rule list.
 */
struct net_ip4_table {
	/** NAT entries. */
	struct net_nat4_entry table[CONFIG_NET_IPV4_CONN_TRACK_MAX_NUM];
	/** List of iptable rules. */
	sys_slist_t rules;
};

#endif /* SUBSYS_NET_IP_IPV4_NAT_INTERNAL_H_ */
