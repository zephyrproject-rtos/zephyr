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
#define NET_ICMPV4_TIME_EXCEEDED 11	/* Time exceeded */
#define NET_ICMPV4_BAD_IP_HEADER 12	/* Bad IP header */

#define NET_ICMPV4_DST_UNREACH_NO_PROTO  2 /* Protocol not supported */
#define NET_ICMPV4_DST_UNREACH_NO_PORT   3 /* Port unreachable */
#define NET_ICMPV4_TIME_EXCEEDED_FRAGMENT_REASSEMBLY_TIME 1 /* Fragment reassembly time exceeded */
#define NET_ICMPV4_BAD_IP_HEADER_LENGTH  2 /* Bad length field */

#define NET_ICMPV4_UNUSED_LEN 4

struct net_icmpv4_echo_req {
	uint16_t identifier;
	uint16_t sequence;
} __packed;

/**
 * @brief Send ICMPv4 error message.
 * @param pkt Network packet that this error is related to.
 * @param type Type of the error message.
 * @param code Code of the type of the error message.
 * @return Return 0 if the sending succeed, <0 otherwise.
 */
int net_icmpv4_send_error(struct net_pkt *pkt, uint8_t type, uint8_t code);

#if defined(CONFIG_NET_NATIVE_IPV4)
enum net_verdict net_icmpv4_input(struct net_pkt *pkt,
				  struct net_ipv4_hdr *ip_hdr);

int net_icmpv4_create(struct net_pkt *pkt, uint8_t icmp_type, uint8_t icmp_code);
int net_icmpv4_finalize(struct net_pkt *pkt);

void net_icmpv4_init(void);
#else
#define net_icmpv4_init(...)
#endif

#endif /* __ICMPV4_H */
