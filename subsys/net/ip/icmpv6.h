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

#include <zephyr/sys/slist.h>
#include <zephyr/types.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>

struct net_icmpv6_ns_hdr {
	uint32_t reserved;
	uint8_t tgt[NET_IPV6_ADDR_SIZE];
} __packed;

struct net_icmpv6_nd_opt_hdr {
	uint8_t type;
	uint8_t len;
} __packed;

struct net_icmpv6_na_hdr {
	uint8_t flags;
	uint8_t reserved[3];
	uint8_t tgt[NET_IPV6_ADDR_SIZE];
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
	uint16_t reserved;
	uint32_t mtu;
} __packed;

struct net_icmpv6_nd_opt_prefix_info {
	uint8_t prefix_len;
	uint8_t flags;
	uint32_t valid_lifetime;
	uint32_t preferred_lifetime;
	uint32_t reserved;
	uint8_t prefix[NET_IPV6_ADDR_SIZE];
} __packed;

struct net_icmpv6_nd_opt_6co {
	uint8_t context_len;
	uint8_t flag; /*res:3,c:1,cid:4 */
	uint16_t reserved;
	uint16_t lifetime;
	uint8_t prefix[NET_IPV6_ADDR_SIZE];
} __packed;

/* RFC 4191, ch. 2.3 */
struct net_icmpv6_nd_opt_route_info {
	uint8_t prefix_len;
	struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		uint8_t reserved_2 :3;
		uint8_t prf        :2;
		uint8_t reserved_1 :3;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		uint8_t reserved_1 :3;
		uint8_t prf        :2;
		uint8_t reserved_2 :3;
#endif
	} flags;
	uint32_t route_lifetime;
	/* Variable-length prefix field follows, can be 0, 8 or 16 bytes
	 * depending on the option length.
	 */
} __packed;

struct net_icmpv6_echo_req {
	uint16_t identifier;
	uint16_t sequence;
} __packed;

struct net_icmpv6_mld_query {
	uint16_t max_response_code;
	uint16_t reserved;
	uint8_t mcast_address[NET_IPV6_ADDR_SIZE];
	uint16_t flagg; /*S, QRV & QQIC */
	uint16_t num_sources;
} __packed;

struct net_icmpv6_mld_mcast_record {
	uint8_t record_type;
	uint8_t aux_data_len;
	uint16_t num_sources;
	uint8_t mcast_address[NET_IPV6_ADDR_SIZE];
} __packed;


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
#define NET_ICMPV6_MLD_QUERY    130	/* Multicast Listener Query */
#define NET_ICMPV6_RS           133	/* Router Solicitation */
#define NET_ICMPV6_RA           134	/* Router Advertisement */
#define NET_ICMPV6_NS           135	/* Neighbor Solicitation */
#define NET_ICMPV6_NA           136	/* Neighbor Advertisement */
#define NET_ICMPV6_MLDv2        143	/* Multicast Listener Report v2 */

/* Codes for ICMPv6 Destination Unreachable message */
#define NET_ICMPV6_DST_UNREACH_NO_ROUTE  0 /* No route to destination */
#define NET_ICMPV6_DST_UNREACH_ADMIN     1 /* Admin prohibited communication */
#define NET_ICMPV6_DST_UNREACH_SCOPE     2 /* Beyond scope of source address */
#define NET_ICMPV6_DST_UNREACH_NO_ADDR   3 /* Address unreachable */
#define NET_ICMPV6_DST_UNREACH_NO_PORT   4 /* Port unreachable */
#define NET_ICMPV6_DST_UNREACH_SRC_ADDR  5 /* Source address failed */
#define NET_ICMPV6_DST_UNREACH_REJ_ROUTE 6 /* Reject route to destination */

/* Codes for ICMPv6 Parameter Problem message */
#define NET_ICMPV6_PARAM_PROB_HEADER     0 /* Erroneous header field */
#define NET_ICMPV6_PARAM_PROB_NEXTHEADER 1 /* Unrecognized next header */
#define NET_ICMPV6_PARAM_PROB_OPTION     2 /* Unrecognized option */

/* ICMPv6 header has 4 unused bytes that must be zero, RFC 4443 ch 3.1 */
#define NET_ICMPV6_UNUSED_LEN 4

typedef enum net_verdict (*icmpv6_callback_handler_t)(
						struct net_pkt *pkt,
						struct net_ipv6_hdr *ip_hdr,
						struct net_icmp_hdr *icmp_hdr);

const char *net_icmpv6_type2str(int icmpv6_type);

struct net_icmpv6_handler {
	sys_snode_t node;
	icmpv6_callback_handler_t handler;
	uint8_t type;
	uint8_t code;
};

/**
 * @brief Send ICMPv6 error message.
 * @param pkt Network packet that this error is related to.
 * @param type Type of the error message.
 * @param code Code of the type of the error message.
 * @param param Optional parameter value for this error. Depending on type
 * and code this gives extra information to the recipient. Set 0 if unsure
 * what value to use.
 * @return Return 0 if the sending succeed, <0 otherwise.
 */
int net_icmpv6_send_error(struct net_pkt *pkt, uint8_t type, uint8_t code,
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
 * @param data Arbitrary payload data that will be included in the
 * Echo Reply verbatim. May be zero.
 * @param data_size Size of the Payload Data in bytes. May be zero.
 *
 * @return Return 0 if the sending succeed, <0 otherwise.
 */
#if defined(CONFIG_NET_NATIVE_IPV6)
int net_icmpv6_send_echo_request(struct net_if *iface,
				 struct in6_addr *dst,
				 uint16_t identifier,
				 uint16_t sequence,
				 const void *data,
				 size_t data_size);
#else
static inline int net_icmpv6_send_echo_request(struct net_if *iface,
					       struct in6_addr *dst,
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

#if defined(CONFIG_NET_NATIVE_IPV6)
void net_icmpv6_register_handler(struct net_icmpv6_handler *handler);
void net_icmpv6_unregister_handler(struct net_icmpv6_handler *handler);
enum net_verdict net_icmpv6_input(struct net_pkt *pkt,
				  struct net_ipv6_hdr *ip_hdr);

int net_icmpv6_create(struct net_pkt *pkt, uint8_t icmp_type, uint8_t icmp_code);
int net_icmpv6_finalize(struct net_pkt *pkt);

void net_icmpv6_init(void);
#else
#define net_icmpv6_init(...)
#define net_icmpv6_register_handler(...)
#define net_icmpv6_unregister_handler(...)
#endif

#endif /* __ICMPV6_H */
