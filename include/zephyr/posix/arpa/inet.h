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

typedef uint32_t in_addr_t;

in_addr_t inet_addr(const char *cp);
char *inet_ntoa(struct in_addr in);
char *inet_ntop(sa_family_t family, const void *src, char *dst, size_t size);
int inet_pton(sa_family_t family, const char *src, void *dst);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_ARPA_INET_H_ */
