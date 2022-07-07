/** @file
 @brief IPv4 related functions

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IPV4_H
#define __IPV4_H

#include <zephyr/types.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_context.h>

#define NET_IPV4_IHL_MASK 0x0F

/* IPv4 Options */
#define NET_IPV4_OPTS_EO   0   /* End of Options */
#define NET_IPV4_OPTS_NOP  1   /* No operation */
#define NET_IPV4_OPTS_RR   7   /* Record Route */
#define NET_IPV4_OPTS_TS   68  /* Timestamp */
#define NET_IPV4_OPTS_RA   148 /* Router Alert */

/* IPv4 Options Timestamp flags */
#define NET_IPV4_TS_OPT_TS_ONLY	0 /* Timestamp only */
#define NET_IPV4_TS_OPT_TS_ADDR	1 /* Timestamp and address */
#define NET_IPV4_TS_OPT_TS_PRES	3 /* Timestamp prespecified hops*/

#define NET_IPV4_HDR_OPTNS_MAX_LEN 40

/* Fragment bits */
#define NET_IPV4_MF BIT(0) /* More fragments  */
#define NET_IPV4_DF BIT(1) /* Do not fragment */

#define NET_IPV4_IGMP_QUERY     0x11 /* Membership query     */
#define NET_IPV4_IGMP_REPORT_V1 0x12 /* v1 Membership report */
#define NET_IPV4_IGMP_REPORT_V2 0x16 /* v2 Membership report */
#define NET_IPV4_IGMP_LEAVE     0x17 /* v2 Leave group       */
#define NET_IPV4_IGMP_REPORT_V3 0x22 /* v3 Membership report */

struct net_ipv4_igmp_v2_query {
	uint8_t type;
	uint8_t max_rsp;
	uint16_t chksum;
	struct in_addr address;
} __packed;

struct net_ipv4_igmp_v2_report {
	uint8_t type;
	uint8_t max_rsp;
	uint16_t chksum;
	struct in_addr address;
} __packed;

/**
 * @brief Create IPv4 packet in provided net_pkt with option to set all the
 *        caller settable values.
 *
 * @param pkt Network packet
 * @param src Source IPv4 address
 * @param dst Destination IPv4 address
 * @param tos Type of service
 * @param id Fragment id
 * @param flags Fragmentation flags
 * @param offset Fragment offset
 * @param ttl Time-to-live value
 *
 * @return 0 on success, negative errno otherwise.
 */
#if defined(CONFIG_NET_NATIVE_IPV4)
int net_ipv4_create_full(struct net_pkt *pkt,
			 const struct in_addr *src,
			 const struct in_addr *dst,
			 uint8_t tos,
			 uint16_t id,
			 uint8_t flags,
			 uint16_t offset,
			 uint8_t ttl);
#else
static inline int net_ipv4_create_full(struct net_pkt *pkt,
				       const struct in_addr *src,
				       const struct in_addr *dst,
				       uint8_t tos,
				       uint16_t id,
				       uint8_t flags,
				       uint16_t offset,
				       uint8_t ttl)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(src);
	ARG_UNUSED(dst);
	ARG_UNUSED(tos);
	ARG_UNUSED(id);
	ARG_UNUSED(flags);
	ARG_UNUSED(offset);
	ARG_UNUSED(ttl);

	return -ENOTSUP;
}
#endif

/**
 * @brief Create IPv4 packet in provided net_pkt.
 *
 * @param pkt Network packet
 * @param src Source IPv4 address
 * @param dst Destination IPv4 address
 *
 * @return 0 on success, negative errno otherwise.
 */
#if defined(CONFIG_NET_NATIVE_IPV4)
int net_ipv4_create(struct net_pkt *pkt,
		    const struct in_addr *src,
		    const struct in_addr *dst);
#else
static inline int net_ipv4_create(struct net_pkt *pkt,
				  const struct in_addr *src,
				  const struct in_addr *dst)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(src);
	ARG_UNUSED(dst);

	return -ENOTSUP;
}
#endif

/**
 * @brief Finalize IPv4 packet. It should be called right before
 * sending the packet and after all the data has been added into
 * the packet. This function will set the length of the
 * packet and calculate the higher protocol checksum if needed.
 *
 * @param pkt Network packet
 * @param next_header_proto Protocol type of the next header after IPv4 header.
 *
 * @return 0 on success, negative errno otherwise.
 */
#if defined(CONFIG_NET_NATIVE_IPV4)
int net_ipv4_finalize(struct net_pkt *pkt, uint8_t next_header_proto);
#else
static inline int net_ipv4_finalize(struct net_pkt *pkt,
				    uint8_t next_header_proto)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(next_header_proto);

	return -ENOTSUP;
}
#endif

/**
 * @typedef net_ipv4_parse_hdr_options_cb_t
 * @brief IPv4 header options handle callback
 *
 * @details The callback is called when parser encounter
 * supported options.
 *
 * @param opt_type Option type
 * @param opt_data Option data
 * @param opt_len Option length
 * @param user_data Userdata given in net_ipv4_parse_hdr_options()
 *
 * @return 0 on success, negative otherwise.
 */
typedef int (*net_ipv4_parse_hdr_options_cb_t)(uint8_t opt_type,
					       uint8_t *opt_data,
					       uint8_t opt_len,
					       void *user_data);

/**
 * @brief Parse IPv4 header options.
 * Parse the IPv4 header options and call the callback with
 * options type, data and length along with user_data.
 *
 * @param pkt Network packet
 * @param cb callback to handle IPv4 header options
 * @param user_data User data
 *
 * @return 0 on success, negative otherwise.
 */
#if defined(CONFIG_NET_IPV4_HDR_OPTIONS)
int net_ipv4_parse_hdr_options(struct net_pkt *pkt,
			       net_ipv4_parse_hdr_options_cb_t cb,
			       void *user_data);
#else
static inline int net_ipv4_parse_hdr_options(struct net_pkt *pkt,
					     net_ipv4_parse_hdr_options_cb_t cb,
					     void *user_data)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
}
#endif

#endif /* __IPV4_H */
