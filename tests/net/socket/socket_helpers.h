/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest_assert.h>

#include <net/socket.h>

#define clear_buf(buf) memset(buf, 0, sizeof(buf))

static inline void prepare_sock_udp_v4(const char *addr, u16_t port,
				       int *sock, struct sockaddr_in *sockaddr)
{
	int rv;

	zassert_not_null(addr, "null addr");
	zassert_not_null(sock, "null sock");
	zassert_not_null(sockaddr, "null sockaddr");

	*sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(*sock >= 0, "socket open failed");

	sockaddr->sin_family = AF_INET;
	sockaddr->sin_port = htons(port);
	rv = inet_pton(AF_INET, addr, &sockaddr->sin_addr);
	zassert_equal(rv, 1, "inet_pton failed");
}

static inline void prepare_sock_udp_v6(const char *addr, u16_t port,
				       int *sock, struct sockaddr_in6 *sockaddr)
{
	int rv;

	zassert_not_null(addr, "null addr");
	zassert_not_null(sock, "null sock");
	zassert_not_null(sockaddr, "null sockaddr");

	*sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(*sock >= 0, "socket open failed");

	(void)memset(sockaddr, 0, sizeof(*sockaddr));
	sockaddr->sin6_family = AF_INET6;
	sockaddr->sin6_port = htons(port);
	rv = inet_pton(AF_INET6, addr, &sockaddr->sin6_addr);
	zassert_equal(rv, 1, "inet_pton failed");
}

static inline void prepare_sock_tcp_v4(const char *addr, u16_t port,
				       int *sock, struct sockaddr_in *sockaddr)
{
	int rv;

	zassert_not_null(addr, "null addr");
	zassert_not_null(sock, "null sock");
	zassert_not_null(sockaddr, "null sockaddr");

	*sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(*sock >= 0, "socket open failed");

	sockaddr->sin_family = AF_INET;
	sockaddr->sin_port = htons(port);
	rv = inet_pton(AF_INET, addr, &sockaddr->sin_addr);
	zassert_equal(rv, 1, "inet_pton failed");
}

static inline void prepare_sock_tcp_v6(const char *addr, u16_t port,
				       int *sock, struct sockaddr_in6 *sockaddr)
{
	int rv;

	zassert_not_null(addr, "null addr");
	zassert_not_null(sock, "null sock");
	zassert_not_null(sockaddr, "null sockaddr");

	*sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(*sock >= 0, "socket open failed");

	sockaddr->sin6_family = AF_INET6;
	sockaddr->sin6_port = htons(port);
	rv = inet_pton(AF_INET6, addr, &sockaddr->sin6_addr);
	zassert_equal(rv, 1, "inet_pton failed");
}
