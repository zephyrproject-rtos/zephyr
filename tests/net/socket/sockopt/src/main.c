/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest_assert.h>
#include <net/socket.h>

#define TEST_STR_SMALL "test"

#define ANY_PORT 0
#define SERVER_PORT 4242

#define MAX_CONNS 5

#define TCP_TEARDOWN_TIMEOUT K_SECONDS(1)

static void prepare_tcp_sock_v6(const char *addr, u16_t port, int *sock,
				struct sockaddr_in6 *sockaddr)
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

static void prepare_udp_sock_v6(const char *addr, u16_t port, int *sock,
				struct sockaddr_in6 *sockaddr)
{
	int rv;

	zassert_not_null(addr, "null addr");
	zassert_not_null(sock, "null sock");
	zassert_not_null(sockaddr, "null sockaddr");

	*sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(*sock >= 0, "socket open failed");

	sockaddr->sin6_family = AF_INET6;
	sockaddr->sin6_port = htons(port);
	rv = inet_pton(AF_INET6, addr, &sockaddr->sin6_addr);
	zassert_equal(rv, 1, "inet_pton failed");
}

static void test_getsockopt(void)
{
	int enable;
	socklen_t size;
	int s6_tcp, s6_udp;
	struct sockaddr_in6 addr6;

	prepare_tcp_sock_v6(CONFIG_NET_APP_MY_IPV6_ADDR, ANY_PORT,
			    &s6_tcp, &addr6);

	errno = 0;
	zassert_equal(getsockopt(0, 0, 0, NULL, NULL), -1, "getsockopt");
	zassert_equal(errno, ENOTSOCK, "getsockopt errno");

	errno = 0;
	zassert_equal(getsockopt(s6_tcp, 0, 0, NULL, NULL), -1, "getsockopt");
	zassert_equal(errno, EOPNOTSUPP, "getsockopt errno");

	errno = 0;
	zassert_equal(getsockopt(s6_tcp, SOL_TLS, 0, NULL, NULL), -1,
		      "getsockopt");
	zassert_equal(errno, EFAULT, "getsockopt errno");

	prepare_udp_sock_v6(CONFIG_NET_APP_MY_IPV6_ADDR, ANY_PORT,
			    &s6_udp, &addr6);

	errno = 0;
	zassert_equal(getsockopt(s6_udp, SOL_TLS, 0, &enable, &size), -1,
		      "getsockopt");
	zassert_equal(errno, EBADF, "getsockopt errno");

	errno = 0;
	zassert_equal(getsockopt(s6_tcp, SOL_TLS, 0, &enable, NULL), -1,
		      "getsockopt");
	zassert_equal(errno, EFAULT, "getsockopt errno");

	errno = 0;
	zassert_equal(getsockopt(s6_tcp, SOL_TLS, 0, NULL, &size), -1,
		      "getsockopt");
	zassert_equal(errno, EFAULT, "getsockopt errno");

	errno = 0;
	zassert_equal(getsockopt(s6_tcp, SOL_TLS, 0, &enable, NULL), -1,
		      "getsockopt");
	zassert_equal(errno, EFAULT, "getsockopt errno");

	errno = 0;
	size = 0;
	zassert_equal(getsockopt(s6_tcp, SOL_TLS, 0, &enable, &size), -1,
		      "getsockopt");
	zassert_equal(errno, EFAULT, "getsockopt errno");

	size = 0;
	errno = 0;
	zassert_equal(getsockopt(s6_tcp, SOL_TLS, 0, &enable, &size), -1,
		      "getsockopt");
	zassert_equal(errno, EFAULT, "getsockopt errno");

	size = sizeof(enable);
	errno = 0;
	zassert_equal(getsockopt(s6_tcp, SOL_TLS, 0, &enable, &size),
		      -1, "getsockopt");
	zassert_equal(errno, ENOPROTOOPT, "getsockopt errno");
	zassert_equal(size, sizeof(enable), "getsockopt errno");

	errno = 0;
	zassert_equal(getsockopt(s6_tcp, SOL_TLS, TLS_ENABLE, &enable, &size),
		      0, "getsockopt");
	zassert_equal(errno, 0, "getsockopt errno");
	zassert_equal(size, sizeof(enable), "getsockopt errno");

	zassert_equal(close(s6_tcp), 0, "close failed");
	zassert_equal(close(s6_udp), 0, "close failed");
}

static void test_setsockopt(void)
{
	int enable = 1;
	int s6_tcp, s6_udp;
	struct sockaddr_in6 addr6;

	prepare_tcp_sock_v6(CONFIG_NET_APP_MY_IPV6_ADDR, ANY_PORT,
			    &s6_tcp, &addr6);

	zassert_equal(setsockopt(0, 0, 0, NULL, 0), -1, "setsockopt");
	zassert_equal(errno, ENOTSOCK, "setsockopt errno");

	errno = 0;
	zassert_equal(setsockopt(s6_tcp, 0, 0, NULL, 0), -1, "setsockopt");
	zassert_equal(errno, EOPNOTSUPP, "setsockopt errno");

	errno = 0;
	zassert_equal(setsockopt(s6_tcp, SOL_TLS, 0, NULL, 0), -1,
		      "setsockopt");
	zassert_equal(errno, EFAULT, "setsockopt errno");

	errno = 0;
	zassert_equal(setsockopt(s6_tcp, SOL_TLS, 0, &enable, 0), -1,
		      "setsockopt");
	zassert_equal(errno, EFAULT, "setsockopt errno");

	errno = 0;
	zassert_equal(setsockopt(s6_tcp, SOL_TLS, 0, &enable, sizeof(enable)),
		      -1, "setsockopt");
	zassert_equal(errno, ENOPROTOOPT, "setsockopt errno");

	prepare_udp_sock_v6(CONFIG_NET_APP_MY_IPV6_ADDR, ANY_PORT,
			    &s6_udp, &addr6);

	errno = 0;
	zassert_equal(setsockopt(s6_udp, SOL_TLS, 0, &enable, sizeof(enable)),
		      -1, "setsockopt");
	zassert_equal(errno, EBADF, "setsockopt errno");

	errno = 0;
	zassert_equal(setsockopt(s6_tcp, SOL_TLS, TLS_ENABLE,
				 &enable, sizeof(enable)), 0, "setsockopt");
	zassert_equal(errno, 0, "setsockopt errno");

	zassert_equal(close(s6_tcp), 0, "close failed");
	zassert_equal(close(s6_udp), 0, "close failed");
}

void test_main(void)
{
	ztest_test_suite(socket_sockopt,
			 ztest_unit_test(test_getsockopt),
			 ztest_unit_test(test_setsockopt));

	ztest_run_test_suite(socket_sockopt);
}
