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

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
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
	IPPROTO_ICMP = 1,
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

#if NET_DEBUG > 0
static inline char *net_addr_type2str(enum net_addr_type type)
{
	switch (type) {
	case NET_ADDR_AUTOCONF:
		return "AUTO";
	case NET_ADDR_DHCP:
		return "DHCP";
	case NET_ADDR_MANUAL:
		return "MANUAL";
	case NET_ADDR_ANY:
	default:
		break;
	}

	return "<unknown>";
}
#else
static inline char *net_addr_type2str(enum net_addr_type type)
{
	return NULL;
}
#endif

/** What is the current state of the network address */
enum net_addr_state {
	NET_ADDR_ANY_STATE = -1,
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

struct net_ipv4_hdr {
	uint8_t vhl;
	uint8_t tos;
	uint8_t len[2];
	uint8_t id[2];
	uint8_t offset[2];
	uint8_t ttl;
	uint8_t proto;
	uint16_t chksum;
	struct in_addr src;
	struct in_addr dst;
} __attribute__((__packed__));

struct net_icmp_hdr {
	uint8_t type;
	uint8_t code;
	uint16_t chksum;
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

/** @brief Check if the IPv6 address is a loopback address (::1).
 *
 * @param addr IPv6 address
 *
 * @return True if address is a loopback address, False otherwise.
 */
static inline bool net_is_ipv6_addr_loopback(struct in6_addr *addr)
{
	return addr->s6_addr32[0] == 0 &&
		addr->s6_addr32[1] == 0 &&
		addr->s6_addr32[2] == 0 &&
		ntohl(addr->s6_addr32[3]) == 1;
}

/** @brief Check if the IPv6 address is a multicast address.
 *
 * @param addr IPv6 address
 *
 * @return True if address is multicast address, False otherwise.
 */
static inline bool net_is_ipv6_addr_mcast(struct in6_addr *addr)
{
	return addr->s6_addr[0] == 0xFF;
}

extern struct net_if_addr *net_if_ipv6_addr_lookup(struct in6_addr *addr);

/** @brief Check if IPv6 address is found in one of the network interfaces.
 *
 * @param addr IPv6 address
 *
 * @return True if address was found, False otherwise.
 */
static inline bool net_is_my_ipv6_addr(struct in6_addr *addr)
{
	return net_if_ipv6_addr_lookup(addr) != NULL;
}

extern struct net_if_mcast_addr *net_if_ipv6_maddr_lookup(struct in6_addr *addr);

/** @brief Check if IPv6 multicast address is found in one of the
 * network interfaces.
 *
 * @param maddr Multicast IPv6 address
 *
 * @return True if address was found, False otherwise.
 */
static inline bool net_is_my_ipv6_maddr(struct in6_addr *maddr)
{
	return net_if_ipv6_maddr_lookup(maddr) != NULL;
}

/** @brief Check if two IPv6 addresses are same when compared after prefix mask.
 *
 * @param addr1 First IPv6 address.
 * @param addr2 Second IPv6 address.
 * @param length Prefix length (max length is 128).
 *
 * @return True if addresses are the same, False otherwise.
 */
static inline bool net_is_ipv6_prefix(uint8_t *addr1, uint8_t *addr2,
				      uint8_t length)
{
	uint8_t bits = 128 - length;
	uint8_t bytes = bits / 8;
	uint8_t remain = bits % 8;

	if (length > 128) {
		return false;
	}

	if (memcmp(addr1, addr2, 16 - bytes)) {
		return false;
	}

	return ((addr1[16 - bytes] & ((8 - remain) << 8))
		==
		(addr2[16 - bytes] & ((8 - remain) << 8)));
}

extern struct net_if_addr *net_if_ipv4_addr_lookup(struct in_addr *addr);

/** @brief Check if the IPv4 address is assigned to any network interface
 * in the system.
 *
 * @return True if IPv4 address is found in one of the network interfaces,
 * False otherwise.
 */
static inline bool net_is_my_ipv4_addr(struct in_addr *addr)
{
	return net_if_ipv4_addr_lookup(addr) != NULL;
}

/** @def net_ipaddr_copy
 *  @brief Copy an IPv4 or IPv6 address
 *
 *  @param dest Destination IP address.
 *  @param src Source IP address.
 *
 *  @return Destination address.
 */
#define net_ipaddr_copy(dest, src) (*(dest) = *(src))

/** @def net_ipv4_addr_cmp
 *  @brief Compare two IPv4 addresses
 *
 *  @param addr1 Pointer to IPv4 address.
 *  @param addr2 Pointer to IPv4 address.
 *
 *  @return True if the addresses are the same, false otherwise.
 */
#define net_ipv4_addr_cmp(addr1, addr2) \
	((addr1)->s_addr[0] == (addr2)->s_addr[0])

/** @brief Check if the given IPv6 address is a link local address.
 *
 * @return True if it is, false otherwise.
 */
static inline bool net_is_ipv6_ll_addr(struct in6_addr *addr)
{
	return ((addr->s6_addr[0]) == 0xFE) &&
		((addr->s6_addr[1]) == 0x80);
}

struct in6_addr *net_if_ipv6_unspecified_addr(void);

/** @brief Return pointer to any (all bits zeros) IPv6 address.
 *
 * @return Any IPv6 address.
 */
static inline struct in6_addr *net_ipv6_unspecified_address(void)
{
	return net_if_ipv6_unspecified_addr();
}

extern const struct in_addr *net_if_ipv4_broadcast_addr(void);

/** @brief Return pointer to broadcast (all bits ones) IPv4 address.
 *
 * @return Broadcast IPv4 address.
 */
static inline const struct in_addr *net_ipv4_broadcast_address(void)
{
	return net_if_ipv4_broadcast_addr();
}

struct net_if;
extern bool net_if_ipv4_addr_mask_cmp(struct net_if *iface,
				      struct in_addr *addr);

/** @brief Check if the given address belongs to same subnet that
 * has been configured for the interface.
 *
 * @return True if address is in same subnet, false otherwise.
 */
static inline bool net_ipv4_addr_mask_cmp(struct net_if *iface,
					  struct in_addr *addr)
{
	return net_if_ipv4_addr_mask_cmp(iface, addr);
}

#ifdef __cplusplus
}
#endif

#endif /* __NET_IP_H */
