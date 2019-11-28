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
int net_ipv4_finalize(struct net_pkt *pkt, u8_t next_header_proto);
#else
static inline int net_ipv4_finalize(struct net_pkt *pkt,
				    u8_t next_header_proto)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(next_header_proto);

	return -ENOTSUP;
}
#endif

#endif /* __IPV4_H */
