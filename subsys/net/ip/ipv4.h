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

#include <stdint.h>

#include <net/net_ip.h>
#include <net/nbuf.h>
#include <net/net_if.h>
#include <net/net_context.h>

#include "ipv4.h"

/**
 * @brief Create IPv4 packet in provided net_buf.
 *
 * @param buf Network buffer
 * @param reserve Link layer reserve
 * @param src Source IPv4 address
 * @param dst Destination IPv4 address
 * @param iface Network interface
 * @param next_header Protocol type of the next header after IPv4 header.
 *
 * @return Return network buffer that contains the IPv4 packet.
 */
struct net_buf *net_ipv4_create_raw(struct net_buf *buf,
				    uint16_t reserve,
				    const struct in_addr *src,
				    const struct in_addr *dst,
				    struct net_if *iface,
				    uint8_t next_header);

/**
 * @brief Create IPv4 packet in provided net_buf.
 *
 * @param context Network context for a connection
 * @param buf Network buffer
 * @param src_addr Source address, or NULL to choose a default
 * @param dst_addr Destination IPv4 address
 *
 * @return Return network buffer that contains the IPv6 packet.
 */
struct net_buf *net_ipv4_create(struct net_context *context,
				struct net_buf *buf,
				const struct in_addr *src_addr,
				const struct in_addr *dst_addr);

/**
 * @brief Finalize IPv4 packet. It should be called right before
 * sending the packet and after all the data has been added into
 * the packet. This function will set the length of the
 * packet and calculate the higher protocol checksum if needed.
 *
 * @param buf Network buffer
 * @param next_header Protocol type of the next header after IPv4 header.
 *
 * @return Return network buffer that contains the IPv4 packet.
 */
struct net_buf *net_ipv4_finalize_raw(struct net_buf *buf,
				      uint8_t next_header);

/**
 * @brief Finalize IPv4 packet. It should be called right before
 * sending the packet and after all the data has been added into
 * the packet. This function will set the length of the
 * packet and calculate the higher protocol checksum if needed.
 *
 * @param context Network context for a connection
 * @param buf Network buffer
 *
 * @return Return network buffer that contains the IPv4 packet.
 */
struct net_buf *net_ipv4_finalize(struct net_context *context,
				  struct net_buf *buf);

#endif /* __IPV4_H */
