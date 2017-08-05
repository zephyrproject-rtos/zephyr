/** @file
 @brief ICMPv4 handler

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ICMPV4_H
#define __ICMPV4_H

#include <zephyr/types.h>

#include <net/net_ip.h>
#include <net/net_pkt.h>

#define NET_ICMPV4_DST_UNREACH  3	/* Destination unreachable */
#define NET_ICMPV4_ECHO_REQUEST 8
#define NET_ICMPV4_ECHO_REPLY   0

#define NET_ICMPV4_DST_UNREACH_NO_PROTO  2 /* Protocol not supported */
#define NET_ICMPV4_DST_UNREACH_NO_PORT   3 /* Port unreachable */

struct net_icmpv4_echo_req {
	u16_t identifier;
	u16_t sequence;
} __packed;

#define NET_ICMPV4_ECHO_REQ(pkt)					\
	((struct net_icmpv4_echo_req *)((u8_t *)net_pkt_icmp_data(pkt) + \
					sizeof(struct net_icmp_hdr)))

typedef enum net_verdict (*icmpv4_callback_handler_t)(struct net_pkt *pkt);

struct net_icmpv4_handler {
	sys_snode_t node;
	u8_t type;
	u8_t code;
	icmpv4_callback_handler_t handler;
};

/**
 * @brief Send ICMPv4 error message.
 * @param pkt Network packet that this error is related to.
 * @param type Type of the error message.
 * @param code Code of the type of the error message.
 * @return Return 0 if the sending succeed, <0 otherwise.
 */
int net_icmpv4_send_error(struct net_pkt *pkt, u8_t type, u8_t code);

/**
 * @brief Send ICMPv4 echo request message.
 *
 * @param iface Network interface.
 * @param dst IPv4 address of the target host.
 * @param identifier An identifier to aid in matching Echo Replies
 * to this Echo Request. May be zero.
 * @param sequence A sequence number to aid in matching Echo Replies
 * to this Echo Request. May be zero.
 *
 * @return Return 0 if the sending succeed, <0 otherwise.
 */
int net_icmpv4_send_echo_request(struct net_if *iface,
				 struct in_addr *dst,
				 u16_t identifier,
				 u16_t sequence);

void net_icmpv4_register_handler(struct net_icmpv4_handler *handler);

void net_icmpv4_unregister_handler(struct net_icmpv4_handler *handler);

enum net_verdict net_icmpv4_input(struct net_pkt *pkt,
				  u8_t type, u8_t code);

struct net_icmp_hdr *net_icmpv4_get_hdr(struct net_pkt *pkt,
					struct net_icmp_hdr *hdr);
struct net_icmp_hdr *net_icmpv4_set_hdr(struct net_pkt *pkt,
					struct net_icmp_hdr *hdr);
struct net_buf *net_icmpv4_set_chksum(struct net_pkt *pkt,
				      struct net_buf *frag);

#if defined(CONFIG_NET_IPV4)
void net_icmpv4_init(void);
#else
#define net_icmpv4_init(...)
#endif

#endif /* __ICMPV4_H */
