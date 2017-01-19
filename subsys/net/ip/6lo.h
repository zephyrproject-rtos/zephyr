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

#include <misc/slist.h>
#include <stdint.h>

#include <net/nbuf.h>
#include "icmpv6.h"

typedef bool (*fragment_handler_t)(struct net_buf *, int);

/**
 *  @brief Compress IPv6 packet as per RFC 6282
 *
 *  @details After this IPv6 packet and next header(if UDP), headers
 *  are compressed as per RFC 6282. After header compression data
 *  will be adjusted according to remaining space in fragments.
 *
 *  @param Pointer to network buffer
 *  @param iphc true for IPHC compression, false for IPv6 dispatch header
 *  @param Pointer to fragment function
 *
 *  @return True on success, false otherwise
 */
bool net_6lo_compress(struct net_buf *buf, bool iphc,
		      fragment_handler_t fragment);

/**
 *  @brief Unompress IPv6 packet as per RFC 6282
 *
 *  @details After this IPv6 packet and next header(if UDP), headers
 *  are uncompressed as per RFC 6282. After header uncompression data
 *  will be adjusted according to remaining space in fragments.
 *
 *  @param Pointer to network buffer
 *
 *  @return True on success, false otherwise
 */
bool net_6lo_uncompress(struct net_buf *buf);

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
#endif /* __NET_6LO_H */
