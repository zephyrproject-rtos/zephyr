/**
 * Copyright (c) 2023-2024 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRIVERS_NET_NSOS_H__
#define __DRIVERS_NET_NSOS_H__

#include <stddef.h>
#include <stdint.h>

/* Protocol families. */
#define NSOS_MID_PF_UNSPEC       0          /**< Unspecified protocol family.  */
#define NSOS_MID_PF_INET         1          /**< IP protocol family version 4. */
#define NSOS_MID_PF_INET6        2          /**< IP protocol family version 6. */

/* Address families. */
#define NSOS_MID_AF_UNSPEC      NSOS_MID_PF_UNSPEC   /**< Unspecified address family.   */
#define NSOS_MID_AF_INET        NSOS_MID_PF_INET     /**< IP protocol family version 4. */
#define NSOS_MID_AF_INET6       NSOS_MID_PF_INET6    /**< IP protocol family version 6. */

/** Protocol numbers from IANA/BSD */
enum nsos_mid_net_ip_protocol {
	NSOS_MID_IPPROTO_IP = 0,            /**< IP protocol (pseudo-val for setsockopt() */
	NSOS_MID_IPPROTO_ICMP = 1,          /**< ICMP protocol   */
	NSOS_MID_IPPROTO_IGMP = 2,          /**< IGMP protocol   */
	NSOS_MID_IPPROTO_IPIP = 4,          /**< IPIP tunnels    */
	NSOS_MID_IPPROTO_TCP = 6,           /**< TCP protocol    */
	NSOS_MID_IPPROTO_UDP = 17,          /**< UDP protocol    */
	NSOS_MID_IPPROTO_IPV6 = 41,         /**< IPv6 protocol   */
	NSOS_MID_IPPROTO_ICMPV6 = 58,       /**< ICMPv6 protocol */
	NSOS_MID_IPPROTO_RAW = 255,         /**< RAW IP packets  */
};

/** Socket type */
enum nsos_mid_net_sock_type {
	NSOS_MID_SOCK_STREAM = 1,           /**< Stream socket type   */
	NSOS_MID_SOCK_DGRAM,                /**< Datagram socket type */
	NSOS_MID_SOCK_RAW                   /**< RAW socket type      */
};

#define NSOS_MID_MSG_PEEK 0x02
#define NSOS_MID_MSG_TRUNC 0x20
#define NSOS_MID_MSG_DONTWAIT 0x40
#define NSOS_MID_MSG_WAITALL 0x100

struct nsos_mid_sockaddr {
	uint16_t sa_family;      /* Address family */
	char     sa_data[];      /* Socket address */
};

struct nsos_mid_sockaddr_in {
	uint16_t sin_family;     /* AF_INET */
	uint16_t sin_port;       /* Port number */
	uint32_t sin_addr;       /* IPv4 address */
};

struct nsos_mid_sockaddr_in6 {
	uint16_t sin6_family;    /* AF_INET6 */
	uint16_t sin6_port;      /* Port number */
	uint8_t  sin6_addr[16];
	uint32_t sin6_scope_id;  /* Set of interfaces for a scope */
};

struct nsos_mid_sockaddr_storage {
	union {
		struct nsos_mid_sockaddr_in sockaddr_in;
		struct nsos_mid_sockaddr_in6 sockaddr_in6;
	};
};

struct nsos_mid_pollfd {
	int fd;
	short events;
	short revents;
};

struct nsos_mid_addrinfo {
	int                       ai_flags;
	int                       ai_family;
	int                       ai_socktype;
	int                       ai_protocol;
	size_t                    ai_addrlen;
	struct nsos_mid_sockaddr *ai_addr;
	char                     *ai_canonname;
	struct nsos_mid_addrinfo *ai_next;
};

static inline void nsos_socket_flag_convert(int *flags_a, int flag_a,
					    int *flags_b, int flag_b)
{
	if ((*flags_a) & flag_a) {
		*flags_a &= ~flag_a;
		*flags_b |= flag_b;
	}
}

int nsos_adapt_get_errno(void);

int nsos_adapt_socket(int family, int type, int proto);

int nsos_adapt_bind(int fd, const struct nsos_mid_sockaddr *addr, size_t addrlen);
int nsos_adapt_connect(int fd, const struct nsos_mid_sockaddr *addr, size_t addrlen);
int nsos_adapt_listen(int fd, int backlog);
int nsos_adapt_accept(int fd, struct nsos_mid_sockaddr *addr, size_t *addrlen);
int nsos_adapt_sendto(int fd, const void *buf, size_t len, int flags,
		      const struct nsos_mid_sockaddr *addr, size_t addrlen);
int nsos_adapt_recvfrom(int fd, void *buf, size_t len, int flags,
			struct nsos_mid_sockaddr *addr, size_t *addrlen);


int nsos_adapt_getaddrinfo(const char *node, const char *service,
			   const struct nsos_mid_addrinfo *hints,
			   struct nsos_mid_addrinfo **res,
			   int *system_errno);
void nsos_adapt_freeaddrinfo(struct nsos_mid_addrinfo *res);

#endif /* __DRIVERS_NET_NSOS_H__ */
