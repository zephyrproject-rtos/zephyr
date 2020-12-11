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

void test_fd_set(void)
{
	fd_set set;

	/* Relies on specific value of CONFIG_POSIX_MAX_FDS in prj.conf */
	zassert_equal(sizeof(set.bitset), sizeof(uint32_t) * 2, "");

	FD_ZERO(&set);
	zassert_equal(set.bitset[0], 0, "");
	zassert_equal(set.bitset[1], 0, "");
	zassert_false(FD_ISSET(0, &set), "");

	FD_SET(0, &set);
	zassert_true(FD_ISSET(0, &set), "");

	FD_CLR(0, &set);
	zassert_false(FD_ISSET(0, &set), "");

	FD_SET(0, &set);
	zassert_equal(set.bitset[0], 0x00000001, "");
	zassert_equal(set.bitset[1], 0, "");

	FD_SET(31, &set);
	zassert_equal(set.bitset[0], 0x80000001, "");
	zassert_equal(set.bitset[1], 0, "");

	FD_SET(33, &set);
	zassert_equal(set.bitset[0], 0x80000001, "");
	zassert_equal(set.bitset[1], 0x00000002, "");

	FD_ZERO(&set);
	zassert_equal(set.bitset[0], 0, "");
	zassert_equal(set.bitset[1], 0, "");
}

void test_select(void)
{
	int res;
	int c_sock;
	int s_sock;
	struct sockaddr_in6 c_addr;
	struct sockaddr_in6 s_addr;
	fd_set readfds;
	uint32_t tstamp;
	ssize_t len;
	char buf[10];
	struct timeval tval;

	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, CLIENT_PORT,
			    &c_sock, &c_addr);
	prepare_sock_udp_v6(CONFIG_NET_CONFIG_MY_IPV6_ADDR, SERVER_PORT,
			    &s_sock, &s_addr);

	res = bind(s_sock, (struct sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(res, 0, "bind failed");

	res = connect(c_sock, (struct sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(res, 0, "connect failed");

	FD_ZERO(&readfds);
	FD_SET(c_sock, &readfds);
	FD_SET(s_sock, &readfds);

	/* Poll non-ready fd's with timeout of 0 */
	tval.tv_sec = tval.tv_usec = 0;
	tstamp = k_uptime_get_32();
	res = select(s_sock + 1, &readfds, NULL, NULL, &tval);
	tstamp = k_uptime_get_32() - tstamp;
	/* Even though we expect select to be non-blocking, scheduler may
	 * preempt the thread. That's why we add FUZZ to the expected
	 * delay time. Also applies to similar cases below.
	 */
	zassert_true(tstamp <= FUZZ, "");
	zassert_equal(res, 0, "");

	zassert_false(FD_ISSET(c_sock, &readfds), "");
	zassert_false(FD_ISSET(s_sock, &readfds), "");

	/* Poll non-ready fd's with timeout of 10ms */
	FD_SET(c_sock, &readfds);
	FD_SET(s_sock, &readfds);
	tval.tv_sec = 0;
	tval.tv_usec = 30 * 1000;
	tstamp = k_uptime_get_32();
	res = select(s_sock + 1, &readfds, NULL, NULL, &tval);
	tstamp = k_uptime_get_32() - tstamp;
	zassert_true(tstamp >= 30U && tstamp <= 30 + FUZZ, "");
	zassert_equal(res, 0, "");


	/* Send pkt for s_sock and poll with timeout of 10ms */
	len = send(c_sock, BUF_AND_SIZE(TEST_STR_SMALL), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

	FD_SET(c_sock, &readfds);
	FD_SET(s_sock, &readfds);
	tval.tv_sec = 0;
	tval.tv_usec = 30 * 1000;
	tstamp = k_uptime_get_32();
	res = select(s_sock + 1, &readfds, NULL, NULL, &tval);
	tstamp = k_uptime_get_32() - tstamp;
	zassert_true(tstamp <= FUZZ, "");
	zassert_equal(res, 1, "");

	zassert_false(FD_ISSET(c_sock, &readfds), "");
	zassert_true(FD_ISSET(s_sock, &readfds), "");


	/* Recv pkt from s_sock and ensure no poll events happen */
	len = recv(s_sock, BUF_AND_SIZE(buf), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid recv len");

	FD_SET(c_sock, &readfds);
	FD_SET(s_sock, &readfds);
	tval.tv_sec = tval.tv_usec = 0;
	tstamp = k_uptime_get_32();
	res = select(s_sock + 1, &readfds, NULL, NULL, &tval);
	zassert_true(k_uptime_get_32() - tstamp <= FUZZ, "");
	zassert_equal(res, 0, "");
	zassert_false(FD_ISSET(s_sock, &readfds), "");


	/* Close one socket and ensure POLLNVAL happens */
	res = close(c_sock);
	zassert_equal(res, 0, "close failed");

	FD_SET(c_sock, &readfds);
	FD_SET(s_sock, &readfds);
	tval.tv_sec = tval.tv_usec = 0;
	tstamp = k_uptime_get_32();
	res = select(s_sock + 1, &readfds, NULL, NULL, &tval);
	zassert_true(k_uptime_get_32() - tstamp <= FUZZ, "");
	zassert_true(res < 0, "");
	zassert_equal(errno, EBADF, "");

	res = close(s_sock);
	zassert_equal(res, 0, "close failed");
}

void test_main(void)
{
	if (IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)) {
		k_thread_priority_set(k_current_get(),
				K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1));
	} else {
		k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(9));
	}

	k_thread_system_pool_assign(k_current_get());

	ztest_test_suite(socket_select,
			 ztest_user_unit_test(test_fd_set),
			 ztest_user_unit_test(test_select));

	ztest_run_test_suite(socket_select);
}
