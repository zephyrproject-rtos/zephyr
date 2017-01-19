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

#include <stdint.h>

#include <net/net_ip.h>
#include <net/nbuf.h>

#define NET_ICMPV4_DST_UNREACH  3	/* Destination unreachable */
#define NET_ICMPV4_ECHO_REQUEST 8
#define NET_ICMPV4_ECHO_REPLY   0

#define NET_ICMPV4_DST_UNREACH_NO_PROTO  2 /* Protocol not supported */
#define NET_ICMPV4_DST_UNREACH_NO_PORT   3 /* Port unreachable */

struct net_icmpv4_echo_req {
	uint16_t identifier;
	uint16_t sequence;
} __packed;

#define NET_ICMPV4_ECHO_REQ_BUF(buf)					\
	((struct net_icmpv4_echo_req *)(net_nbuf_icmp_data(buf) +	\
				      sizeof(struct net_icmp_hdr)))

/**
 * @brief Send ICMPv4 error message.
 * @param buf Network buffer that this error is related to.
 * @param type Type of the error message.
 * @param code Code of the type of the error message.
 * @return Return 0 if the sending succeed, <0 otherwise.
 */
int net_icmpv4_send_error(struct net_buf *buf, uint8_t type, uint8_t code);

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
				 uint16_t identifier,
				 uint16_t sequence);

enum net_verdict net_icmpv4_input(struct net_buf *buf, uint16_t len,
				  uint8_t type, uint8_t code);

#endif /* __ICMPV4_H */
