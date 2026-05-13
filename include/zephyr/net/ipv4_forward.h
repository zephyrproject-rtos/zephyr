/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_IPV4_FORWARD_H_
#define ZEPHYR_INCLUDE_NET_IPV4_FORWARD_H_

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

struct iptable_rule_params {
	int input_iface_idx;
	int output_iface_idx;
	uint8_t src[NET_IPV4_ADDR_SIZE];
	uint8_t src_mask[NET_IPV4_ADDR_SIZE];
	uint8_t dst[NET_IPV4_ADDR_SIZE];
	uint8_t dst_mask[NET_IPV4_ADDR_SIZE];
	uint8_t proto;
	uint8_t priority;
	/* unreplied timeout in seconds, 0 means default, -1 means forever */
	int unreply_timeout;
	/* replied timeout in seconds, 0 means default, -1 means forever */
	int reply_timeout;
};

#if defined(CONFIG_NET_IPV4_FORWARD)

enum {
	DIR_ORIGIN,
	DIR_REPLY,
	DIR_NUM
};

struct conn_tuple {
	uint8_t src[NET_IPV4_ADDR_SIZE];
	uint8_t dst[NET_IPV4_ADDR_SIZE];
	/* ICMP, TCP, UDP */
	uint8_t proto;
	union {
		struct {
			uint16_t src_port;
			uint16_t dst_port;
		} tcp;
		struct {
			uint16_t src_port;
			uint16_t dst_port;
		} udp;
		struct {
			uint16_t identifier;
		} icmp;
	};
};

struct ipforward_entry {
	struct conn_tuple tuple[DIR_NUM];
	/* conn state */
	int state;
	/* input and output interfaces */
	struct net_if *iface[DIR_NUM];
	/* timeout user configuration */
	int unreply_timeout;
	int reply_timeout;
	/* timeout management */
	int timeout;
	int64_t last_seen;
	int64_t next_timeout;
};

struct iptable_rule {
	sys_snode_t node;
	struct net_if *iface[DIR_NUM];
	uint8_t src[NET_IPV4_ADDR_SIZE];
	uint8_t src_mask[NET_IPV4_ADDR_SIZE];
	uint8_t dst[NET_IPV4_ADDR_SIZE];
	uint8_t dst_mask[NET_IPV4_ADDR_SIZE];
	uint8_t proto;
	/* user configurations */
	/* match priority 0 - 255, the highest one is matched first,
	 * for equal ones, the earliest added one is matched first
	 */
	uint8_t priority;
	/* unreplied timeout in seconds, 0 means default, -1 means forever */
	int unreply_timeout;
	/* replied timeout in seconds, 0 means default, -1 means forever */
	int reply_timeout;
	/* iptable rule pool management */
	uint8_t idx;
	uint8_t used;
};

struct ipforward_table {
	struct ipforward_entry table[CONFIG_NET_IPV4_CONN_TRACK_MAX_NUM];
	/* ipforward rules */
	sys_slist_t rules;
	/* timeout management */
	int64_t now;
};

/* iptables apis */
struct iptable_rule *iptable_rule_get(struct iptable_rule_params *param);
int iptable_rule_add(struct iptable_rule_params *param);
void iptable_rule_del(int idx);

/* ipforward apis */
void ipforward_init(void);
enum net_verdict ipforward_route(struct net_pkt *pkt,
				 struct net_ipv4_hdr *iphdr);
#else

static inline int iptable_rule_add(struct iptable_rule_params *param)
{
	return -1;
}

static inline void iptable_rule_del(int idx)
{
	/* Do nothing */
}

static inline void ipforward_init(void)
{
	/* Do nothing */
}

static inline enum net_verdict ipforward_route(struct net_pkt *pkt,
					       struct net_ipv4_hdr *iphdr)
{
	return NET_CONTINUE;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_IPV4_FORWARD_H_ */
