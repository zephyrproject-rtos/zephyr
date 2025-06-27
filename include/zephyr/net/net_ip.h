/** @file
 * @brief IPv6 and IPv4 definitions
 *
 * Generic IPv6 and IPv4 address definitions.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_IP_H_
#define ZEPHYR_INCLUDE_NET_NET_IP_H_

/**
 * @brief IPv4/IPv6 primitives and helpers
 * @defgroup ip_4_6 IPv4/IPv6 primitives and helpers
 * @since 1.0
 * @version 1.0.0
 * @ingroup networking
 * @{
 */

#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/toolchain.h>

#include <zephyr/net/net_linkaddr.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */
/* Specifying VLAN tag here in order to avoid circular dependencies */
#define NET_VLAN_TAG_UNSPEC 0x0fff
/** @endcond */

/* Protocol families. */
#define PF_UNSPEC       0          /**< Unspecified protocol family.  */
#define PF_INET         1          /**< IP protocol family version 4. */
#define PF_INET6        2          /**< IP protocol family version 6. */
#define PF_PACKET       3          /**< Packet family.                */
#define PF_CAN          4          /**< Controller Area Network.      */
#define PF_NET_MGMT     5          /**< Network management info.      */
#define PF_LOCAL        6          /**< Inter-process communication   */
#define PF_UNIX         PF_LOCAL   /**< Inter-process communication   */

/* Address families. */
#define AF_UNSPEC      PF_UNSPEC   /**< Unspecified address family.   */
#define AF_INET        PF_INET     /**< IP protocol family version 4. */
#define AF_INET6       PF_INET6    /**< IP protocol family version 6. */
#define AF_PACKET      PF_PACKET   /**< Packet family.                */
#define AF_CAN         PF_CAN      /**< Controller Area Network.      */
#define AF_NET_MGMT    PF_NET_MGMT /**< Network management info.      */
#define AF_LOCAL       PF_LOCAL    /**< Inter-process communication   */
#define AF_UNIX        PF_UNIX     /**< Inter-process communication   */

/** Protocol numbers from IANA/BSD */
enum net_ip_protocol {
	IPPROTO_IP = 0,            /**< IP protocol (pseudo-val for setsockopt() */
	IPPROTO_ICMP = 1,          /**< ICMP protocol   */
	IPPROTO_IGMP = 2,          /**< IGMP protocol   */
	IPPROTO_ETH_P_ALL = 3,     /**< Every packet. from linux if_ether.h   */
	IPPROTO_IPIP = 4,          /**< IPIP tunnels    */
	IPPROTO_TCP = 6,           /**< TCP protocol    */
	IPPROTO_UDP = 17,          /**< UDP protocol    */
	IPPROTO_IPV6 = 41,         /**< IPv6 protocol   */
	IPPROTO_ICMPV6 = 58,       /**< ICMPv6 protocol */
	IPPROTO_RAW = 255,         /**< RAW IP packets  */
};

/** Protocol numbers for TLS protocols */
enum net_ip_protocol_secure {
	IPPROTO_TLS_1_0 = 256,     /**< TLS 1.0 protocol */
	IPPROTO_TLS_1_1 = 257,     /**< TLS 1.1 protocol */
	IPPROTO_TLS_1_2 = 258,     /**< TLS 1.2 protocol */
	IPPROTO_TLS_1_3 = 259,     /**< TLS 1.3 protocol */
	IPPROTO_DTLS_1_0 = 272,    /**< DTLS 1.0 protocol */
	IPPROTO_DTLS_1_2 = 273,    /**< DTLS 1.2 protocol */
};

/** Socket type */
enum net_sock_type {
	SOCK_STREAM = 1,           /**< Stream socket type   */
	SOCK_DGRAM,                /**< Datagram socket type */
	SOCK_RAW                   /**< RAW socket type      */
};

/** @brief Convert 16-bit value from network to host byte order.
 *
 * @param x The network byte order value to convert.
 *
 * @return Host byte order value.
 */
#define ntohs(x) sys_be16_to_cpu(x)

/** @brief Convert 32-bit value from network to host byte order.
 *
 * @param x The network byte order value to convert.
 *
 * @return Host byte order value.
 */
#define ntohl(x) sys_be32_to_cpu(x)

/** @brief Convert 64-bit value from network to host byte order.
 *
 * @param x The network byte order value to convert.
 *
 * @return Host byte order value.
 */
#define ntohll(x) sys_be64_to_cpu(x)

/** @brief Convert 16-bit value from host to network byte order.
 *
 * @param x The host byte order value to convert.
 *
 * @return Network byte order value.
 */
#define htons(x) sys_cpu_to_be16(x)

/** @brief Convert 32-bit value from host to network byte order.
 *
 * @param x The host byte order value to convert.
 *
 * @return Network byte order value.
 */
#define htonl(x) sys_cpu_to_be32(x)

/** @brief Convert 64-bit value from host to network byte order.
 *
 * @param x The host byte order value to convert.
 *
 * @return Network byte order value.
 */
#define htonll(x) sys_cpu_to_be64(x)

/** IPv6 address struct */
struct in6_addr {
	union {
		uint8_t s6_addr[16];   /**< IPv6 address buffer */
		uint16_t s6_addr16[8]; /**< In big endian */
		uint32_t s6_addr32[4]; /**< In big endian */
	};
};

/** Binary size of the IPv6 address */
#define NET_IPV6_ADDR_SIZE 16

/** IPv4 address struct */
struct in_addr {
	union {
		uint8_t s4_addr[4];    /**< IPv4 address buffer */
		uint16_t s4_addr16[2]; /**< In big endian */
		uint32_t s4_addr32[1]; /**< In big endian */
		uint32_t s_addr; /**< In big endian, for POSIX compatibility. */
	};
};

/** Binary size of the IPv4 address */
#define NET_IPV4_ADDR_SIZE 4

/** Socket address family type */
typedef unsigned short int sa_family_t;

/** Length of a socket address */
#ifndef __socklen_t_defined
typedef size_t socklen_t;
#define __socklen_t_defined
#endif

/*
 * Note that the sin_port and sin6_port are in network byte order
 * in various sockaddr* structs.
 */

/** Socket address struct for IPv6. */
struct sockaddr_in6 {
	sa_family_t		sin6_family;   /**< AF_INET6               */
	uint16_t		sin6_port;     /**< Port number            */
	struct in6_addr		sin6_addr;     /**< IPv6 address           */
	uint8_t			sin6_scope_id; /**< Interfaces for a scope */
};

/** Socket address struct for IPv4. */
struct sockaddr_in {
	sa_family_t		sin_family;    /**< AF_INET      */
	uint16_t		sin_port;      /**< Port number  */
	struct in_addr		sin_addr;      /**< IPv4 address */
};

/** Socket address struct for packet socket. */
struct sockaddr_ll {
	sa_family_t sll_family;   /**< Always AF_PACKET                   */
	uint16_t    sll_protocol; /**< Physical-layer protocol            */
	int         sll_ifindex;  /**< Interface number                   */
	uint16_t    sll_hatype;   /**< ARP hardware type                  */
	uint8_t     sll_pkttype;  /**< Packet type                        */
	uint8_t     sll_halen;    /**< Length of address                  */
	uint8_t     sll_addr[8];  /**< Physical-layer address, big endian */
};

/** @cond INTERNAL_HIDDEN */

/** Socket address struct for IPv6 where address is a pointer */
struct sockaddr_in6_ptr {
	sa_family_t		sin6_family;   /**< AF_INET6               */
	uint16_t		sin6_port;     /**< Port number            */
	struct in6_addr		*sin6_addr;    /**< IPv6 address           */
	uint8_t			sin6_scope_id; /**< interfaces for a scope */
};

/** Socket address struct for IPv4 where address is a pointer */
struct sockaddr_in_ptr {
	sa_family_t		sin_family;    /**< AF_INET      */
	uint16_t		sin_port;      /**< Port number  */
	struct in_addr		*sin_addr;     /**< IPv4 address */
};

/** Socket address struct for packet socket where address is a pointer */
struct sockaddr_ll_ptr {
	sa_family_t sll_family;   /**< Always AF_PACKET                   */
	uint16_t    sll_protocol; /**< Physical-layer protocol            */
	int         sll_ifindex;  /**< Interface number                   */
	uint16_t    sll_hatype;   /**< ARP hardware type                  */
	uint8_t     sll_pkttype;  /**< Packet type                        */
	uint8_t     sll_halen;    /**< Length of address                  */
	uint8_t     *sll_addr;    /**< Physical-layer address, big endian */
};

/** Socket address struct for unix socket where address is a pointer */
struct sockaddr_un_ptr {
	sa_family_t sun_family;    /**< Always AF_UNIX */
	char        *sun_path;     /**< pathname */
};

struct sockaddr_can_ptr {
	sa_family_t can_family;
	int         can_ifindex;
};

/** @endcond */

#if !defined(HAVE_IOVEC)
/** IO vector array element */
struct iovec {
	void  *iov_base; /**< Pointer to data */
	size_t iov_len;  /**< Length of the data */
};
#endif

/** Message struct */
struct msghdr {
	void         *msg_name;       /**< Optional socket address, big endian */
	socklen_t     msg_namelen;    /**< Size of socket address */
	struct iovec *msg_iov;        /**< Scatter/gather array */
	size_t        msg_iovlen;     /**< Number of elements in msg_iov */
	void         *msg_control;    /**< Ancillary data */
	size_t        msg_controllen; /**< Ancillary data buffer len */
	int           msg_flags;      /**< Flags on received message */
};

/** Control message ancillary data */
struct cmsghdr {
	socklen_t cmsg_len;    /**< Number of bytes, including header */
	int       cmsg_level;  /**< Originating protocol */
	int       cmsg_type;   /**< Protocol-specific type */
	z_max_align_t cmsg_data[]; /**< Flexible array member to force alignment of cmsghdr */
};

/** @cond INTERNAL_HIDDEN */

/* Alignment for headers and data. These are arch specific but define
 * them here atm if not found already.
 */
#if !defined(ALIGN_H)
#define ALIGN_H(x) ROUND_UP(x, __alignof__(struct cmsghdr))
#endif
#if !defined(ALIGN_D)
#define ALIGN_D(x) ROUND_UP(x, __alignof__(z_max_align_t))
#endif

/** @endcond */

#if !defined(CMSG_FIRSTHDR)
/**
 * Returns a pointer to the first cmsghdr in the ancillary data buffer
 * associated with the passed msghdr.  It returns NULL if there isn't
 * enough space for a cmsghdr in the buffer.
 */
#define CMSG_FIRSTHDR(msghdr)					\
	((msghdr)->msg_controllen >= sizeof(struct cmsghdr) ?	\
	 (struct cmsghdr *)((msghdr)->msg_control) : NULL)
#endif

#if !defined(CMSG_NXTHDR)
/**
 * Returns the next valid cmsghdr after the passed cmsghdr. It returns NULL
 * when there isn't enough space left in the buffer.
 */
#define CMSG_NXTHDR(msghdr, cmsg)					 \
	(((cmsg) == NULL) ? CMSG_FIRSTHDR(msghdr) :			 \
	 (((uint8_t *)(cmsg) + ALIGN_H((cmsg)->cmsg_len) +		 \
	   ALIGN_D(sizeof(struct cmsghdr)) >				 \
	   (uint8_t *)((msghdr)->msg_control) + (msghdr)->msg_controllen) ? \
	  NULL :							 \
	  (struct cmsghdr *)((uint8_t *)(cmsg) +			 \
			     ALIGN_H((cmsg)->cmsg_len))))
#endif

#if !defined(CMSG_DATA)
/**
 * Returns a pointer to the data portion of a cmsghdr.  The pointer returned
 * cannot be assumed to be suitably aligned for accessing arbitrary payload
 * data types. Applications should not cast it to a pointer type matching
 * the payload, but should instead use memcpy(3) to copy data to or from a
 * suitably declared object.
 */
#define CMSG_DATA(cmsg) ((uint8_t *)(cmsg) + ALIGN_D(sizeof(struct cmsghdr)))
#endif

#if !defined(CMSG_SPACE)
/**
 * Returns the number of bytes an ancillary element with payload of the passed
 * data length occupies.
 */
#define CMSG_SPACE(length) (ALIGN_D(sizeof(struct cmsghdr)) + ALIGN_H(length))
#endif

#if !defined(CMSG_LEN)
/**
 * Returns the value to store in the cmsg_len member of the cmsghdr structure,
 * taking into account any necessary alignment.
 * It takes the data length as an argument.
 */
#define CMSG_LEN(length) (ALIGN_D(sizeof(struct cmsghdr)) + length)
#endif

/** @cond INTERNAL_HIDDEN */

/* Packet types.  */
#define PACKET_HOST         0     /* To us            */
#define PACKET_BROADCAST    1     /* To all           */
#define PACKET_MULTICAST    2     /* To group         */
#define PACKET_OTHERHOST    3     /* To someone else  */
#define PACKET_OUTGOING     4     /* Originated by us */
#define PACKET_LOOPBACK     5
#define PACKET_FASTROUTE    6

/* ARP protocol HARDWARE identifiers. */
#define ARPHRD_ETHER 1

/* Note: These macros are defined in a specific order.
 * The largest sockaddr size is the last one.
 */
#if defined(CONFIG_NET_IPV4)
#undef NET_SOCKADDR_MAX_SIZE
#undef NET_SOCKADDR_PTR_MAX_SIZE
#define NET_SOCKADDR_MAX_SIZE (sizeof(struct sockaddr_in))
#define NET_SOCKADDR_PTR_MAX_SIZE (sizeof(struct sockaddr_in_ptr))
#endif

#if defined(CONFIG_NET_SOCKETS_PACKET)
#undef NET_SOCKADDR_MAX_SIZE
#undef NET_SOCKADDR_PTR_MAX_SIZE
#define NET_SOCKADDR_MAX_SIZE (sizeof(struct sockaddr_ll))
#define NET_SOCKADDR_PTR_MAX_SIZE (sizeof(struct sockaddr_ll_ptr))
#endif

#if defined(CONFIG_NET_IPV6)
#undef NET_SOCKADDR_MAX_SIZE
#define NET_SOCKADDR_MAX_SIZE (sizeof(struct sockaddr_in6))
#if !defined(CONFIG_NET_SOCKETS_PACKET)
#undef NET_SOCKADDR_PTR_MAX_SIZE
#define NET_SOCKADDR_PTR_MAX_SIZE (sizeof(struct sockaddr_in6_ptr))
#endif
#endif

#if defined(CONFIG_NET_NATIVE_OFFLOADED_SOCKETS)
#define UNIX_PATH_MAX 108
#undef NET_SOCKADDR_MAX_SIZE
/* Define NET_SOCKADDR_MAX_SIZE to be struct of sa_family_t + char[UNIX_PATH_MAX] */
#define NET_SOCKADDR_MAX_SIZE (UNIX_PATH_MAX+sizeof(sa_family_t))
#if !defined(CONFIG_NET_SOCKETS_PACKET)
#undef NET_SOCKADDR_PTR_MAX_SIZE
#define NET_SOCKADDR_PTR_MAX_SIZE (sizeof(struct sockaddr_un_ptr))
#endif
#endif

#if !defined(CONFIG_NET_IPV4)
#if !defined(CONFIG_NET_IPV6)
#if !defined(CONFIG_NET_SOCKETS_PACKET)
#if !defined(CONFIG_NET_NATIVE_OFFLOADED_SOCKETS)
#define NET_SOCKADDR_MAX_SIZE (sizeof(struct sockaddr_in6))
#define NET_SOCKADDR_PTR_MAX_SIZE (sizeof(struct sockaddr_in6_ptr))
#endif
#endif
#endif
#endif

/** @endcond */

/** Generic sockaddr struct. Must be cast to proper type. */
struct sockaddr {
	sa_family_t sa_family; /**< Address family */
/** @cond INTERNAL_HIDDEN */
	char data[NET_SOCKADDR_MAX_SIZE - sizeof(sa_family_t)];
/** @endcond */
};

/** @cond INTERNAL_HIDDEN */

struct sockaddr_ptr {
	sa_family_t family;
	char data[NET_SOCKADDR_PTR_MAX_SIZE - sizeof(sa_family_t)];
};

/* Same as sockaddr in our case */
struct sockaddr_storage {
	sa_family_t ss_family;
	char data[NET_SOCKADDR_MAX_SIZE - sizeof(sa_family_t)];
};

/* Socket address struct for UNIX domain sockets */
struct sockaddr_un {
	sa_family_t sun_family;    /* AF_UNIX */
	char        sun_path[NET_SOCKADDR_MAX_SIZE - sizeof(sa_family_t)];
};

struct net_addr {
	sa_family_t family;
	union {
		struct in6_addr in6_addr;
		struct in_addr in_addr;
	};
};

/** A pointer to IPv6 any address (all values zero) */
extern const struct in6_addr in6addr_any;

/** A pointer to IPv6 loopback address (::1) */
extern const struct in6_addr in6addr_loopback;

/** @endcond */

/** IPv6 address initializer */
#define IN6ADDR_ANY_INIT { { { 0, 0, 0, 0, 0, 0, 0, 0, 0, \
				0, 0, 0, 0, 0, 0, 0 } } }

/** IPv6 loopback address initializer */
#define IN6ADDR_LOOPBACK_INIT { { { 0, 0, 0, 0, 0, 0, 0, \
				0, 0, 0, 0, 0, 0, 0, 0, 1 } } }

/** IPv4 any address */
#define INADDR_ANY 0

/** IPv4 broadcast address */
#define INADDR_BROADCAST 0xffffffff

/** IPv4 address initializer */
#define INADDR_ANY_INIT { { { INADDR_ANY } } }

/** IPv6 loopback address initializer */
#define INADDR_LOOPBACK_INIT  { { { 127, 0, 0, 1 } } }

/** Max length of the IPv4 address as a string. Defined by POSIX. */
#define INET_ADDRSTRLEN 16
/** Max length of the IPv6 address as a string. Takes into account possible
 * mapped IPv4 addresses.
 */
#define INET6_ADDRSTRLEN 46

/** @cond INTERNAL_HIDDEN */

/* These are for internal usage of the stack */
#define NET_IPV6_ADDR_LEN sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx")
#define NET_IPV4_ADDR_LEN sizeof("xxx.xxx.xxx.xxx")

/** @endcond */

/** @brief IP Maximum Transfer Unit */
enum net_ip_mtu {
	/** IPv6 MTU length. We must be able to receive this size IPv6 packet
	 * without fragmentation.
	 */
#if defined(CONFIG_NET_NATIVE_IPV6)
	NET_IPV6_MTU = CONFIG_NET_IPV6_MTU,
#else
	NET_IPV6_MTU = 1280,
#endif

	/** IPv4 MTU length. We must be able to receive this size IPv4 packet
	 * without fragmentation.
	 */
	NET_IPV4_MTU = 576,
};

/** @brief Network packet priority settings described in IEEE 802.1Q Annex I.1 */
enum net_priority {
	NET_PRIORITY_BK = 1, /**< Background (lowest)                */
	NET_PRIORITY_BE = 0, /**< Best effort (default)              */
	NET_PRIORITY_EE = 2, /**< Excellent effort                   */
	NET_PRIORITY_CA = 3, /**< Critical applications              */
	NET_PRIORITY_VI = 4, /**< Video, < 100 ms latency and jitter */
	NET_PRIORITY_VO = 5, /**< Voice, < 10 ms latency and jitter  */
	NET_PRIORITY_IC = 6, /**< Internetwork control               */
	NET_PRIORITY_NC = 7  /**< Network control (highest)          */
} __packed;

#define NET_MAX_PRIORITIES 8 /**< How many priority values there are */

/** @brief IPv6/IPv4 network connection tuple */
struct net_tuple {
	struct net_addr *remote_addr;  /**< IPv6/IPv4 remote address */
	struct net_addr *local_addr;   /**< IPv6/IPv4 local address  */
	uint16_t remote_port;          /**< UDP/TCP remote port      */
	uint16_t local_port;           /**< UDP/TCP local port       */
	enum net_ip_protocol ip_proto; /**< IP protocol              */
};

/** @brief What is the current state of the network address */
enum net_addr_state {
	NET_ADDR_ANY_STATE = -1, /**< Default (invalid) address type */
	NET_ADDR_TENTATIVE = 0,  /**< Tentative address              */
	NET_ADDR_PREFERRED,      /**< Preferred address              */
	NET_ADDR_DEPRECATED,     /**< Deprecated address             */
} __packed;

/** @brief How the network address is assigned to network interface */
enum net_addr_type {
	/** Default value. This is not a valid value. */
	NET_ADDR_ANY = 0,
	/** Auto configured address */
	NET_ADDR_AUTOCONF,
	/** Address is from DHCP */
	NET_ADDR_DHCP,
	/** Manually set address */
	NET_ADDR_MANUAL,
	/** Manually set address which is overridable by DHCP */
	NET_ADDR_OVERRIDABLE,
} __packed;

/** @cond INTERNAL_HIDDEN */

struct net_ipv6_hdr {
	uint8_t vtc;
	uint8_t tcflow;
	uint16_t flow;
	uint16_t len;
	uint8_t nexthdr;
	uint8_t hop_limit;
	uint8_t src[NET_IPV6_ADDR_SIZE];
	uint8_t dst[NET_IPV6_ADDR_SIZE];
} __packed;

struct net_ipv6_frag_hdr {
	uint8_t nexthdr;
	uint8_t reserved;
	uint16_t offset;
	uint32_t id;
} __packed;

struct net_ipv4_hdr {
	uint8_t vhl;
	uint8_t tos;
	uint16_t len;
	uint8_t id[2];
	uint8_t offset[2];
	uint8_t ttl;
	uint8_t proto;
	uint16_t chksum;
	uint8_t src[NET_IPV4_ADDR_SIZE];
	uint8_t dst[NET_IPV4_ADDR_SIZE];
} __packed;

struct net_icmp_hdr {
	uint8_t type;
	uint8_t code;
	uint16_t chksum;
} __packed;

struct net_udp_hdr {
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t len;
	uint16_t chksum;
} __packed;

struct net_tcp_hdr {
	uint16_t src_port;
	uint16_t dst_port;
	uint8_t seq[4];
	uint8_t ack[4];
	uint8_t offset;
	uint8_t flags;
	uint8_t wnd[2];
	uint16_t chksum;
	uint8_t urg[2];
	uint8_t optdata[0];
} __packed;

static inline const char *net_addr_type2str(enum net_addr_type type)
{
	switch (type) {
	case NET_ADDR_AUTOCONF:
		return "AUTO";
	case NET_ADDR_DHCP:
		return "DHCP";
	case NET_ADDR_MANUAL:
		return "MANUAL";
	case NET_ADDR_OVERRIDABLE:
		return "OVERRIDE";
	case NET_ADDR_ANY:
	default:
		break;
	}

	return "<unknown>";
}

/* IPv6 extension headers types */
#define NET_IPV6_NEXTHDR_HBHO        0
#define NET_IPV6_NEXTHDR_DESTO       60
#define NET_IPV6_NEXTHDR_ROUTING     43
#define NET_IPV6_NEXTHDR_FRAG        44
#define NET_IPV6_NEXTHDR_NONE        59

/**
 * This 2 unions are here temporarily, as long as net_context.h will
 * be still public and not part of the core only.
 */
union net_ip_header {
	struct net_ipv4_hdr *ipv4;
	struct net_ipv6_hdr *ipv6;
};

union net_proto_header {
	struct net_udp_hdr *udp;
	struct net_tcp_hdr *tcp;
};

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

#define NET_IPV6H_LENGTH_OFFSET		0x04	/* Offset of the Length field in the IPv6 header */

#define NET_IPV6_FRAGH_OFFSET_MASK	0xfff8	/* Mask for the 13-bit Fragment Offset field */
#define NET_IPV4_FRAGH_OFFSET_MASK	0x1fff	/* Mask for the 13-bit Fragment Offset field */
#define NET_IPV4_MORE_FRAG_MASK		0x2000	/* Mask for the 1-bit More Fragments field */
#define NET_IPV4_DO_NOT_FRAG_MASK	0x4000	/* Mask for the 1-bit Do Not Fragment field */

/** @endcond */

/**
 * @brief Check if the IPv6 address is a loopback address (::1).
 *
 * @param addr IPv6 address
 *
 * @return True if address is a loopback address, False otherwise.
 */
static inline bool net_ipv6_is_addr_loopback(struct in6_addr *addr)
{
	return UNALIGNED_GET(&addr->s6_addr32[0]) == 0 &&
		UNALIGNED_GET(&addr->s6_addr32[1]) == 0 &&
		UNALIGNED_GET(&addr->s6_addr32[2]) == 0 &&
		ntohl(UNALIGNED_GET(&addr->s6_addr32[3])) == 1;
}

/**
 * @brief Check if the IPv6 address is a multicast address.
 *
 * @param addr IPv6 address
 *
 * @return True if address is multicast address, False otherwise.
 */
static inline bool net_ipv6_is_addr_mcast(const struct in6_addr *addr)
{
	return addr->s6_addr[0] == 0xFF;
}

struct net_if;
struct net_if_config;

extern struct net_if_addr *net_if_ipv6_addr_lookup(const struct in6_addr *addr,
						   struct net_if **iface);

/**
 * @brief Check if IPv6 address is found in one of the network interfaces.
 *
 * @param addr IPv6 address
 *
 * @return True if address was found, False otherwise.
 */
static inline bool net_ipv6_is_my_addr(struct in6_addr *addr)
{
	return net_if_ipv6_addr_lookup(addr, NULL) != NULL;
}

extern struct net_if_mcast_addr *net_if_ipv6_maddr_lookup(
	const struct in6_addr *addr, struct net_if **iface);

/**
 * @brief Check if IPv6 multicast address is found in one of the
 * network interfaces.
 *
 * @param maddr Multicast IPv6 address
 *
 * @return True if address was found, False otherwise.
 */
static inline bool net_ipv6_is_my_maddr(struct in6_addr *maddr)
{
	return net_if_ipv6_maddr_lookup(maddr, NULL) != NULL;
}

/**
 * @brief Check if two IPv6 addresses are same when compared after prefix mask.
 *
 * @param addr1 First IPv6 address.
 * @param addr2 Second IPv6 address.
 * @param length Prefix length (max length is 128).
 *
 * @return True if IPv6 prefixes are the same, False otherwise.
 */
static inline bool net_ipv6_is_prefix(const uint8_t *addr1,
				      const uint8_t *addr2,
				      uint8_t length)
{
	uint8_t bits = 128 - length;
	uint8_t bytes = length / 8U;
	uint8_t remain = bits % 8;
	uint8_t mask;

	if (length > 128) {
		return false;
	}

	if (memcmp(addr1, addr2, bytes)) {
		return false;
	}

	if (!remain) {
		/* No remaining bits, the prefixes are the same as first
		 * bytes are the same.
		 */
		return true;
	}

	/* Create a mask that has remaining most significant bits set */
	mask = (uint8_t)((0xff << (8 - remain)) ^ 0xff) << remain;

	return (addr1[bytes] & mask) == (addr2[bytes] & mask);
}


/**
 * @brief Get the IPv6 network address via the unicast address and the prefix mask.
 *
 * @param inaddr Unicast IPv6 address.
 * @param outaddr Prefix masked IPv6 address (network address).
 * @param prefix_len Prefix length (max length is 128).
 */
static inline void net_ipv6_addr_prefix_mask(const uint8_t *inaddr,
					     uint8_t *outaddr,
					     uint8_t prefix_len)
{
	uint8_t bits = 128 - prefix_len;
	uint8_t bytes = prefix_len / 8U;
	uint8_t remain = bits % 8;
	uint8_t mask;

	memset(outaddr, 0, 16U);
	memcpy(outaddr, inaddr, bytes);

	if (!remain) {
		/* No remaining bits, the prefixes are the same as first
		 * bytes are the same.
		 */
		return;
	}

	/* Create a mask that has remaining most significant bits set */
	mask = (uint8_t)((0xff << (8 - remain)) ^ 0xff) << remain;
	outaddr[bytes] = inaddr[bytes] & mask;
}

/**
 * @brief Check if the IPv4 address is a loopback address (127.0.0.0/8).
 *
 * @param addr IPv4 address
 *
 * @return True if address is a loopback address, False otherwise.
 */
static inline bool net_ipv4_is_addr_loopback(struct in_addr *addr)
{
	return addr->s4_addr[0] == 127U;
}

/**
 *  @brief Check if the IPv4 address is unspecified (all bits zero)
 *
 *  @param addr IPv4 address.
 *
 *  @return True if the address is unspecified, false otherwise.
 */
static inline bool net_ipv4_is_addr_unspecified(const struct in_addr *addr)
{
	return UNALIGNED_GET(&addr->s_addr) == 0;
}

/**
 * @brief Check if the IPv4 address is a multicast address.
 *
 * @param addr IPv4 address
 *
 * @return True if address is multicast address, False otherwise.
 */
static inline bool net_ipv4_is_addr_mcast(const struct in_addr *addr)
{
	return (ntohl(UNALIGNED_GET(&addr->s_addr)) & 0xF0000000) == 0xE0000000;
}

/**
 * @brief Check if the given IPv4 address is a link local address.
 *
 * @param addr A valid pointer on an IPv4 address
 *
 * @return True if it is, false otherwise.
 */
static inline bool net_ipv4_is_ll_addr(const struct in_addr *addr)
{
	return (ntohl(UNALIGNED_GET(&addr->s_addr)) & 0xFFFF0000) == 0xA9FE0000;
}

/**
 * @brief Check if the given IPv4 address is from a private address range.
 *
 * See https://en.wikipedia.org/wiki/Reserved_IP_addresses for details.
 *
 * @param addr A valid pointer on an IPv4 address
 *
 * @return True if it is, false otherwise.
 */
static inline bool net_ipv4_is_private_addr(const struct in_addr *addr)
{
	uint32_t masked_24, masked_16, masked_12, masked_10, masked_8;

	masked_24 = ntohl(UNALIGNED_GET(&addr->s_addr)) & 0xFFFFFF00;
	masked_16 = masked_24 & 0xFFFF0000;
	masked_12 = masked_24 & 0xFFF00000;
	masked_10 = masked_24 & 0xFFC00000;
	masked_8 = masked_24 & 0xFF000000;

	return masked_8  == 0x0A000000 || /* 10.0.0.0/8      */
	       masked_10 == 0x64400000 || /* 100.64.0.0/10   */
	       masked_12 == 0xAC100000 || /* 172.16.0.0/12   */
	       masked_16 == 0xC0A80000 || /* 192.168.0.0/16  */
	       masked_24 == 0xC0000200 || /* 192.0.2.0/24    */
	       masked_24 == 0xC0336400 || /* 192.51.100.0/24 */
	       masked_24 == 0xCB007100;   /* 203.0.113.0/24  */
}

/**
 *  @brief Copy an IPv4 or IPv6 address
 *
 *  @param dest Destination IP address.
 *  @param src Source IP address.
 *
 *  @return Destination address.
 */
#define net_ipaddr_copy(dest, src) \
	UNALIGNED_PUT(UNALIGNED_GET(src), dest)

/**
 *  @brief Copy an IPv4 address raw buffer
 *
 *  @param dest Destination IP address.
 *  @param src Source IP address.
 */
static inline void net_ipv4_addr_copy_raw(uint8_t *dest,
					  const uint8_t *src)
{
	net_ipaddr_copy((struct in_addr *)dest, (const struct in_addr *)src);
}

/**
 *  @brief Copy an IPv6 address raw buffer
 *
 *  @param dest Destination IP address.
 *  @param src Source IP address.
 */
static inline void net_ipv6_addr_copy_raw(uint8_t *dest,
					  const uint8_t *src)
{
	memcpy(dest, src, sizeof(struct in6_addr));
}

/**
 *  @brief Compare two IPv4 addresses
 *
 *  @param addr1 Pointer to IPv4 address.
 *  @param addr2 Pointer to IPv4 address.
 *
 *  @return True if the addresses are the same, false otherwise.
 */
static inline bool net_ipv4_addr_cmp(const struct in_addr *addr1,
				     const struct in_addr *addr2)
{
	return UNALIGNED_GET(&addr1->s_addr) == UNALIGNED_GET(&addr2->s_addr);
}

/**
 *  @brief Compare two raw IPv4 address buffers
 *
 *  @param addr1 Pointer to IPv4 address buffer.
 *  @param addr2 Pointer to IPv4 address buffer.
 *
 *  @return True if the addresses are the same, false otherwise.
 */
static inline bool net_ipv4_addr_cmp_raw(const uint8_t *addr1,
					 const uint8_t *addr2)
{
	return net_ipv4_addr_cmp((const struct in_addr *)addr1,
				 (const struct in_addr *)addr2);
}

/**
 *  @brief Compare two IPv6 addresses
 *
 *  @param addr1 Pointer to IPv6 address.
 *  @param addr2 Pointer to IPv6 address.
 *
 *  @return True if the addresses are the same, false otherwise.
 */
static inline bool net_ipv6_addr_cmp(const struct in6_addr *addr1,
				     const struct in6_addr *addr2)
{
	return !memcmp(addr1, addr2, sizeof(struct in6_addr));
}

/**
 *  @brief Compare two raw IPv6 address buffers
 *
 *  @param addr1 Pointer to IPv6 address buffer.
 *  @param addr2 Pointer to IPv6 address buffer.
 *
 *  @return True if the addresses are the same, false otherwise.
 */
static inline bool net_ipv6_addr_cmp_raw(const uint8_t *addr1,
					 const uint8_t *addr2)
{
	return net_ipv6_addr_cmp((const struct in6_addr *)addr1,
				 (const struct in6_addr *)addr2);
}

/**
 * @brief Check if the given IPv6 address is a link local address.
 *
 * @param addr A valid pointer on an IPv6 address
 *
 * @return True if it is, false otherwise.
 */
static inline bool net_ipv6_is_ll_addr(const struct in6_addr *addr)
{
	return UNALIGNED_GET(&addr->s6_addr16[0]) == htons(0xFE80);
}

/**
 * @brief Check if the given IPv6 address is a site local address.
 *
 * @param addr A valid pointer on an IPv6 address
 *
 * @return True if it is, false otherwise.
 */
static inline bool net_ipv6_is_sl_addr(const struct in6_addr *addr)
{
	return UNALIGNED_GET(&addr->s6_addr16[0]) == htons(0xFEC0);
}


/**
 * @brief Check if the given IPv6 address is a unique local address.
 *
 * @param addr A valid pointer on an IPv6 address
 *
 * @return True if it is, false otherwise.
 */
static inline bool net_ipv6_is_ula_addr(const struct in6_addr *addr)
{
	return addr->s6_addr[0] == 0xFD;
}

/**
 * @brief Check if the given IPv6 address is a global address.
 *
 * @param addr A valid pointer on an IPv6 address
 *
 * @return True if it is, false otherwise.
 */
static inline bool net_ipv6_is_global_addr(const struct in6_addr *addr)
{
	return (addr->s6_addr[0] & 0xE0) == 0x20;
}

/**
 * @brief Check if the given IPv6 address is from a private/local address range.
 *
 * See https://en.wikipedia.org/wiki/Reserved_IP_addresses for details.
 *
 * @param addr A valid pointer on an IPv6 address
 *
 * @return True if it is, false otherwise.
 */
static inline bool net_ipv6_is_private_addr(const struct in6_addr *addr)
{
	uint32_t masked_32, masked_7;

	masked_32 = ntohl(UNALIGNED_GET(&addr->s6_addr32[0]));
	masked_7 = masked_32 & 0xfc000000;

	return masked_32 == 0x20010db8 || /* 2001:db8::/32 */
	       masked_7  == 0xfc000000;   /* fc00::/7      */
}

/**
 * @brief Return pointer to any (all bits zeros) IPv6 address.
 *
 * @return Any IPv6 address.
 */
const struct in6_addr *net_ipv6_unspecified_address(void);

/**
 * @brief Return pointer to any (all bits zeros) IPv4 address.
 *
 * @return Any IPv4 address.
 */
const struct in_addr *net_ipv4_unspecified_address(void);

/**
 * @brief Return pointer to broadcast (all bits ones) IPv4 address.
 *
 * @return Broadcast IPv4 address.
 */
const struct in_addr *net_ipv4_broadcast_address(void);

struct net_if;
extern bool net_if_ipv4_addr_mask_cmp(struct net_if *iface,
				      const struct in_addr *addr);

/**
 * @brief Check if the given address belongs to same subnet that
 * has been configured for the interface.
 *
 * @param iface A valid pointer on an interface
 * @param addr IPv4 address
 *
 * @return True if address is in same subnet, false otherwise.
 */
static inline bool net_ipv4_addr_mask_cmp(struct net_if *iface,
					  const struct in_addr *addr)
{
	return net_if_ipv4_addr_mask_cmp(iface, addr);
}

extern bool net_if_ipv4_is_addr_bcast(struct net_if *iface,
				      const struct in_addr *addr);

/**
 * @brief Check if the given IPv4 address is a broadcast address.
 *
 * @param iface Interface to use. Must be a valid pointer to an interface.
 * @param addr IPv4 address
 *
 * @return True if address is a broadcast address, false otherwise.
 */
#if defined(CONFIG_NET_NATIVE_IPV4)
static inline bool net_ipv4_is_addr_bcast(struct net_if *iface,
					  const struct in_addr *addr)
{
	if (net_ipv4_addr_cmp(addr, net_ipv4_broadcast_address())) {
		return true;
	}

	return net_if_ipv4_is_addr_bcast(iface, addr);
}
#else
static inline bool net_ipv4_is_addr_bcast(struct net_if *iface,
					  const struct in_addr *addr)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(addr);

	return false;
}
#endif

extern struct net_if_addr *net_if_ipv4_addr_lookup(const struct in_addr *addr,
						   struct net_if **iface);

/**
 * @brief Check if the IPv4 address is assigned to any network interface
 * in the system.
 *
 * @param addr A valid pointer on an IPv4 address
 *
 * @return True if IPv4 address is found in one of the network interfaces,
 * False otherwise.
 */
static inline bool net_ipv4_is_my_addr(const struct in_addr *addr)
{
	bool ret;

	ret = net_if_ipv4_addr_lookup(addr, NULL) != NULL;
	if (!ret) {
		ret = net_ipv4_is_addr_bcast(NULL, addr);
	}

	return ret;
}

/**
 *  @brief Check if the IPv6 address is unspecified (all bits zero)
 *
 *  @param addr IPv6 address.
 *
 *  @return True if the address is unspecified, false otherwise.
 */
static inline bool net_ipv6_is_addr_unspecified(const struct in6_addr *addr)
{
	return UNALIGNED_GET(&addr->s6_addr32[0]) == 0 &&
		UNALIGNED_GET(&addr->s6_addr32[1]) == 0 &&
		UNALIGNED_GET(&addr->s6_addr32[2]) == 0 &&
		UNALIGNED_GET(&addr->s6_addr32[3]) == 0;
}

/**
 *  @brief Check if the IPv6 address is solicited node multicast address
 *  FF02:0:0:0:0:1:FFXX:XXXX defined in RFC 3513
 *
 *  @param addr IPv6 address.
 *
 *  @return True if the address is solicited node address, false otherwise.
 */
static inline bool net_ipv6_is_addr_solicited_node(const struct in6_addr *addr)
{
	return UNALIGNED_GET(&addr->s6_addr32[0]) == htonl(0xff020000) &&
		UNALIGNED_GET(&addr->s6_addr32[1]) == 0x00000000 &&
		UNALIGNED_GET(&addr->s6_addr32[2]) == htonl(0x00000001) &&
		((UNALIGNED_GET(&addr->s6_addr32[3]) & htonl(0xff000000)) ==
		 htonl(0xff000000));
}

/**
 * @brief Check if the IPv6 address is a given scope multicast
 * address (FFyx::).
 *
 * @param addr IPv6 address
 * @param scope Scope to check
 *
 * @return True if the address is in given scope multicast address,
 * false otherwise.
 */
static inline bool net_ipv6_is_addr_mcast_scope(const struct in6_addr *addr,
						int scope)
{
	return (addr->s6_addr[0] == 0xff) && ((addr->s6_addr[1] & 0xF) == scope);
}

/**
 * @brief Check if the IPv6 addresses have the same multicast scope (FFyx::).
 *
 * @param addr_1 IPv6 address 1
 * @param addr_2 IPv6 address 2
 *
 * @return True if both addresses have same multicast scope,
 * false otherwise.
 */
static inline bool net_ipv6_is_same_mcast_scope(const struct in6_addr *addr_1,
						const struct in6_addr *addr_2)
{
	return (addr_1->s6_addr[0] == 0xff) && (addr_2->s6_addr[0] == 0xff) &&
			(addr_1->s6_addr[1] == addr_2->s6_addr[1]);
}

/**
 * @brief Check if the IPv6 address is a global multicast address (FFxE::/16).
 *
 * @param addr IPv6 address.
 *
 * @return True if the address is global multicast address, false otherwise.
 */
static inline bool net_ipv6_is_addr_mcast_global(const struct in6_addr *addr)
{
	return net_ipv6_is_addr_mcast_scope(addr, 0x0e);
}

/**
 * @brief Check if the IPv6 address is a interface scope multicast
 * address (FFx1::).
 *
 * @param addr IPv6 address.
 *
 * @return True if the address is a interface scope multicast address,
 * false otherwise.
 */
static inline bool net_ipv6_is_addr_mcast_iface(const struct in6_addr *addr)
{
	return net_ipv6_is_addr_mcast_scope(addr, 0x01);
}

/**
 * @brief Check if the IPv6 address is a link local scope multicast
 * address (FFx2::).
 *
 * @param addr IPv6 address.
 *
 * @return True if the address is a link local scope multicast address,
 * false otherwise.
 */
static inline bool net_ipv6_is_addr_mcast_link(const struct in6_addr *addr)
{
	return net_ipv6_is_addr_mcast_scope(addr, 0x02);
}

/**
 * @brief Check if the IPv6 address is a mesh-local scope multicast
 * address (FFx3::).
 *
 * @param addr IPv6 address.
 *
 * @return True if the address is a mesh-local scope multicast address,
 * false otherwise.
 */
static inline bool net_ipv6_is_addr_mcast_mesh(const struct in6_addr *addr)
{
	return net_ipv6_is_addr_mcast_scope(addr, 0x03);
}

/**
 * @brief Check if the IPv6 address is a site scope multicast
 * address (FFx5::).
 *
 * @param addr IPv6 address.
 *
 * @return True if the address is a site scope multicast address,
 * false otherwise.
 */
static inline bool net_ipv6_is_addr_mcast_site(const struct in6_addr *addr)
{
	return net_ipv6_is_addr_mcast_scope(addr, 0x05);
}

/**
 * @brief Check if the IPv6 address is an organization scope multicast
 * address (FFx8::).
 *
 * @param addr IPv6 address.
 *
 * @return True if the address is an organization scope multicast address,
 * false otherwise.
 */
static inline bool net_ipv6_is_addr_mcast_org(const struct in6_addr *addr)
{
	return net_ipv6_is_addr_mcast_scope(addr, 0x08);
}

/**
 * @brief Check if the IPv6 address belongs to certain multicast group
 *
 * @param addr IPv6 address.
 * @param group Group id IPv6 address, the values must be in network
 * byte order
 *
 * @return True if the IPv6 multicast address belongs to given multicast
 * group, false otherwise.
 */
static inline bool net_ipv6_is_addr_mcast_group(const struct in6_addr *addr,
						const struct in6_addr *group)
{
	return UNALIGNED_GET(&addr->s6_addr16[1]) == group->s6_addr16[1] &&
		UNALIGNED_GET(&addr->s6_addr16[2]) == group->s6_addr16[2] &&
		UNALIGNED_GET(&addr->s6_addr16[3]) == group->s6_addr16[3] &&
		UNALIGNED_GET(&addr->s6_addr32[1]) == group->s6_addr32[1] &&
		UNALIGNED_GET(&addr->s6_addr32[2]) == group->s6_addr32[1] &&
		UNALIGNED_GET(&addr->s6_addr32[3]) == group->s6_addr32[3];
}

/**
 * @brief Check if the IPv6 address belongs to the all nodes multicast group
 *
 * @param addr IPv6 address
 *
 * @return True if the IPv6 multicast address belongs to the all nodes multicast
 * group, false otherwise
 */
static inline bool
net_ipv6_is_addr_mcast_all_nodes_group(const struct in6_addr *addr)
{
	static const struct in6_addr all_nodes_mcast_group = {
		{ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x00, 0x00, 0x00, 0x01 } }
	};

	return net_ipv6_is_addr_mcast_group(addr, &all_nodes_mcast_group);
}

/**
 * @brief Check if the IPv6 address is a interface scope all nodes multicast
 * address (FF01::1).
 *
 * @param addr IPv6 address.
 *
 * @return True if the address is a interface scope all nodes multicast address,
 * false otherwise.
 */
static inline bool
net_ipv6_is_addr_mcast_iface_all_nodes(const struct in6_addr *addr)
{
	return net_ipv6_is_addr_mcast_iface(addr) &&
	       net_ipv6_is_addr_mcast_all_nodes_group(addr);
}

/**
 * @brief Check if the IPv6 address is a link local scope all nodes multicast
 * address (FF02::1).
 *
 * @param addr IPv6 address.
 *
 * @return True if the address is a link local scope all nodes multicast
 * address, false otherwise.
 */
static inline bool
net_ipv6_is_addr_mcast_link_all_nodes(const struct in6_addr *addr)
{
	return net_ipv6_is_addr_mcast_link(addr) &&
	       net_ipv6_is_addr_mcast_all_nodes_group(addr);
}

/**
 *  @brief Create solicited node IPv6 multicast address
 *  FF02:0:0:0:0:1:FFXX:XXXX defined in RFC 3513
 *
 *  @param src IPv6 address.
 *  @param dst IPv6 address.
 */
static inline
void net_ipv6_addr_create_solicited_node(const struct in6_addr *src,
					 struct in6_addr *dst)
{
	dst->s6_addr[0]   = 0xFF;
	dst->s6_addr[1]   = 0x02;
	UNALIGNED_PUT(0, &dst->s6_addr16[1]);
	UNALIGNED_PUT(0, &dst->s6_addr16[2]);
	UNALIGNED_PUT(0, &dst->s6_addr16[3]);
	UNALIGNED_PUT(0, &dst->s6_addr16[4]);
	dst->s6_addr[10]  = 0U;
	dst->s6_addr[11]  = 0x01;
	dst->s6_addr[12]  = 0xFF;
	dst->s6_addr[13]  = src->s6_addr[13];
	UNALIGNED_PUT(UNALIGNED_GET(&src->s6_addr16[7]), &dst->s6_addr16[7]);
}

/** @brief Construct an IPv6 address from eight 16-bit words.
 *
 *  @param addr IPv6 address
 *  @param addr0 16-bit word which is part of the address
 *  @param addr1 16-bit word which is part of the address
 *  @param addr2 16-bit word which is part of the address
 *  @param addr3 16-bit word which is part of the address
 *  @param addr4 16-bit word which is part of the address
 *  @param addr5 16-bit word which is part of the address
 *  @param addr6 16-bit word which is part of the address
 *  @param addr7 16-bit word which is part of the address
 */
static inline void net_ipv6_addr_create(struct in6_addr *addr,
					uint16_t addr0, uint16_t addr1,
					uint16_t addr2, uint16_t addr3,
					uint16_t addr4, uint16_t addr5,
					uint16_t addr6, uint16_t addr7)
{
	UNALIGNED_PUT(htons(addr0), &addr->s6_addr16[0]);
	UNALIGNED_PUT(htons(addr1), &addr->s6_addr16[1]);
	UNALIGNED_PUT(htons(addr2), &addr->s6_addr16[2]);
	UNALIGNED_PUT(htons(addr3), &addr->s6_addr16[3]);
	UNALIGNED_PUT(htons(addr4), &addr->s6_addr16[4]);
	UNALIGNED_PUT(htons(addr5), &addr->s6_addr16[5]);
	UNALIGNED_PUT(htons(addr6), &addr->s6_addr16[6]);
	UNALIGNED_PUT(htons(addr7), &addr->s6_addr16[7]);
}

/**
 *  @brief Create link local allnodes multicast IPv6 address
 *
 *  @param addr IPv6 address
 */
static inline void net_ipv6_addr_create_ll_allnodes_mcast(struct in6_addr *addr)
{
	net_ipv6_addr_create(addr, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);
}

/**
 *  @brief Create link local allrouters multicast IPv6 address
 *
 *  @param addr IPv6 address
 */
static inline void net_ipv6_addr_create_ll_allrouters_mcast(struct in6_addr *addr)
{
	net_ipv6_addr_create(addr, 0xff02, 0, 0, 0, 0, 0, 0, 0x0002);
}

/**
 *  @brief Create IPv4 mapped IPv6 address
 *
 *  @param addr4 IPv4 address
 *  @param addr6 IPv6 address to be created
 */
static inline void net_ipv6_addr_create_v4_mapped(const struct in_addr *addr4,
						  struct in6_addr *addr6)
{
	net_ipv6_addr_create(addr6, 0, 0, 0, 0, 0, 0xffff,
			     ntohs(addr4->s4_addr16[0]),
			     ntohs(addr4->s4_addr16[1]));
}

/**
 *  @brief Is the IPv6 address an IPv4 mapped one. The v4 mapped addresses
 *         look like \::ffff:a.b.c.d
 *
 *  @param addr IPv6 address
 *
 *  @return True if IPv6 address is a IPv4 mapped address, False otherwise.
 */
static inline bool net_ipv6_addr_is_v4_mapped(const struct in6_addr *addr)
{
	if (UNALIGNED_GET(&addr->s6_addr32[0]) == 0 &&
	    UNALIGNED_GET(&addr->s6_addr32[1]) == 0 &&
	    UNALIGNED_GET(&addr->s6_addr16[5]) == 0xffff) {
		return true;
	}

	return false;
}

/**
 *  @brief Generate IPv6 address using a prefix and interface identifier.
 *         Interface identifier is either generated from EUI-64 (MAC) defined
 *         in RFC 4291 or from randomized value defined in RFC 7217.
 *
 *  @param iface Network interface
 *  @param prefix IPv6 prefix, can be left out in which case fe80::/64 is used
 *  @param network_id Network identifier (for example SSID in WLAN), this is
 *         optional can be set to NULL
 *  @param network_id_len Network identifier length, if set to 0 then the
 *         network id is ignored.
 *  @param dad_counter Duplicate Address Detection counter value, can be set to 0
 *         if it is not known.
 *  @param addr IPv6 address
 *  @param lladdr Link local address
 *
 *  @return 0 if ok, < 0 if error
 */
int net_ipv6_addr_generate_iid(struct net_if *iface,
			       const struct in6_addr *prefix,
			       uint8_t *network_id, size_t network_id_len,
			       uint8_t dad_counter,
			       struct in6_addr *addr,
			       struct net_linkaddr *lladdr);

/**
 *  @brief Create IPv6 address interface identifier.
 *
 *  @param addr IPv6 address
 *  @param lladdr Link local address
 */
static inline void net_ipv6_addr_create_iid(struct in6_addr *addr,
					    struct net_linkaddr *lladdr)
{
	(void)net_ipv6_addr_generate_iid(NULL, NULL, NULL, 0, 0, addr, lladdr);
}

/**
 *  @brief Check if given address is based on link layer address
 *
 *  @return True if it is, False otherwise
 */
static inline bool net_ipv6_addr_based_on_ll(const struct in6_addr *addr,
					     const struct net_linkaddr *lladdr)
{
	if (!addr || !lladdr) {
		return false;
	}

	switch (lladdr->len) {
	case 2:
		if (!memcmp(&addr->s6_addr[14], lladdr->addr, lladdr->len) &&
		    addr->s6_addr[8]  == 0U &&
		    addr->s6_addr[9]  == 0U &&
		    addr->s6_addr[10] == 0U &&
		    addr->s6_addr[11] == 0xff &&
		    addr->s6_addr[12] == 0xfe) {
			return true;
		}

		break;
	case 6:
		if (lladdr->type == NET_LINK_ETHERNET) {
			if (!memcmp(&addr->s6_addr[9], &lladdr->addr[1], 2) &&
			    !memcmp(&addr->s6_addr[13], &lladdr->addr[3], 3) &&
			    addr->s6_addr[11] == 0xff &&
			    addr->s6_addr[12] == 0xfe &&
			    (addr->s6_addr[8] ^ 0x02) == lladdr->addr[0]) {
				return true;
			}
		}

		break;
	case 8:
		if (!memcmp(&addr->s6_addr[9], &lladdr->addr[1],
			    lladdr->len - 1) &&
		    (addr->s6_addr[8] ^ 0x02) == lladdr->addr[0]) {
			return true;
		}

		break;
	}

	return false;
}

/**
 * @brief Get sockaddr_in6 from sockaddr. This is a helper so that
 * the code calling this function can be made shorter.
 *
 * @param addr Socket address
 *
 * @return Pointer to IPv6 socket address
 */
static inline struct sockaddr_in6 *net_sin6(const struct sockaddr *addr)
{
	return (struct sockaddr_in6 *)addr;
}

/**
 * @brief Get sockaddr_in from sockaddr. This is a helper so that
 * the code calling this function can be made shorter.
 *
 * @param addr Socket address
 *
 * @return Pointer to IPv4 socket address
 */
static inline struct sockaddr_in *net_sin(const struct sockaddr *addr)
{
	return (struct sockaddr_in *)addr;
}

/**
 * @brief Get sockaddr_in6_ptr from sockaddr_ptr. This is a helper so that
 * the code calling this function can be made shorter.
 *
 * @param addr Socket address
 *
 * @return Pointer to IPv6 socket address
 */
static inline
struct sockaddr_in6_ptr *net_sin6_ptr(const struct sockaddr_ptr *addr)
{
	return (struct sockaddr_in6_ptr *)addr;
}

/**
 * @brief Get sockaddr_in_ptr from sockaddr_ptr. This is a helper so that
 * the code calling this function can be made shorter.
 *
 * @param addr Socket address
 *
 * @return Pointer to IPv4 socket address
 */
static inline
struct sockaddr_in_ptr *net_sin_ptr(const struct sockaddr_ptr *addr)
{
	return (struct sockaddr_in_ptr *)addr;
}

/**
 * @brief Get sockaddr_ll_ptr from sockaddr_ptr. This is a helper so that
 * the code calling this function can be made shorter.
 *
 * @param addr Socket address
 *
 * @return Pointer to linklayer socket address
 */
static inline
struct sockaddr_ll_ptr *net_sll_ptr(const struct sockaddr_ptr *addr)
{
	return (struct sockaddr_ll_ptr *)addr;
}

/**
 * @brief Get sockaddr_can_ptr from sockaddr_ptr. This is a helper so that
 * the code needing this functionality can be made shorter.
 *
 * @param addr Socket address
 *
 * @return Pointer to CAN socket address
 */
static inline
struct sockaddr_can_ptr *net_can_ptr(const struct sockaddr_ptr *addr)
{
	return (struct sockaddr_can_ptr *)addr;
}

/**
 * @brief Convert a string to IP address.
 *
 * @param family IP address family (AF_INET or AF_INET6)
 * @param src IP address in a null terminated string
 * @param dst Pointer to struct in_addr if family is AF_INET or
 * pointer to struct in6_addr if family is AF_INET6
 *
 * @note This function doesn't do precise error checking,
 * do not use for untrusted strings.
 *
 * @return 0 if ok, < 0 if error
 */
__syscall int net_addr_pton(sa_family_t family, const char *src, void *dst);

/**
 * @brief Convert IP address to string form.
 *
 * @param family IP address family (AF_INET or AF_INET6)
 * @param src Pointer to struct in_addr if family is AF_INET or
 *        pointer to struct in6_addr if family is AF_INET6
 * @param dst Buffer for IP address as a null terminated string
 * @param size Number of bytes available in the buffer
 *
 * @return dst pointer if ok, NULL if error
 */
__syscall char *net_addr_ntop(sa_family_t family, const void *src,
			      char *dst, size_t size);

/**
 * @brief Parse a string that contains either IPv4 or IPv6 address
 * and optional port, and store the information in user supplied
 * sockaddr struct.
 *
 * @details Syntax of the IP address string:
 *   192.0.2.1:80
 *   192.0.2.42
 *   [2001:db8::1]:8080
 *   [2001:db8::2]
 *   2001:db::42
 * Note that the str_len parameter is used to restrict the amount of
 * characters that are checked. If the string does not contain port
 * number, then the port number in sockaddr is not modified.
 *
 * @param str String that contains the IP address.
 * @param str_len Length of the string to be parsed.
 * @param addr Pointer to user supplied struct sockaddr.
 *
 * @return True if parsing could be done, false otherwise.
 */
bool net_ipaddr_parse(const char *str, size_t str_len,
		      struct sockaddr *addr);

/**
 * @brief Set the default port in the sockaddr structure.
 * If the port is already set, then do nothing.
 *
 * @param addr Pointer to user supplied struct sockaddr.
 * @param default_port Default port number to set.
 *
 * @return 0 if ok, <0 if error
 */
int net_port_set_default(struct sockaddr *addr, uint16_t default_port);

/**
 * @brief Compare TCP sequence numbers.
 *
 * @details This function compares TCP sequence numbers,
 *          accounting for wraparound effects.
 *
 * @param seq1 First sequence number
 * @param seq2 Seconds sequence number
 *
 * @return < 0 if seq1 < seq2, 0 if seq1 == seq2, > 0 if seq > seq2
 */
static inline int32_t net_tcp_seq_cmp(uint32_t seq1, uint32_t seq2)
{
	return (int32_t)(seq1 - seq2);
}

/**
 * @brief Check that one TCP sequence number is greater.
 *
 * @details This is convenience function on top of net_tcp_seq_cmp().
 *
 * @param seq1 First sequence number
 * @param seq2 Seconds sequence number
 *
 * @return True if seq > seq2
 */
static inline bool net_tcp_seq_greater(uint32_t seq1, uint32_t seq2)
{
	return net_tcp_seq_cmp(seq1, seq2) > 0;
}

/**
 * @brief Convert a string of hex values to array of bytes.
 *
 * @details The syntax of the string is "ab:02:98:fa:42:01"
 *
 * @param buf Pointer to memory where the bytes are written.
 * @param buf_len Length of the memory area.
 * @param src String of bytes.
 *
 * @return 0 if ok, <0 if error
 */
int net_bytes_from_str(uint8_t *buf, int buf_len, const char *src);

/**
 * @brief Convert Tx network packet priority to traffic class so we can place
 * the packet into correct Tx queue.
 *
 * @param prio Network priority
 *
 * @return Tx traffic class that handles that priority network traffic.
 */
int net_tx_priority2tc(enum net_priority prio);

/**
 * @brief Convert Rx network packet priority to traffic class so we can place
 * the packet into correct Rx queue.
 *
 * @param prio Network priority
 *
 * @return Rx traffic class that handles that priority network traffic.
 */
int net_rx_priority2tc(enum net_priority prio);

/**
 * @brief Convert network packet VLAN priority to network packet priority so we
 * can place the packet into correct queue.
 *
 * @param priority VLAN priority
 *
 * @return Network priority
 */
static inline enum net_priority net_vlan2priority(uint8_t priority)
{
	/* Map according to IEEE 802.1Q */
	static const uint8_t vlan2priority[] = {
		NET_PRIORITY_BE,
		NET_PRIORITY_BK,
		NET_PRIORITY_EE,
		NET_PRIORITY_CA,
		NET_PRIORITY_VI,
		NET_PRIORITY_VO,
		NET_PRIORITY_IC,
		NET_PRIORITY_NC
	};

	if (priority >= ARRAY_SIZE(vlan2priority)) {
		/* Use Best Effort as the default priority */
		return NET_PRIORITY_BE;
	}

	return (enum net_priority)vlan2priority[priority];
}

/**
 * @brief Convert network packet priority to network packet VLAN priority.
 *
 * @param priority Packet priority
 *
 * @return VLAN priority (PCP)
 */
static inline uint8_t net_priority2vlan(enum net_priority priority)
{
	/* The conversion works both ways */
	return (uint8_t)net_vlan2priority(priority);
}

/**
 * @brief Return network address family value as a string. This is only usable
 * for debugging.
 *
 * @param family Network address family code
 *
 * @return Network address family as a string, or NULL if family is unknown.
 */
const char *net_family2str(sa_family_t family);

/**
 * @brief Add IPv6 prefix as a privacy extension filter.
 *
 * @details Note that the filters can either allow or deny listing.
 *
 * @param addr IPv6 prefix
 * @param is_denylist Tells if this filter is for allowing or denying listing.
 *
 * @return 0 if ok, <0 if error
 */
#if defined(CONFIG_NET_IPV6_PE)
int net_ipv6_pe_add_filter(struct in6_addr *addr, bool is_denylist);
#else
static inline int net_ipv6_pe_add_filter(struct in6_addr *addr,
					 bool is_denylist)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(is_denylist);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_IPV6_PE */

/**
 * @brief Delete IPv6 prefix from privacy extension filter list.
 *
 * @param addr IPv6 prefix
 *
 * @return 0 if ok, <0 if error
 */
#if defined(CONFIG_NET_IPV6_PE)
int net_ipv6_pe_del_filter(struct in6_addr *addr);
#else
static inline int net_ipv6_pe_del_filter(struct in6_addr *addr)
{
	ARG_UNUSED(addr);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_IPV6_PE */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/net_ip.h>

/**
 * @}
 */


#endif /* ZEPHYR_INCLUDE_NET_NET_IP_H_ */
