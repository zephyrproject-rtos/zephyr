/** @file
 @brief IPv6 data handler

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __IPV6_H
#define __IPV6_H

#include <stdint.h>

#include <net/net_ip.h>
#include <net/nbuf.h>
#include <net/net_if.h>
#include <net/net_context.h>

#include "icmpv6.h"

#define NET_IPV6_ND_HOP_LIMIT 255
#define NET_IPV6_ND_INFINITE_LIFETIME 0xFFFFFFFF

#define NET_IPV6_DEFAULT_PREFIX_LEN 64

#define NET_MAX_RS_COUNT 3

#if defined(CONFIG_NET_IPV6_DAD)
int net_ipv6_start_dad(struct net_if *iface, struct net_if_addr *ifaddr);
#endif

int net_ipv6_send_ns(struct net_if *iface, struct net_buf *pending,
		     struct in6_addr *src, struct in6_addr *dst,
		     struct in6_addr *tgt, bool is_my_address);

int net_ipv6_start_rs(struct net_if *iface);
int net_ipv6_send_rs(struct net_if *iface);
int net_ipv6_start_rs(struct net_if *iface);

/**
 * @brief Create IPv6 packet in provided net_buf.
 *
 * @param context Network context for a connection
 * @param buf Network buffer
 * @param dst_addr Destination IPv6 address
 *
 * @return Return network buffer that contains the IPv6 packet.
 */
struct net_buf *net_ipv6_create(struct net_context *context,
				struct net_buf *buf,
				const struct in6_addr *dst_addr);

/**
 * @brief Finalize IPv6 packet. It should be called right before
 * sending the packet and after all the data has been added into
 * the packet. This function will set the length of the
 * packet and calculate the higher protocol checksum if needed.
 *
 * @param context Network context for a connection
 * @param buf Network buffer
 *
 * @return Return network buffer that contains the IPv6 packet.
 */
struct net_buf *net_ipv6_finalize(struct net_context *context,
				  struct net_buf *buf);

/**
 * @brief Make sure the link layer address is set according to
 * destination address. If the ll address is not yet known, then
 * start neighbor discovery to find it out. If ND needs to be done
 * then the returned packet is the Neighbor Solicitation message
 * and the original message is sent after Neighbor Advertisement
 * message is received.
 *
 * @param buf Network buffer
 *
 * @return Return network buffer to be sent.
 */
#if defined(CONFIG_NET_IPV6_ND)
struct net_buf *net_ipv6_prepare_for_send(struct net_buf *buf);
#else
static inline struct net_buf *net_ipv6_prepare_for_send(struct net_buf *buf)
{
	return buf;
}
#endif

#if defined(CONFIG_NET_IPV6)
void net_ipv6_init(void);
#else
#define net_ipv6_init(...)
#endif

#endif /* __IPV6_H */
