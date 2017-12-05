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

static void prepare_sock_v4(const char *addr,
			    u16_t port,
			    int *sock,
			    struct sockaddr_in *sockaddr)
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

static void prepare_sock_v6(const char *addr,
			    u16_t port,
			    int *sock,
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

static void test_send(int sock, const void *buf, size_t len, int flags)
{
	zassert_equal(send(sock, buf, len, flags),
		      len,
		      "send failed");
}

static void test_sendto(int sock, const void *buf, size_t len, int flags,
			const struct sockaddr *addr, socklen_t addrlen)
{
	zassert_equal(sendto(sock, buf, len, flags, addr, addrlen),
		      len,
		      "send failed");
}

static void test_accept(int sock, int *new_sock, struct sockaddr *addr,
			socklen_t *addrlen)
{
	zassert_not_null(new_sock, "null newsock");

	*new_sock = accept(sock, addr, addrlen);
	zassert_true(*new_sock >= 0, "accept failed");
}

static void test_recv(int sock)
{
	ssize_t recved = 0;
	char rx_buf[30] = {0};

	recved = recv(sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(recved,
		      strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_equal(strncmp(rx_buf, TEST_STR_SMALL, strlen(TEST_STR_SMALL)),
		      0,
		      "unexpected data");
}

static void test_recvfrom(int sock,
			  struct sockaddr *addr,
			  socklen_t *addrlen)
{
	ssize_t recved = 0;
	char rx_buf[30] = {0};

	recved = recvfrom(sock,
			  rx_buf,
			  sizeof(rx_buf),
			  0,
			  addr,
			  addrlen);
	zassert_equal(recved,
		      strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_equal(strncmp(rx_buf, TEST_STR_SMALL, strlen(TEST_STR_SMALL)),
		      0,
		      "unexpected data");
}

static void test_close(int sock)
{
	zassert_equal(close(sock),
		      0,
		      "close failed");
}

void test_v4_send_recv(void)
{
	/* Test if send() and recv() work on a ipv4 stream socket. */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_v4(CONFIG_NET_APP_MY_IPV4_ADDR,
			ANY_PORT,
			&c_sock,
			&c_saddr);

	prepare_sock_v4(CONFIG_NET_APP_MY_IPV4_ADDR,
			SERVER_PORT,
			&s_sock,
			&s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_send(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0);

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	test_recv(new_sock);

	test_close(new_sock);
	test_close(c_sock);
	test_close(s_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_v6_send_recv(void)
{
	/* Test if send() and recv() work on a ipv6 stream socket. */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr;
	struct sockaddr_in6 s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_v6(CONFIG_NET_APP_MY_IPV6_ADDR,
			ANY_PORT,
			&c_sock,
			&c_saddr);

	prepare_sock_v6(CONFIG_NET_APP_MY_IPV6_ADDR,
			SERVER_PORT,
			&s_sock,
			&s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_send(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0);

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	test_recv(new_sock);

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_v4_sendto_recvfrom(void)
{
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_v4(CONFIG_NET_APP_MY_IPV4_ADDR,
			ANY_PORT,
			&c_sock,
			&c_saddr);

	prepare_sock_v4(CONFIG_NET_APP_MY_IPV4_ADDR,
			SERVER_PORT,
			&s_sock,
			&s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_sendto(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0,
		    (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	test_recvfrom(new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_v6_sendto_recvfrom(void)
{
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr;
	struct sockaddr_in6 s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_v6(CONFIG_NET_APP_MY_IPV6_ADDR,
			ANY_PORT,
			&c_sock,
			&c_saddr);

	prepare_sock_v6(CONFIG_NET_APP_MY_IPV6_ADDR,
			SERVER_PORT,
			&s_sock,
			&s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_sendto(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0,
		    (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	test_recvfrom(new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_v4_sendto_recvfrom_null_dest(void)
{
	/* For a stream socket, sendto() should ignore NULL dest address */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in c_saddr;
	struct sockaddr_in s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_v4(CONFIG_NET_APP_MY_IPV4_ADDR,
			ANY_PORT,
			&c_sock,
			&c_saddr);

	prepare_sock_v4(CONFIG_NET_APP_MY_IPV4_ADDR,
			SERVER_PORT,
			&s_sock,
			&s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_sendto(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0,
		    (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in), "wrong addrlen");

	test_recvfrom(new_sock, NULL, NULL);

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_v6_sendto_recvfrom_null_dest(void)
{
	/* For a stream socket, sendto() should ignore NULL dest address */
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_saddr;
	struct sockaddr_in6 s_saddr;
	struct sockaddr addr;
	socklen_t addrlen = sizeof(addr);

	prepare_sock_v6(CONFIG_NET_APP_MY_IPV6_ADDR,
			ANY_PORT,
			&c_sock,
			&c_saddr);

	prepare_sock_v6(CONFIG_NET_APP_MY_IPV6_ADDR,
			SERVER_PORT,
			&s_sock,
			&s_saddr);

	test_bind(s_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_listen(s_sock);

	test_connect(c_sock, (struct sockaddr *)&s_saddr, sizeof(s_saddr));
	test_sendto(c_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL), 0,
		    (struct sockaddr *)&s_saddr, sizeof(s_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct sockaddr_in6), "wrong addrlen");

	test_recvfrom(new_sock, NULL, NULL);

	test_close(new_sock);
	test_close(s_sock);
	test_close(c_sock);

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

void test_main(void)
{
	ztest_test_suite(socket_tcp,
			 ztest_unit_test(test_v4_send_recv),
			 ztest_unit_test(test_v6_send_recv),
			 ztest_unit_test(test_v4_sendto_recvfrom),
			 ztest_unit_test(test_v6_sendto_recvfrom),
			 ztest_unit_test(test_v4_sendto_recvfrom_null_dest),
			 ztest_unit_test(test_v6_sendto_recvfrom_null_dest));

	ztest_run_test_suite(socket_tcp);
}
