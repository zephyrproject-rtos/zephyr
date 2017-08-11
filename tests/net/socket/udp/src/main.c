/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <ztest_assert.h>

#include <net/socket.h>

#define BUF_AND_SIZE(buf) buf, sizeof(buf) - 1
#define STRLEN(buf) (sizeof(buf) - 1)

#define TEST_STR_SMALL "test"

#define LOCAL_PORT 9898
#define REMOTE_PORT 4242

#define V4_ANY_ADDR "0.0.0.0"
#define V6_ANY_ADDR "0:0:0:0:0:0:0:0"

#define V4_REMOTE_ADDR "192.0.2.2"
#define V6_REMOTE_ADDR "2001:db8::2"

static void test_v4_sendto_recvfrom(void)
{
	int rv;
	int sock;
	ssize_t sent = 0;
	ssize_t recved = 0;
	char rx_buf[30] = {0};
	struct sockaddr_in addr;
	socklen_t socklen;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "socket open failed");

	addr.sin_family = AF_INET;
	addr.sin_port = htons(REMOTE_PORT);
	rv = inet_pton(AF_INET, V4_REMOTE_ADDR, &(addr.sin_addr));
	zassert_equal(rv, 1, "inet_pton failed");

	sent = sendto(sock,
		      TEST_STR_SMALL,
		      strlen(TEST_STR_SMALL),
		      0,
		      (struct sockaddr *)&addr,
		      sizeof(addr));
	zassert_equal(sent, strlen(TEST_STR_SMALL), "sendto failed");

	socklen = sizeof(addr);
	recved = recvfrom(sock,
			  rx_buf,
			  sizeof(rx_buf),
			  0,
			  (struct sockaddr *)&addr,
			  &socklen);
	zassert_true(recved > 0, "recvfrom fail");
	zassert_equal(recved,
		      strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_equal(strncmp(rx_buf, TEST_STR_SMALL, strlen(TEST_STR_SMALL)),
		      0,
		      "unexpected data");
}

static void test_v6_sendto_recvfrom(void)
{
	int rv;
	int sock;
	ssize_t sent = 0;
	ssize_t recved = 0;
	char rx_buf[30] = {0};
	struct sockaddr_in6 addr;
	socklen_t socklen;

	sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "socket open failed");

	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(REMOTE_PORT);
	rv = inet_pton(AF_INET6, V6_REMOTE_ADDR, &(addr.sin6_addr));
	zassert_equal(rv, 1, "inet_pton failed");

	sent = sendto(sock,
		      TEST_STR_SMALL,
		      strlen(TEST_STR_SMALL),
		      0,
		      (struct sockaddr *)&addr,
		      sizeof(addr));
	zassert_equal(sent, strlen(TEST_STR_SMALL), "sendto failed");

	socklen = sizeof(addr);
	recved = recvfrom(sock,
			  rx_buf,
			  sizeof(rx_buf),
			  0,
			  (struct sockaddr *)&addr,
			  &socklen);
	zassert_true(recved > 0, "recvfrom fail");
	zassert_equal(recved,
		      strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_equal(strncmp(rx_buf, TEST_STR_SMALL, strlen(TEST_STR_SMALL)),
		      0,
		      "unexpected data");
}

static void test_v4_bind_sendto(void)
{
	int rv;
	int sock;
	ssize_t sent = 0;
	ssize_t recved = 0;
	char rx_buf[30] = {0};
	struct sockaddr_in remote_addr;
	struct sockaddr_in local_addr;
	socklen_t socklen;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "socket open failed");

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(LOCAL_PORT);
	rv = inet_pton(AF_INET, V4_ANY_ADDR, &(local_addr.sin_addr));
	zassert_equal(rv, 1, "inet_pton failed");

	rv = bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr));
	zassert_equal(rv, 0, "bind failed");

	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(REMOTE_PORT);
	rv = inet_pton(AF_INET, V4_REMOTE_ADDR, &(remote_addr.sin_addr));
	zassert_equal(rv, 1, "inet_pton failed");

	sent = sendto(sock,
		      TEST_STR_SMALL,
		      strlen(TEST_STR_SMALL),
		      0,
		      (struct sockaddr *)&remote_addr,
		      sizeof(remote_addr));
	zassert_equal(sent, strlen(TEST_STR_SMALL), "sendto failed");

	socklen = sizeof(remote_addr);
	recved = recvfrom(sock,
			  rx_buf,
			  sizeof(rx_buf),
			  0,
			  (struct sockaddr *)&remote_addr,
			  &socklen);
	zassert_true(recved > 0, "recvfrom fail");
	zassert_equal(recved,
		      strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_equal(strncmp(rx_buf, TEST_STR_SMALL, strlen(TEST_STR_SMALL)),
		      0,
		      "unexpected data");
}

static void test_v6_bind_sendto(void)
{
	int rv;
	int sock;
	ssize_t sent = 0;
	ssize_t recved = 0;
	char rx_buf[30] = {0};
	struct sockaddr_in6 remote_addr;
	struct sockaddr_in6 local_addr;
	socklen_t socklen;

	sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "socket open failed");

	local_addr.sin6_family = AF_INET6;
	local_addr.sin6_port = htons(LOCAL_PORT);
	rv = inet_pton(AF_INET6, V6_ANY_ADDR, &(local_addr.sin6_addr));
	zassert_equal(rv, 1, "inet_pton failed");

	rv = bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr));
	zassert_equal(rv, 0, "bind failed");

	remote_addr.sin6_family = AF_INET6;
	remote_addr.sin6_port = htons(REMOTE_PORT);
	rv = inet_pton(AF_INET6, V6_REMOTE_ADDR, &(remote_addr.sin6_addr));
	zassert_equal(rv, 1, "inet_pton failed");

	sent = sendto(sock,
		      TEST_STR_SMALL,
		      strlen(TEST_STR_SMALL),
		      0,
		      (struct sockaddr *)&remote_addr,
		      sizeof(remote_addr));
	zassert_equal(sent, strlen(TEST_STR_SMALL), "sendto failed");

	socklen = sizeof(remote_addr);
	recved = recvfrom(sock,
			  rx_buf,
			  sizeof(rx_buf),
			  0,
			  (struct sockaddr *)&remote_addr,
			  &socklen);
	zassert_true(recved > 0, "recvfrom fail");
	zassert_equal(recved,
		      strlen(TEST_STR_SMALL),
		      "unexpected received bytes");
	zassert_equal(strncmp(rx_buf, TEST_STR_SMALL, strlen(TEST_STR_SMALL)),
		      0,
		      "unexpected data");
}

void test_send_recv_2_sock(void)
{
	int sock1, sock2;
	struct sockaddr_in bind_addr, conn_addr;
	char buf[10];
	int len, cmp;

	sock1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sock2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind_addr.sin_port = htons(55555);
	bind(sock1, (struct sockaddr *)&bind_addr, sizeof(bind_addr));

	conn_addr.sin_family = AF_INET;
	conn_addr.sin_addr.s_addr = htonl(0xc0000201);
	conn_addr.sin_port = htons(55555);
	connect(sock2, (struct sockaddr *)&conn_addr, sizeof(conn_addr));

	send(sock2, BUF_AND_SIZE(TEST_STR_SMALL), 0);

	len = recv(sock1, buf, sizeof(buf), 0);
	zassert_equal(len, 4, "Invalid recv len");
	cmp = memcmp(buf, TEST_STR_SMALL, STRLEN(TEST_STR_SMALL));
	zassert_equal(cmp, 0, "Invalid recv data");
}

void test_main(void)
{
	ztest_test_suite(socket_udp,
			 ztest_unit_test(test_send_recv_2_sock),
			 ztest_unit_test(test_v4_sendto_recvfrom),
			 ztest_unit_test(test_v6_sendto_recvfrom),
			 ztest_unit_test(test_v4_bind_sendto),
			 ztest_unit_test(test_v6_bind_sendto));

	ztest_run_test_suite(socket_udp);
}
