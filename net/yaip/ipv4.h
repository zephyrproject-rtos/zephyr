/** @file
 @brief IPv4 related functions

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
 * @param context Network context for a connection
 * @param buf Network buffer
 * @param dst_addr Destination IPv4 address
 *
 * @return Return network buffer that contains the IPv6 packet.
 */
struct net_buf *net_ipv4_create(struct net_context *context,
				struct net_buf *buf,
				const struct in_addr *addr);

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
