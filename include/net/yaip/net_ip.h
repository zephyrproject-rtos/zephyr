/** @file
 * @brief IPv6 and IPv4 definitions
 *
 * Generic IPv6 and IPv4 address definitions.
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

#ifndef __NET_IP_H
#define __NET_IP_H

#include <stdint.h>
#include <misc/byteorder.h>

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

#define ntohs(x) sys_be16_to_cpu(x)
#define ntohl(x) sys_be32_to_cpu(x)
#define htons(x) sys_cpu_to_be16(x)
#define htonl(x) sys_cpu_to_be32(x)

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
#if defined(CONFIG_NET_IPV6)
		struct in6_addr in6_addr;
#endif
#if defined(CONFIG_NET_IPV4)
		struct in_addr in_addr;
#endif
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

/** How the network address is assigned to network interface */
enum net_addr_type {
	NET_ADDR_ANY = 0,
	NET_ADDR_AUTOCONF,
	NET_ADDR_DHCP,
	NET_ADDR_MANUAL,
};

/** What is the current state of the network address */
enum net_addr_state {
	NET_ADDR_TENTATIVE = 0,
	NET_ADDR_PREFERRED,
	NET_ADDR_DEPRECATED,
};

struct net_ipv6_hdr {
	uint8_t vtc;
	uint8_t tcflow;
	uint16_t flow;
	uint8_t len[2];
	uint8_t nexthdr;
	uint8_t hop_limit;
	struct in6_addr src;
	struct in6_addr dst;
} __attribute__((__packed__));

#define NET_UDPH_LEN	8			/* Size of UDP header */
#define NET_TCPH_LEN	20			/* Size of TCP header */
#define NET_ICMPH_LEN	4			/* Size of ICMP header */

#define NET_IPV6H_LEN	   40			/* Size of IPv6 header */
#define NET_ICMPV6H_LEN	   NET_ICMPH_LEN	/* Size of ICMPv6 header */
#define NET_IPV6UDPH_LEN   (NET_UDPH_LEN + NET_IPV6H_LEN) /* IPv6 + UDP */
#define NET_IPV6TCPH_LEN   (NET_TCPH_LEN + NET_IPV6H_LEN) /* IPv6 + TCP */
#define NET_IPV6ICMPH_LEN  (NET_IPV6H_LEN + NET_ICMPH_LEN) /* ICMPv6 + IPv6 */
#define NET_IPV6_FRAGH_LEN 8

#define NET_IPV4H_LEN	   20			/* Size of IPv4 header */
#define NET_ICMPV4H_LEN	   NET_ICMPH_LEN	/* Size of ICMPv4 header */
#define NET_IPV4UDPH_LEN   (NET_UDPH_LEN + NET_IPV4H_LEN) /* IPv4 + UDP */
#define NET_IPV4TCPH_LEN   (NET_TCPH_LEN + NET_IPV4H_LEN) /* IPv4 + TCP */
#define NET_IPV4ICMPH_LEN  (NET_IPV4H_LEN + NET_ICMPH_LEN) /* ICMPv4 + IPv4 */

#ifdef __cplusplus
}
#endif

#endif /* __NET_IP_H */
