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

#include "icmpv6.h"

#define NET_IPV6_ND_HOP_LIMIT 255

#if !defined(CONFIG_NET_IPV6_NO_DAD)
int net_ipv6_start_dad(struct net_if *iface, struct net_if_addr *ifaddr);
#endif

int net_ipv6_send_ns(struct net_if *iface, struct net_buf *pending,
		     struct in6_addr *src, struct in6_addr *dst,
		     struct in6_addr *tgt, bool is_my_address);

int net_ipv6_start_rs(struct net_if *iface);
int net_ipv6_send_rs(struct net_if *iface);

#if defined(CONFIG_NET_IPV6)
void net_ipv6_init(void);
#else
#define net_ipv6_init(...)
#endif

#endif /* __IPV6_H */
