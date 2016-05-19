/** @file
 @brief ICMPv4 handler

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

#ifndef __ICMPV4_H
#define __ICMPV4_H

#include <stdint.h>

#include <net/net_ip.h>
#include <net/nbuf.h>

#define NET_ICMPV4_ECHO_REQUEST 8
#define NET_ICMPV4_ECHO_REPLY   0

enum net_verdict net_icmpv4_input(struct net_buf *buf, uint16_t len,
				  uint8_t type, uint8_t code);

#endif /* __ICMPV4_H */
