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

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>

#define NET_ICMPV4_DST_UNREACH  3	/* Destination unreachable */
#define NET_ICMPV4_ECHO_REQUEST 8
#define NET_ICMPV4_ECHO_REPLY   0

#define NET_ICMPV4_DST_UNREACH_NO_PROTO  2 /* Protocol not supported */
#define NET_ICMPV4_DST_UNREACH_NO_PORT   3 /* Port unreachable */

#define NET_ICMPV4_UNUSED_LEN 4

struct net_icmpv4_echo_req {
	uint16_t identifier;
	uint16_t sequence;
} __packed;

typedef enum net_verdict (*icmpv4_callback_handler_t)(
					struct net_pkt *pkt,
					struct net_ipv4_hdr *ip_hdr,
					struct net_icmp_hdr *icmp_hdr);

struct net_icmpv4_handler {
	sys_snode_t node;
	icmpv4_callback_handler_t handler;
	uint8_t type;
	uint8_t code;
};

/**
 * @brief Send ICMPv4 error message.
 * @param pkt Network packet that this error is related to.
 * @param type Type of the error message.
 * @param code Code of the type of the error message.
 * @return Return 0 if the sending succeed, <0 otherwise.
 */
int net_icmpv4_send_error(struct net_pkt *pkt, uint8_t type, uint8_t code);

/**
 * @brief Send ICMPv4 echo request message.
 *
 * @param iface Network interface.
 * @param dst IPv4 address of the target host.
 * @param identifier An identifier to aid in matching Echo Replies
 * to this Echo Request. May be zero.
 * @param sequence A sequence number to aid in matching Echo Replies
 * to this Echo Request. May be zero.
 * @param data Arbitrary payload data that will be included in the
 * Echo Reply verbatim. May be zero.
 * @param data_size Size of the Payload Data in bytes. May be zero.
 *
 * @return Return 0 if the sending succeed, <0 otherwise.
 */
#if defined(CONFIG_NET_NATIVE_IPV4)
int net_icmpv4_send_echo_request(struct net_if *iface,
				 struct in_addr *dst,
				 uint16_t identifier,
				 uint16_t sequence,
				 const void *data,
				 size_t data_size);
#else
static inline int net_icmpv4_send_echo_request(struct net_if *iface,
					       struct in_addr *dst,
					       uint16_t identifier,
					       uint16_t sequence,
					       const void *data,
					       size_t data_size)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(dst);
	ARG_UNUSED(identifier);
	ARG_UNUSED(sequence);
	ARG_UNUSED(data);
	ARG_UNUSED(data_size);

	return -ENOTSUP;
}
#endif

#if defined(CONFIG_NET_NATIVE_IPV4)
void net_icmpv4_register_handler(struct net_icmpv4_handler *handler);

void net_icmpv4_unregister_handler(struct net_icmpv4_handler *handler);

enum net_verdict net_icmpv4_input(struct net_pkt *pkt,
				  struct net_ipv4_hdr *ip_hdr);

int net_icmpv4_finalize(struct net_pkt *pkt);

void net_icmpv4_init(void);
#else
#define net_icmpv4_init(...)
#define net_icmpv4_register_handler(...)
#define net_icmpv4_unregister_handler(...)
#endif

#endif /* __ICMPV4_H */
