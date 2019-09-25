/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <ztest_assert.h>

#include <net/socket.h>

#include "../../socket_helpers.h"

#define BUF_AND_SIZE(buf) buf, sizeof(buf) - 1
#define STRLEN(buf) (sizeof(buf) - 1)

#define TEST_STR_SMALL "test"

#define ANY_PORT 0
#define SERVER_PORT 4242
#define CLIENT_PORT 9898

/* On QEMU, poll() which waits takes +10ms from the requested time. */
#define FUZZ 10

void test_poll(void)
{
	int res;
	int c_sock;
	int s_sock;
	struct sockaddr_in6 c_addr;
	struct sockaddr_in6 s_addr;
	struct pollfd pollfds[2];
	u32_t tstamp;
	ssize_t len;
	char buf[10];

	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, CLIENT_PORT,
			    &c_sock, &c_addr);
	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, SERVER_PORT,
			    &s_sock, &s_addr);

	res = bind(s_sock, (struct sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(res, 0, "bind failed");

	res = connect(c_sock, (struct sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(res, 0, "connect failed");

	memset(pollfds, 0, sizeof(pollfds));
	pollfds[0].fd = c_sock;
	pollfds[0].events = POLLIN;
	pollfds[1].fd = s_sock;
	pollfds[1].events = POLLIN;

	/* Poll non-ready fd's with timeout of 0 */
	tstamp = k_uptime_get_32();
	res = poll(pollfds, ARRAY_SIZE(pollfds), 0);
	zassert_true(k_uptime_get_32() - tstamp <= FUZZ, "");
	zassert_equal(res, 0, "");

	zassert_equal(pollfds[0].fd, c_sock, "");
	zassert_equal(pollfds[0].events, POLLIN, "");
	zassert_equal(pollfds[0].revents, 0, "");
	zassert_equal(pollfds[1].fd, s_sock, "");
	zassert_equal(pollfds[1].events, POLLIN, "");
	zassert_equal(pollfds[1].revents, 0, "");


	/* Poll non-ready fd's with timeout of 30 */
	tstamp = k_uptime_get_32();
	res = poll(pollfds, ARRAY_SIZE(pollfds), 30);
	tstamp = k_uptime_get_32() - tstamp;
	zassert_true(tstamp >= 30U && tstamp <= 30 + FUZZ * 2, "tstamp %d",
		     tstamp);
	zassert_equal(res, 0, "");


	/* Send pkt for s_sock and poll with timeout of 10 */
	len = send(c_sock, BUF_AND_SIZE(TEST_STR_SMALL), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

	tstamp = k_uptime_get_32();
	res = poll(pollfds, ARRAY_SIZE(pollfds), 30);
	tstamp = k_uptime_get_32() - tstamp;
	zassert_true(tstamp <= FUZZ, "");
	zassert_equal(res, 1, "");

	zassert_equal(pollfds[0].fd, c_sock, "");
	zassert_equal(pollfds[0].events, POLLIN, "");
	zassert_equal(pollfds[0].revents, 0, "");
	zassert_equal(pollfds[1].fd, s_sock, "");
	zassert_equal(pollfds[1].events, POLLIN, "");
	zassert_equal(pollfds[1].revents, POLLIN, "");


	/* Recv pkt from s_sock and ensure no poll events happen */
	len = recv(s_sock, BUF_AND_SIZE(buf), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid recv len");

	tstamp = k_uptime_get_32();
	res = poll(pollfds, ARRAY_SIZE(pollfds), 0);
	zassert_true(k_uptime_get_32() - tstamp <= FUZZ, "");
	zassert_equal(res, 0, "");
	zassert_equal(pollfds[1].revents, 0, "");


	/* Close one socket and ensure POLLNVAL happens */
	res = close(c_sock);
	zassert_equal(res, 0, "close failed");

	tstamp = k_uptime_get_32();
	res = poll(pollfds, ARRAY_SIZE(pollfds), 0);
	zassert_true(k_uptime_get_32() - tstamp <= FUZZ, "");
	zassert_equal(res, 1, "");
	zassert_equal(pollfds[0].revents, POLLNVAL, "");
	zassert_equal(pollfds[1].revents, 0, "");


	res = close(s_sock);
	zassert_equal(res, 0, "close failed");
}

void test_main(void)
{
	ztest_test_suite(socket_poll,
			 ztest_unit_test(test_poll));

	ztest_run_test_suite(socket_poll);
}
