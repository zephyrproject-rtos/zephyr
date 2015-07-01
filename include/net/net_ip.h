/** @file
 @brief IPv6 and IPv4 definitions

 Generic IPv6 and IPv4 address definitions.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>

#ifndef __NET_IP_H
#define __NET_IP_H

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

#define IN6ADDR_ANY_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } }
#define IN6ADDR_LOOPBACK_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } }

#define INET6_ADDRSTRLEN 46

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

#endif /* __NET_IP_H */
