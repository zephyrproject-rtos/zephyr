/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_NETDB_H_
#define ZEPHYR_INCLUDE_POSIX_NETDB_H_

#include <net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_NETDB_H_ */
