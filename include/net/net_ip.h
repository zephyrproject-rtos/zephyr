/** @file
 * @brief IPv6 and IPv4 definitions
 *
 * Generic IPv6 and IPv4 address definitions.
 */

/*
 * Copyright (c) 2015 Intel Corporation
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

#ifndef __NET_IP_H
#define __NET_IP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Protocol families */
#define PF_UNSPEC	0	/* Unspecified.  */
#define PF_INET		2	/* IP protocol family.  */
#define PF_INET6	10	/* IP version 6.  */

/** Address families.  */
#define AF_UNSPEC	PF_UNSPEC
#define AF_INET		PF_INET
#define AF_INET6	PF_INET6

/** Protocol numbers from IANA */
enum ip_protocol {
	IPPROTO_TCP = 6,
	IPPROTO_UDP = 17,
	IPPROTO_ICMPV6 = 58,
};

/** IPv6 address structure */
struct in6_addr {
	union {
		uint8_t		u6_addr8[16];
		uint16_t	u6_addr16[8]; /* In big endian */
		uint32_t	u6_addr32[4]; /* In big endian */
	} in6_u;
#define s6_addr			in6_u.u6_addr8
#define s6_addr16		in6_u.u6_addr16
#define s6_addr32		in6_u.u6_addr32
};

/** IPv4 address */
struct in_addr {
	union {
		uint8_t		u4_addr8[4];
		uint16_t	u4_addr16[2]; /* In big endian */
		uint32_t	u4_addr32[1]; /* In big endian */
	} in4_u;
#define s4_addr			in4_u.u4_addr8
#define s4_addr16		in4_u.u4_addr16
#define s4_addr32		in4_u.u4_addr32

#define s_addr			s4_addr32
};

typedef unsigned short int sa_family_t;

struct net_addr {
	sa_family_t family;
	union {
		struct in6_addr in6_addr;
		struct in_addr in_addr;
	};
};

#define IN6ADDR_ANY_INIT { { { 0, 0, 0, 0, 0, 0, 0, 0, 0, \
				0, 0, 0, 0, 0, 0, 0 } } }
#define IN6ADDR_LOOPBACK_INIT { { { 0, 0, 0, 0, 0, 0, 0, \
				0, 0, 0, 0, 0, 0, 0, 0, 1 } } }

#define INET6_ADDRSTRLEN 46

#define INADDR_ANY 0

/** IPv6/IPv4 network connection tuple */
struct net_tuple {
	/** IPv6/IPv4 remote address */
	struct net_addr *remote_addr;
	/** IPv6/IPv4 local address */
	struct net_addr *local_addr;
	/** UDP/TCP remote port */
	uint16_t remote_port;
	/** UDP/TCP local port */
	uint16_t local_port;
	/** IP protocol */
	enum ip_protocol ip_proto;
};

#if defined(CONFIG_NETWORKING_WITH_TCP)
enum tcp_event_type {
	TCP_READ_EVENT = 1,
	TCP_WRITE_EVENT,
};
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NET_IP_H */
