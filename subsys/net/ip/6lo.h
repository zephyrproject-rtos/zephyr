/** @file
 @brief 6lowpan handler

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_6LO_H
#define __NET_6LO_H

#include <zephyr/sys/slist.h>
#include <zephyr/types.h>

#include <zephyr/net/net_pkt.h>
#include "icmpv6.h"

/**
 *  @brief Compress IPv6 packet as per RFC 6282
 *
 *  @details After this IPv6 packet and next header(if UDP), headers
 *  are compressed as per RFC 6282. After header compression data
 *  will be adjusted according to remaining space in fragments.
 *
 *  @param Pointer to network packet
 *  @param iphc true for IPHC compression, false for IPv6 dispatch header
 *
 *  @return header size difference on success (>= 0), negative errno otherwise
 */
#if defined(CONFIG_NET_6LO)
int net_6lo_compress(struct net_pkt *pkt, bool iphc);
#else
static inline int net_6lo_compress(struct net_pkt *pkt, bool iphc)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(iphc);

	return 0;
}
#endif

/**
 *  @brief Uncompress IPv6 packet as per RFC 6282
 *
 *  @details After this IPv6 packet and next header(if UDP), headers
 *  are uncompressed as per RFC 6282. After header uncompression data
 *  will be adjusted according to remaining space in fragments.
 *
 *  @param Pointer to network packet
 *
 *  @return True on success, false otherwise
 */
#if defined(CONFIG_NET_6LO)
bool net_6lo_uncompress(struct net_pkt *pkt);
#else
static inline bool net_6lo_uncompress(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return true;
}
#endif

/**
 *  @brief Set 6lowpan context information
 *
 *  @details Set 6lowpan context information. This context information
 *  will be used in IPv6 header compression and uncompression.
 *
 *  @return True on success, false otherwise
 */
#if defined(CONFIG_NET_6LO_CONTEXT)
void net_6lo_set_context(struct net_if *iface,
			 struct net_icmpv6_nd_opt_6co *context);
#endif

/**
 *  @brief Return the header size difference after uncompression
 *
 * @details This will do a dry-run on uncompressing the headers.
 *          The point is only to calculate the difference.
 *
 * @param Pointer to network packet
 *
 * @return header difference or INT_MAX in case of error.
 */
#if defined(CONFIG_NET_6LO)
int net_6lo_uncompress_hdr_diff(struct net_pkt *pkt);
#endif

#endif /* __NET_6LO_H */
