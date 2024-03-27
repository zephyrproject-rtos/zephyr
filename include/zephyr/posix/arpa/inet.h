/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_ARPA_INET_H_
#define ZEPHYR_INCLUDE_POSIX_ARPA_INET_H_

#include <stddef.h>

#include <zephyr/posix/netinet/in.h>
#include <zephyr/posix/sys/socket.h>

#include <zephyr/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_NET_SOCKETS_POSIX_NAMES

static inline char *inet_ntop(sa_family_t family, const void *src, char *dst,
			      size_t size)
{
	return zsock_inet_ntop(family, src, dst, size);
}

static inline int inet_pton(sa_family_t family, const char *src, void *dst)
{
	return zsock_inet_pton(family, src, dst);
}

#endif /* CONFIG_NET_SOCKETS_POSIX_NAMES */

typedef uint32_t in_addr_t;

in_addr_t inet_addr(const char *cp);
char *inet_ntoa(struct in_addr in);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_ARPA_INET_H_ */
