/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <zephyr/ztest_assert.h>

#include <zephyr/net/socket.h>
#include <zephyr/sys/fdtable.h>

#include "../../socket_helpers.h"

#define BUF_AND_SIZE(buf) buf, sizeof(buf) - 1
#define STRLEN(buf) (sizeof(buf) - 1)

#define TEST_STR_SMALL "test"

#define MY_IPV6_ADDR "::1"

#define ANY_PORT 0
#define SERVER_PORT 4242
#define CLIENT_PORT 9898

/* On QEMU, poll() which waits takes +10ms from the requested time. */
#define FUZZ 10

#define TCP_TEARDOWN_TIMEOUT K_SECONDS(3)

ZTEST(net_socket_poll, test_poll)
{
	int res;
	int c_sock;
	int s_sock;
	int c_sock_tcp;
	int s_sock_tcp;
	struct sockaddr_in6 c_addr;
	struct sockaddr_in6 s_addr;
	struct zsock_pollfd pollfds[2];
	struct zsock_pollfd pollout[1];
	uint32_t tstamp;
	ssize_t len;
	char buf[10];

	prepare_sock_udp_v6(MY_IPV6_ADDR, CLIENT_PORT, &c_sock, &c_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_addr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, CLIENT_PORT, &c_sock_tcp, &c_addr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock_tcp, &s_addr);

	res = zsock_bind(s_sock, (struct sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(res, 0, "bind failed");

	res = zsock_connect(c_sock, (struct sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(res, 0, "connect failed");

	memset(pollfds, 0, sizeof(pollfds));
	pollfds[0].fd = c_sock;
	pollfds[0].events = ZSOCK_POLLIN;
	pollfds[1].fd = s_sock;
	pollfds[1].events = ZSOCK_POLLIN;

	/* Poll non-ready fd's with timeout of 0 */
	tstamp = k_uptime_get_32();
	res = zsock_poll(pollfds, ARRAY_SIZE(pollfds), 0);
	zassert_true(k_uptime_get_32() - tstamp <= FUZZ, "");
	zassert_equal(res, 0, "");

	zassert_equal(pollfds[0].fd, c_sock, "");
	zassert_equal(pollfds[0].events, ZSOCK_POLLIN, "");
	zassert_equal(pollfds[0].revents, 0, "");
	zassert_equal(pollfds[1].fd, s_sock, "");
	zassert_equal(pollfds[1].events, ZSOCK_POLLIN, "");
	zassert_equal(pollfds[1].revents, 0, "");


	/* Poll non-ready fd's with timeout of 30 */
	tstamp = k_uptime_get_32();
	res = zsock_poll(pollfds, ARRAY_SIZE(pollfds), 30);
	tstamp = k_uptime_get_32() - tstamp;
	zassert_true(tstamp >= 30U && tstamp <= 30 + FUZZ * 2, "tstamp %d",
		     tstamp);
	zassert_equal(res, 0, "");

	/* Send pkt for s_sock and poll with timeout of 10 */
	len = zsock_send(c_sock, BUF_AND_SIZE(TEST_STR_SMALL), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

	tstamp = k_uptime_get_32();
	res = zsock_poll(pollfds, ARRAY_SIZE(pollfds), 30);
	tstamp = k_uptime_get_32() - tstamp;
	zassert_true(tstamp <= FUZZ, "");
	zassert_equal(res, 1, "");

	zassert_equal(pollfds[0].fd, c_sock, "");
	zassert_equal(pollfds[0].events, ZSOCK_POLLIN, "");
	zassert_equal(pollfds[0].revents, 0, "");
	zassert_equal(pollfds[1].fd, s_sock, "");
	zassert_equal(pollfds[1].events, ZSOCK_POLLIN, "");
	zassert_equal(pollfds[1].revents, ZSOCK_POLLIN, "");


	/* Recv pkt from s_sock and ensure no poll events happen */
	len = zsock_recv(s_sock, BUF_AND_SIZE(buf), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid recv len");

	tstamp = k_uptime_get_32();
	res = zsock_poll(pollfds, ARRAY_SIZE(pollfds), 0);
	zassert_true(k_uptime_get_32() - tstamp <= FUZZ, "");
	zassert_equal(res, 0, "");
	zassert_equal(pollfds[1].revents, 0, "");

	/* Make sure that POLLOUT does not wait if not really needed */
	memset(pollout, 0, sizeof(pollout));
	pollout[0].fd = c_sock;
	pollout[0].events = ZSOCK_POLLOUT;

	res = zsock_connect(c_sock, (const struct sockaddr *)&s_addr,
			    sizeof(s_addr));
	zassert_equal(res, 0, "");

	tstamp = k_uptime_get_32();
	res = zsock_poll(pollout, ARRAY_SIZE(pollout), 200);
	zassert_true(k_uptime_get_32() - tstamp < 100, "");
	zassert_equal(res, 1, "");
	zassert_equal(pollout[0].revents, ZSOCK_POLLOUT, "");

	/* First test that TCP POLLOUT will not wait if there is enough
	 * room in TCP window
	 */
	memset(pollout, 0, sizeof(pollout));
	pollout[0].fd = c_sock_tcp;
	pollout[0].events = ZSOCK_POLLOUT;

	res = zsock_bind(s_sock_tcp, (struct sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(res, 0, "");
	res = zsock_listen(s_sock_tcp, 0);
	zassert_equal(res, 0, "");

	res = zsock_connect(c_sock_tcp, (const struct sockaddr *)&s_addr,
			    sizeof(s_addr));
	zassert_equal(res, 0, "");

	tstamp = k_uptime_get_32();
	res = zsock_poll(pollout, ARRAY_SIZE(pollout), 200);
	zassert_true(k_uptime_get_32() - tstamp < 100, "");
	zassert_equal(res, 1, "");
	zassert_equal(pollout[0].revents, ZSOCK_POLLOUT, "");

	res = zsock_close(c_sock_tcp);
	zassert_equal(res, 0, "close failed");

	res = zsock_close(s_sock_tcp);
	zassert_equal(res, 0, "close failed");

	/* Close one socket and ensure POLLNVAL happens */
	res = zsock_close(c_sock);
	zassert_equal(res, 0, "close failed");

	tstamp = k_uptime_get_32();
	res = zsock_poll(pollfds, ARRAY_SIZE(pollfds), 0);
	zassert_true(k_uptime_get_32() - tstamp <= FUZZ, "");
	zassert_equal(res, 1, "");
	zassert_equal(pollfds[0].revents, ZSOCK_POLLNVAL, "");
	zassert_equal(pollfds[1].revents, 0, "");

	res = zsock_close(s_sock);
	zassert_equal(res, 0, "close failed");

	k_sleep(TCP_TEARDOWN_TIMEOUT);
}

#define TEST_SNDBUF_SIZE CONFIG_NET_TCP_MAX_RECV_WINDOW_SIZE

ZTEST(net_socket_poll, test_pollout_tcp)
{
	int res;
	int c_sock;
	int s_sock;
	int new_sock;
	struct sockaddr_in6 c_addr;
	struct sockaddr_in6 s_addr;
	struct zsock_pollfd pollout[1];
	char buf[TEST_SNDBUF_SIZE] = { };

	prepare_sock_tcp_v6(MY_IPV6_ADDR, CLIENT_PORT, &c_sock, &c_addr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_addr);

	res = zsock_bind(s_sock, (struct sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(res, 0, "");
	res = zsock_listen(s_sock, 0);
	zassert_equal(res, 0, "");
	res = zsock_connect(c_sock, (const struct sockaddr *)&s_addr,
			    sizeof(s_addr));
	zassert_equal(res, 0, "");
	new_sock = zsock_accept(s_sock, NULL, NULL);
	zassert_true(new_sock >= 0, "");

	k_msleep(10);

	/* POLLOUT should be reported after connecting */
	memset(pollout, 0, sizeof(pollout));
	pollout[0].fd = c_sock;
	pollout[0].events = ZSOCK_POLLOUT;

	res = zsock_poll(pollout, ARRAY_SIZE(pollout), 10);
	zassert_equal(res, 1, "");
	zassert_equal(pollout[0].revents, ZSOCK_POLLOUT, "");

	/* POLLOUT should not be reported after filling the window */
	res = zsock_send(c_sock, buf, sizeof(buf), 0);
	zassert_equal(res, sizeof(buf), "");

	memset(pollout, 0, sizeof(pollout));
	pollout[0].fd = c_sock;
	pollout[0].events = ZSOCK_POLLOUT;

	res = zsock_poll(pollout, ARRAY_SIZE(pollout), 10);
	zassert_equal(res, 0, "%d", pollout[0].revents);
	zassert_equal(pollout[0].revents, 0, "");

	/* POLLOUT should be reported again after consuming the data server
	 * side.
	 */
	res = zsock_recv(new_sock, buf, sizeof(buf), 0);
	zassert_equal(res, sizeof(buf), "");

	memset(pollout, 0, sizeof(pollout));
	pollout[0].fd = c_sock;
	pollout[0].events = ZSOCK_POLLOUT;

	/* Wait longer this time to give TCP stack a chance to send ZWP. */
	res = zsock_poll(pollout, ARRAY_SIZE(pollout), 500);
	zassert_equal(res, 1, "");
	zassert_equal(pollout[0].revents, ZSOCK_POLLOUT, "");

	k_msleep(10);

	/* Finalize the test */
	res = zsock_close(c_sock);
	zassert_equal(res, 0, "close failed");
	res = zsock_close(s_sock);
	zassert_equal(res, 0, "close failed");
	res = zsock_close(new_sock);
	zassert_equal(res, 0, "close failed");
}

ZTEST_SUITE(net_socket_poll, NULL, NULL, NULL, NULL, NULL);
