/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_NETDB_H_
#define ZEPHYR_INCLUDE_POSIX_NETDB_H_

#include <zephyr/net/socket.h>

#ifndef NI_MAXSERV
/** Provide a reasonable size for apps using getnameinfo */
#define NI_MAXSERV 32
#endif

#define EAI_BADFLAGS DNS_EAI_BADFLAGS
#define EAI_NONAME DNS_EAI_NONAME
#define EAI_AGAIN DNS_EAI_AGAIN
#define EAI_FAIL DNS_EAI_FAIL
#define EAI_NODATA DNS_EAI_NODATA
#define EAI_MEMORY DNS_EAI_MEMORY
#define EAI_SYSTEM DNS_EAI_SYSTEM
#define EAI_SERVICE DNS_EAI_SERVICE
#define EAI_SOCKTYPE DNS_EAI_SOCKTYPE
#define EAI_FAMILY DNS_EAI_FAMILY
#define EAI_OVERFLOW DNS_EAI_OVERFLOW

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_NET_SOCKETS_POSIX_NAMES

#define addrinfo zsock_addrinfo

static inline int getaddrinfo(const char *host, const char *service,
			      const struct zsock_addrinfo *hints,
			      struct zsock_addrinfo **res)
{
	return zsock_getaddrinfo(host, service, hints, res);
}

static inline void freeaddrinfo(struct zsock_addrinfo *ai)
{
	zsock_freeaddrinfo(ai);
}

static inline const char *gai_strerror(int errcode)
{
	return zsock_gai_strerror(errcode);
}

static inline int getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
			      char *host, socklen_t hostlen,
			      char *serv, socklen_t servlen, int flags)
{
	return zsock_getnameinfo(addr, addrlen, host, hostlen,
				 serv, servlen, flags);
}

#endif /* CONFIG_NET_SOCKETS_POSIX_NAMES */

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_NETDB_H_ */
