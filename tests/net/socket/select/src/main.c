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

#include "../../socket_helpers.h"

#define BUF_AND_SIZE(buf) buf, sizeof(buf) - 1
#define STRLEN(buf) (sizeof(buf) - 1)

#define TEST_STR_SMALL "test"

#define MY_IPV6_ADDR "::1"

#define ANY_PORT 0
#define SERVER_PORT 4242
#define CLIENT_PORT 9898

/* Fudge factor added to expected timeouts, in milliseconds.
 * Had to set this pretty large to avoid spurious failures in CI.
 */
#define FUZZ 180

#define TIMEOUT_MS 120

ZTEST_USER(net_socket_select, test_fd_set)
{
	zsock_fd_set set;

	/* Relies on specific value of CONFIG_ZVFS_OPEN_MAX in prj.conf */
	zassert_equal(sizeof(set.bitset), sizeof(uint32_t) * 2, "");

	ZSOCK_FD_ZERO(&set);
	zassert_equal(set.bitset[0], 0, "");
	zassert_equal(set.bitset[1], 0, "");
	zassert_false(ZSOCK_FD_ISSET(0, &set), "");

	ZSOCK_FD_SET(0, &set);
	zassert_true(ZSOCK_FD_ISSET(0, &set), "");

	ZSOCK_FD_CLR(0, &set);
	zassert_false(ZSOCK_FD_ISSET(0, &set), "");

	ZSOCK_FD_SET(0, &set);
	zassert_equal(set.bitset[0], 0x00000001, "");
	zassert_equal(set.bitset[1], 0, "");

	ZSOCK_FD_SET(31, &set);
	zassert_equal(set.bitset[0], 0x80000001, "");
	zassert_equal(set.bitset[1], 0, "");

	ZSOCK_FD_SET(33, &set);
	zassert_equal(set.bitset[0], 0x80000001, "");
	zassert_equal(set.bitset[1], 0x00000002, "");

	ZSOCK_FD_ZERO(&set);
	zassert_equal(set.bitset[0], 0, "");
	zassert_equal(set.bitset[1], 0, "");
}

ZTEST_USER(net_socket_select, test_select)
{
	int res;
	int c_sock;
	int s_sock;
	struct net_sockaddr_in6 c_addr;
	struct net_sockaddr_in6 s_addr;
	zsock_fd_set readfds;
	uint32_t tstamp;
	ssize_t len;
	char buf[10];
	struct timeval tval;

	prepare_sock_udp_v6(MY_IPV6_ADDR, CLIENT_PORT, &c_sock, &c_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_addr);

	res = zsock_bind(s_sock, (struct net_sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(res, 0, "bind failed");

	res = zsock_connect(c_sock, (struct net_sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(res, 0, "connect failed");

	ZSOCK_FD_ZERO(&readfds);
	ZSOCK_FD_SET(c_sock, &readfds);
	ZSOCK_FD_SET(s_sock, &readfds);

	/* Poll non-ready fd's with timeout of 0 */
	tval.tv_sec = tval.tv_usec = 0;
	tstamp = k_uptime_get_32();
	res = zsock_select(s_sock + 1, &readfds, NULL, NULL, &tval);
	tstamp = k_uptime_get_32() - tstamp;
	/* Even though we expect select to be non-blocking, scheduler may
	 * preempt the thread. That's why we add FUZZ to the expected
	 * delay time. Also applies to similar cases below.
	 */
	zassert_true(tstamp <= FUZZ, "");
	zassert_equal(res, 0, "");

	zassert_false(ZSOCK_FD_ISSET(c_sock, &readfds), "");
	zassert_false(ZSOCK_FD_ISSET(s_sock, &readfds), "");

	/* Poll non-ready fd's with timeout of 10ms */
	ZSOCK_FD_SET(c_sock, &readfds);
	ZSOCK_FD_SET(s_sock, &readfds);
	tval.tv_sec = 0;
	tval.tv_usec = TIMEOUT_MS * 1000;
	tstamp = k_uptime_get_32();
	res = zsock_select(s_sock + 1, &readfds, NULL, NULL, &tval);
	tstamp = k_uptime_get_32() - tstamp;
	zassert_true(tstamp >= TIMEOUT_MS && tstamp <= TIMEOUT_MS + FUZZ, "");
	zassert_equal(res, 0, "");


	/* Send pkt for s_sock and poll with timeout of 10ms */
	len = zsock_send(c_sock, BUF_AND_SIZE(TEST_STR_SMALL), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

	ZSOCK_FD_SET(c_sock, &readfds);
	ZSOCK_FD_SET(s_sock, &readfds);
	tval.tv_sec = 0;
	tval.tv_usec = TIMEOUT_MS * 1000;
	tstamp = k_uptime_get_32();
	res = zsock_select(s_sock + 1, &readfds, NULL, NULL, &tval);
	tstamp = k_uptime_get_32() - tstamp;
	zassert_true(tstamp <= FUZZ, "");
	zassert_equal(res, 1, "");

	zassert_false(ZSOCK_FD_ISSET(c_sock, &readfds), "");
	zassert_true(ZSOCK_FD_ISSET(s_sock, &readfds), "");


	/* Recv pkt from s_sock and ensure no poll events happen */
	len = zsock_recv(s_sock, BUF_AND_SIZE(buf), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid recv len");

	ZSOCK_FD_SET(c_sock, &readfds);
	ZSOCK_FD_SET(s_sock, &readfds);
	tval.tv_sec = tval.tv_usec = 0;
	tstamp = k_uptime_get_32();
	res = zsock_select(s_sock + 1, &readfds, NULL, NULL, &tval);
	zassert_true(k_uptime_get_32() - tstamp <= FUZZ, "");
	zassert_equal(res, 0, "");
	zassert_false(ZSOCK_FD_ISSET(s_sock, &readfds), "");


	/* Close one socket and ensure POLLNVAL happens */
	res = zsock_close(c_sock);
	zassert_equal(res, 0, "close failed");

	ZSOCK_FD_SET(c_sock, &readfds);
	ZSOCK_FD_SET(s_sock, &readfds);
	tval.tv_sec = tval.tv_usec = 0;
	tstamp = k_uptime_get_32();
	res = zsock_select(s_sock + 1, &readfds, NULL, NULL, &tval);
	zassert_true(k_uptime_get_32() - tstamp <= FUZZ, "");
	zassert_true(res < 0, "");
	zassert_equal(errno, EBADF, "");

	res = zsock_close(s_sock);
	zassert_equal(res, 0, "close failed");
}

/*
 * Verify that select() honors a large timeout and does not expire prematurely.
 *
 * OVERFLOW_TIMEOUT_SEC (4295) is the smallest whole-second timeout whose
 * conversion to microseconds (seconds * 1,000,000) exceeds UINT32_MAX. If that
 * multiplication is done in 32-bit arithmetic, the result wraps to a small
 * value (~33 ms) instead of the intended >1 hour.
 *
 * SENDER_DELAY_MS (200) is when the helper thread makes the socket readable.
 * That is long enough to outlast a wrongly truncated timeout, but far shorter
 * than the real timeout, so select() should return 1 (data ready), not 0
 * (timed out early).
 */
#define OVERFLOW_TIMEOUT_SEC 4295
#define SENDER_DELAY_MS      200

static int overflow_c_sock;

static void overflow_sender(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_msleep(SENDER_DELAY_MS);
	(void)zsock_send(overflow_c_sock, BUF_AND_SIZE(TEST_STR_SMALL), 0);
}

K_THREAD_STACK_DEFINE(overflow_sender_stack, 1024);
static struct k_thread overflow_sender_thread;

ZTEST(net_socket_select, test_select_large_timeout_no_overflow)
{
	int c_sock;
	int s_sock;
	struct net_sockaddr_in6 c_addr;
	struct net_sockaddr_in6 s_addr;
	zsock_fd_set readfds;
	struct timeval tval = {.tv_sec = OVERFLOW_TIMEOUT_SEC, .tv_usec = 0};

	prepare_sock_udp_v6(MY_IPV6_ADDR, CLIENT_PORT, &c_sock, &c_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock, &s_addr);
	zassert_equal(zsock_bind(s_sock, (struct net_sockaddr *)&s_addr, sizeof(s_addr)), 0);
	zassert_equal(zsock_connect(c_sock, (struct net_sockaddr *)&s_addr, sizeof(s_addr)), 0);

	ZSOCK_FD_ZERO(&readfds);
	ZSOCK_FD_SET(s_sock, &readfds);

	overflow_c_sock = c_sock;
	k_thread_create(&overflow_sender_thread, overflow_sender_stack,
			K_THREAD_STACK_SIZEOF(overflow_sender_stack), overflow_sender, NULL, NULL,
			NULL, K_PRIO_PREEMPT(10), 0, K_NO_WAIT);

	/* 1 = woken by data; 0 = timed out early (large timeout not honored) */
	zassert_equal(zsock_select(s_sock + 1, &readfds, NULL, NULL, &tval), 1,
		      "select timed out before data arrived: large timeout not honored");

	zassert_ok(k_thread_join(&overflow_sender_thread, K_FOREVER));
	zassert_equal(zsock_close(c_sock), 0);
	zassert_equal(zsock_close(s_sock), 0);
}

static void *setup(void)
{
	if (IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)) {
		k_thread_priority_set(k_current_get(),
				K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1));
	} else {
		k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(9));
	}

	k_thread_system_pool_assign(k_current_get());
	return NULL;
}

ZTEST_SUITE(net_socket_select, NULL, setup, NULL, NULL, NULL);
