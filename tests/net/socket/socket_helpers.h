/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest_assert.h>

#include <net/socket.h>

#define clear_buf(buf) memset(buf, 0, sizeof(buf))

static inline int prepare_listen_sock_udp_v4(struct sockaddr_in *addr)
{
	int ret, sock;

	ztest_not_null(addr, "null sockaddr");

	ret = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	ztest_true(ret >= 0, "socket open failed");

	sock = ret;

	ztest_equal(addr->sin_family, AF_INET, "Invalid family");

	ret = bind(sock, (struct sockaddr *)addr, sizeof(*addr));
	ztest_equal(ret, 0, "bind failed (%d/%d)", ret, errno);

	return sock;
}

static inline int prepare_listen_sock_udp_v6(struct sockaddr_in6 *addr)
{
	int ret, sock;

	ztest_not_null(addr, "null sockaddr");

	ret = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	ztest_true(ret >= 0, "socket open failed");

	sock = ret;

	ztest_equal(addr->sin6_family, AF_INET6, "Invalid family");

	ret = bind(sock, (struct sockaddr *)addr, sizeof(*addr));
	ztest_equal(ret, 0, "bind failed (%d/%d)", ret, errno);

	return sock;
}

static inline void prepare_sock_udp_v4(const char *addr, u16_t port,
				       int *sock, struct sockaddr_in *sockaddr)
{
	int rv;

	ztest_not_null(addr, "null addr");
	ztest_not_null(sock, "null sock");
	ztest_not_null(sockaddr, "null sockaddr");

	*sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	ztest_true(*sock >= 0, "socket open failed");

	sockaddr->sin_family = AF_INET;
	sockaddr->sin_port = htons(port);
	rv = inet_pton(AF_INET, addr, &sockaddr->sin_addr);
	ztest_equal(rv, 1, "inet_pton failed");
}

static inline void prepare_sock_udp_v6(const char *addr, u16_t port,
				       int *sock, struct sockaddr_in6 *sockaddr)
{
	int rv;

	ztest_not_null(addr, "null addr");
	ztest_not_null(sock, "null sock");
	ztest_not_null(sockaddr, "null sockaddr");

	*sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	ztest_true(*sock >= 0, "socket open failed");

	(void)memset(sockaddr, 0, sizeof(*sockaddr));
	sockaddr->sin6_family = AF_INET6;
	sockaddr->sin6_port = htons(port);
	rv = zsock_inet_pton(AF_INET6, addr, &sockaddr->sin6_addr);
	ztest_equal(rv, 1, "inet_pton failed");
}

static inline void prepare_sock_tcp_v4(const char *addr, u16_t port,
				       int *sock, struct sockaddr_in *sockaddr)
{
	int rv;

	ztest_not_null(addr, "null addr");
	ztest_not_null(sock, "null sock");
	ztest_not_null(sockaddr, "null sockaddr");

	*sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ztest_true(*sock >= 0, "socket open failed");

	sockaddr->sin_family = AF_INET;
	sockaddr->sin_port = htons(port);
	rv = zsock_inet_pton(AF_INET, addr, &sockaddr->sin_addr);
	ztest_equal(rv, 1, "inet_pton failed");
}

static inline void prepare_sock_tcp_v6(const char *addr, u16_t port,
				       int *sock, struct sockaddr_in6 *sockaddr)
{
	int rv;

	ztest_not_null(addr, "null addr");
	ztest_not_null(sock, "null sock");
	ztest_not_null(sockaddr, "null sockaddr");

	*sock = zsock_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	ztest_true(*sock >= 0, "socket open failed");

	sockaddr->sin6_family = AF_INET6;
	sockaddr->sin6_port = htons(port);
	rv = zsock_inet_pton(AF_INET6, addr, &sockaddr->sin6_addr);
	ztest_equal(rv, 1, "inet_pton failed");
}
