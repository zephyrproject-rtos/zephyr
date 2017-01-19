/** @file
 @brief ICMPv6 handler

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ICMPV6_H
#define __ICMPV6_H

#include <misc/slist.h>
#include <stdint.h>

#include <net/net_ip.h>
#include <net/nbuf.h>

struct net_icmpv6_ns_hdr {
	uint32_t reserved;
	struct in6_addr tgt;
} __packed;

struct net_icmpv6_nd_opt_hdr {
	uint8_t type;
	uint8_t len;
} __packed;

struct net_icmpv6_na_hdr {
	uint8_t flags;
	uint8_t reserved[3];
	struct in6_addr tgt;
} __packed;

struct net_icmpv6_rs_hdr {
	uint32_t reserved;
} __packed;

struct net_icmpv6_ra_hdr {
	uint8_t cur_hop_limit;
	uint8_t flags;
	uint16_t router_lifetime;
	uint32_t reachable_time;
	uint32_t retrans_timer;
} __packed;

struct net_icmpv6_nd_opt_mtu {
	uint8_t type;
	uint8_t len;
	uint16_t reserved;
	uint32_t mtu;
} __packed;

struct net_icmpv6_nd_opt_prefix_info {
	uint8_t type;
	uint8_t len;
	uint8_t prefix_len;
	uint8_t flags;
	uint32_t valid_lifetime;
	uint32_t preferred_lifetime;
	uint32_t reserved;
	struct in6_addr prefix;
} __packed;

struct net_icmpv6_nd_opt_6co {
	uint8_t type;
	uint8_t len;
	uint8_t context_len;
	uint8_t flag; /*res:3,c:1,cid:4 */
	uint16_t reserved;
	uint16_t lifetime;
	struct in6_addr prefix;
} __packed;

#define NET_ICMPV6_NS_BUF(buf)						\
	((struct net_icmpv6_ns_hdr *)(net_nbuf_icmp_data(buf) +		\
				      sizeof(struct net_icmp_hdr)))

#define NET_ICMPV6_ND_OPT_HDR_BUF(buf)					\
	((struct net_icmpv6_nd_opt_hdr *)(net_nbuf_icmp_data(buf) +	\
					  sizeof(struct net_icmp_hdr) +	\
					  net_nbuf_ext_opt_len(buf)))

#define NET_ICMPV6_NA_BUF(buf)						\
	((struct net_icmpv6_na_hdr *)(net_nbuf_icmp_data(buf) +		\
				      sizeof(struct net_icmp_hdr)))

#define NET_ICMPV6_RS_BUF(buf)						\
	((struct net_icmpv6_rs_hdr *)(net_nbuf_icmp_data(buf) +		\
				      sizeof(struct net_icmp_hdr)))

#define NET_ICMPV6_RA_BUF(buf)						\
	((struct net_icmpv6_ra_hdr *)(net_nbuf_icmp_data(buf) +		\
				      sizeof(struct net_icmp_hdr)))

#define NET_ICMPV6_ND_O_FLAG(flag) ((flag) & 0x40)
#define NET_ICMPV6_ND_M_FLAG(flag) ((flag) & 0x80)

#define NET_ICMPV6_ND_OPT_SLLAO       1
#define NET_ICMPV6_ND_OPT_TLLAO       2
#define NET_ICMPV6_ND_OPT_PREFIX_INFO 3
#define NET_ICMPV6_ND_OPT_MTU         5
#define NET_ICMPV6_ND_OPT_ROUTE       24
#define NET_ICMPV6_ND_OPT_RDNSS       25
#define NET_ICMPV6_ND_OPT_DNSSL       31
#define NET_ICMPV6_ND_OPT_6CO         34

#define NET_ICMPV6_OPT_TYPE_OFFSET   0
#define NET_ICMPV6_OPT_LEN_OFFSET    1
#define NET_ICMPV6_OPT_DATA_OFFSET   2

#define NET_ICMPV6_NA_FLAG_ROUTER     0x80
#define NET_ICMPV6_NA_FLAG_SOLICITED  0x40
#define NET_ICMPV6_NA_FLAG_OVERRIDE   0x20
#define NET_ICMPV6_RA_FLAG_ONLINK     0x80
#define NET_ICMPV6_RA_FLAG_AUTONOMOUS 0x40

#define NET_ICMPV6_DST_UNREACH    1	/* Destination unreachable */
#define NET_ICMPV6_PACKET_TOO_BIG 2	/* Packet too big */
#define NET_ICMPV6_TIME_EXCEEDED  3	/* Time exceeded */
#define NET_ICMPV6_PARAM_PROBLEM  4	/* IPv6 header is bad */
#define NET_ICMPV6_ECHO_REQUEST 128
#define NET_ICMPV6_ECHO_REPLY   129
#define NET_ICMPV6_RS           133	/* Router Solicitation */
#define NET_ICMPV6_RA           134	/* Router Advertisement */
#define NET_ICMPV6_NS           135	/* Neighbor Solicitation */
#define NET_ICMPV6_NA           136	/* Neighbor Advertisement */

/* Codes for ICMPv6 Destination Unreachable message */
#define NET_ICMPV6_DST_UNREACH_NO_ROUTE  0 /* No route to destination */
#define NET_ICMPV6_DST_UNREACH_ADMIN     1 /* Admin prohibited communication */
#define NET_ICMPV6_DST_UNREACH_SCOPE     2 /* Beoynd scope of source address */
#define NET_ICMPV6_DST_UNREACH_NO_ADDR   3 /* Address unrechable */
#define NET_ICMPV6_DST_UNREACH_NO_PORT   4 /* Port unreachable */
#define NET_ICMPV6_DST_UNREACH_SRC_ADDR  5 /* Source address failed */
#define NET_ICMPV6_DST_UNREACH_REJ_ROUTE 6 /* Reject route to destination */

/* Codes for ICMPv6 Parameter Problem message */
#define NET_ICMPV6_PARAM_PROB_HEADER     0 /* Erroneous header field */
#define NET_ICMPV6_PARAM_PROB_NEXTHEADER 1 /* Unrecognized next header */
#define NET_ICMPV6_PARAM_PROB_OPTION     2 /* Unrecognized option */

/* ICMPv6 header has 4 unused bytes that must be zero, RFC 4443 ch 3.1 */
#define NET_ICMPV6_UNUSED_LEN 4

typedef enum net_verdict (*icmpv6_callback_handler_t)(struct net_buf *buf);

struct net_icmpv6_handler {
	sys_snode_t node;
	uint8_t type;
	uint8_t code;
	icmpv6_callback_handler_t handler;
};

/**
 * @brief Send ICMPv6 error message.
 * @param buf Network buffer that this error is related to.
 * @param type Type of the error message.
 * @param code Code of the type of the error message.
 * @param param Optional parameter value for this error. Depending on type
 * and code this gives extra information to the recipient. Set 0 if unsure
 * what value to use.
 * @return Return 0 if the sending succeed, <0 otherwise.
 */
int net_icmpv6_send_error(struct net_buf *buf, uint8_t type, uint8_t code,
			  uint32_t param);

/**
 * @brief Send ICMPv6 echo request message.
 *
 * @param iface Network interface.
 * @param dst IPv6 address of the target host.
 * @param identifier An identifier to aid in matching Echo Replies
 * to this Echo Request. May be zero.
 * @param sequence A sequence number to aid in matching Echo Replies
 * to this Echo Request. May be zero.
 *
 * @return Return 0 if the sending succeed, <0 otherwise.
 */
int net_icmpv6_send_echo_request(struct net_if *iface,
				 struct in6_addr *dst,
				 uint16_t identifier,
				 uint16_t sequence);

void net_icmpv6_register_handler(struct net_icmpv6_handler *handler);
enum net_verdict net_icmpv6_input(struct net_buf *buf, uint16_t len,
				  uint8_t type, uint8_t code);
#if defined(CONFIG_NET_IPV6)
void net_icmpv6_init(void);
#else
#define net_icmpv6_init(...)
#endif

#endif /* __ICMPV6_H */
