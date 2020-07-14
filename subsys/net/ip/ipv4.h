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

#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_context.h>

#define NET_IPV4_IHL_MASK 0x0F

/* IPv4 Options */
#define NET_IPV4_OPTS_EO   0   /* End of Options */
#define NET_IPV4_OPTS_NOP  1   /* No operation */
#define NET_IPV4_OPTS_RR   7   /* Record Route */
#define NET_IPV4_OPTS_TS   68  /* Timestamp */

/* IPv4 Options Timestamp flags */
#define NET_IPV4_TS_OPT_TS_ONLY	0 /* Timestamp only */
#define NET_IPV4_TS_OPT_TS_ADDR	1 /* Timestamp and address */
#define NET_IPV4_TS_OPT_TS_PRES	3 /* Timestamp prespecified hops*/

#define NET_IPV4_HDR_OPTNS_MAX_LEN 40

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
