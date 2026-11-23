/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IPv4 NAT and iptable interface for Zephyr networking stack.
 *
 * @defgroup nat IPv4 Network Address Translation
 * @ingroup networking
 * @since 4.5
 * @version 0.1.0
 * @{
 *
 * This header defines the data structures and APIs used for
 * IPv4 NAT (Network Address Translation) and rule-based iptable
 * manipulation in the Zephyr networking subsystem.
 */

#ifndef ZEPHYR_INCLUDE_NET_IPV4_NAT_H_
#define ZEPHYR_INCLUDE_NET_IPV4_NAT_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <stdbool.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parameters to define an iptable rule.
 *
 * Used with table rule management APIs to specify matching criteria,
 * input/output interfaces, protocols, and timeout values.
 */
struct net_iptable_rule_params {
	/** Input interface index. */
	int input_iface_idx;
	/** Output interface index. */
	int output_iface_idx;
	/** Source IPv4 address. */
	uint8_t src[NET_IPV4_ADDR_SIZE];
	/** Source IPv4 mask. */
	uint8_t src_mask[NET_IPV4_ADDR_SIZE];
	/** Destination IPv4 address. */
	uint8_t dst[NET_IPV4_ADDR_SIZE];
	/** Destination IPv4 mask. */
	uint8_t dst_mask[NET_IPV4_ADDR_SIZE];
	/** Protocol number (e.g., IPPROTO_TCP, IPPROTO_UDP). */
	enum net_ip_protocol proto;
	/** Rule's match priority (higher value is higher priority). */
	uint8_t priority;
	/** Unreplied timeout in seconds; 0: use default, -1: never expires. */
	int unreply_timeout;
	/** Replied timeout in seconds; 0: use default, -1: never expires. */
	int reply_timeout;
};

/**
 * @brief Add a new iptable rule.
 *
 * @param param Rule configuration and match criteria.
 * @return Non-negative rule index on success, negative code on error.
 */
int net_ipv4_table_rule_add(struct net_iptable_rule_params *param);

/**
 * @brief Remove an iptable rule by index.
 *
 * @param idx Index of the rule to remove.
 */
void net_ipv4_table_rule_del(int idx);

/**
 * @brief Initialize IPv4 NAT subsystem.
 *
 * Prepares internal NAT state for operation.
 */
void net_ipv4_nat_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_IPV4_NAT_H_ */
