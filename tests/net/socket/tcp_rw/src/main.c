/*
 * Copyright (c) 2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <ztest_assert.h>
#include <fcntl.h>
#include <net/socket.h>

#ifdef CONFIG_POSIX_API
#include <unistd.h>
#endif

#include "../../socket_helpers.h"

#define TEST_STR_SMALL "test"

#define ANY_PORT 0
#define SERVER_PORT 4242

#define MAX_CONNS 5

#define TCP_TEARDOWN_TIMEOUT K_SECONDS(1)

static void test_bind(int sock, struct sockaddr *addr, socklen_t addrlen)
{
	zassert_equal(bind(sock, addr, addrlen),
		      0,
		      "bind failed");
}

static void test_listen(int sock)
{
	zassert_equal(listen(sock, MAX_CONNS),
		      0,
		      "listen failed");
}

static void test_connect(int sock, struct sockaddr *addr, socklen_t addrlen)
{
	zassert_equal(connect(sock, addr, addrlen),
		      0,
		      "connect failed");
}


static void test_accept(int sock, int *new_sock, struct sockaddr *addr,
			socklen_t *addrlen)
{
	zassert_not_null(new_sock, "null newsock");

	*new_sock = accept(sock, addr, addrlen);
	zassert_true(*new_sock >= 0, "accept failed");
}

#ifdef CONFIG_POSIX_API
static void test_write(int sock, const void *buf, size_t len)
{
	zassert_equal(write(sock, buf, len),
		      len,
		      "write failed");
}

static void test_read(int sock)
{
	ssize_t recved = 0;
	char rx_buf[30] = {0};

	recved = read(sock, rx_buf, sizeof(rx_buf));
	zassert_equal(recved,
		      strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_equal(strncmp(rx_buf, TEST_STR_SMALL, strlen(TEST_STR_SMALL)),
		      0,
		      "unexpected data");
}
#endif

static void test_close(int sock)
{
	zassert_equal(close(sock),
		      0,
		      "close failed");
}

/* Test that EOF handling works correctly. Should be called with socket
 * whose peer socket was closed.
 */
static void test_eof(int sock)
{
	char rx_buf[1];
	ssize_t recved;

	/* Test that EOF properly detected. */
	recved = recv(sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(recved, 0, "");

	/* Calling again should be OK. */
	recved = recv(sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(recved, 0, "");

	/* Calling when TCP connection is fully torn down should be still OK. */
	k_sleep(TCP_TEARDOWN_TIMEOUT);
	recved = recv(sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(recved, 0, "");
}

/* These tests require POSIX API. */
#ifdef CONFIG_POSIX_API

void test_v4_write_read(void)
{
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v4(CONFIG_NET_CONFIG_MY_IPV4_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_write(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	test_read(new_sock);

	test_close(c_sock);
	test_eof(new_sock);

	test_close(new_sock);
	test_close(s_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_v6_write_read(void)
{
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr;
	struct sockaddr_in6 s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, ANY_PORT,
			    &c_sock, &c_saddr);
	prepare_sock_tcp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, SERVER_PORT,
			    &s_sock, &s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_write(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	test_read(new_sock);

	test_close(c_sock);
	test_eof(new_sock);

	test_close(new_sock);
	test_close(s_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

#else /* CONFIG_POSIX_API */

void test_v4_write_read(void)
{
	ztest_test_skip();
}

void test_v6_write_read(void)
{
	ztest_test_skip();
}

#endif /* CONFIG_POSIX_API */

void test_main(void)
{
	ztest_test_suite(
		socket_tcp_read_write,
		ztest_user_unit_test(test_v4_write_read),
		ztest_user_unit_test(test_v6_write_read)
		);

	ztest_run_test_suite(socket_tcp_read_write);
}
