/** @file
 @brief ICMPv6 handler

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

#ifndef __ICMPV6_H
#define __ICMPV6_H

#include <misc/slist.h>
#include <stdint.h>

#include <net/net_ip.h>
#include <net/nbuf.h>

#define NET_ICMPV6_ECHO_REQUEST 128
#define NET_ICMPV6_ECHO_REPLY   129

typedef enum net_verdict (*icmpv6_callback_handler_t)(struct net_buf *buf);

struct net_icmpv6_handler {
	sys_snode_t node;
	uint8_t type;
	uint8_t code;
	icmpv6_callback_handler_t handler;
};

void net_icmpv6_register_handler(struct net_icmpv6_handler *handler);
enum net_verdict net_icmpv6_input(struct net_buf *buf, uint16_t len,
				  uint8_t type, uint8_t code);
#if defined(CONFIG_NET_IPV6)
void net_icmpv6_init(void);
#else
#define net_icmpv6_init(...)
#endif

#endif /* __ICMPV6_H */
