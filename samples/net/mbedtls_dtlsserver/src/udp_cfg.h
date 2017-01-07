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

#ifndef UDP_CONFIG_H_
#define UDP_CONFIG_H_

#if defined(CONFIG_NET_IPV6)
#define MCAST_IP_ADDR { { { 0xff, 0x84, 0, 0, 0, 0, 0, 0, \
			    0, 0, 0, 0, 0, 0, 0, 0x2 } } }

#define SERVER_PREFIX_LEN 64

static struct in6_addr server_addr;
static struct in6_addr mcast_addr = MCAST_IP_ADDR;

#else

static struct in_addr server_addr;

#endif

#define SERVER_PORT	4433

#endif
