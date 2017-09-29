/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <ztest_assert.h>

#include <net/socket.h>
#include <net/net_if.h>

#define BUF_AND_SIZE(buf) buf, sizeof(buf) - 1
#define STRLEN(buf) (sizeof(buf) - 1)

#define TEST_STR_SMALL "test"

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
	zassert_not_null(net_if_get_default(), "No default netif");
	static struct in_addr in4addr_my = { { {192, 0, 2, 1} } };

	net_if_ipv4_addr_add(net_if_get_default(), &in4addr_my,
			     NET_ADDR_MANUAL, 0);

	ztest_test_suite(socket_udp,
		ztest_unit_test(test_send_recv_2_sock)
	);

	ztest_run_test_suite(socket_udp);
}
