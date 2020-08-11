/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <ztest.h>

#include "test_socketpair_thread.h"

/* in happy_path.c */
extern void test_socketpair_AF_LOCAL__SOCK_STREAM__0(void);
extern void test_socketpair_AF_UNIX__SOCK_STREAM__0(void);

/* in expected_failures.c */
extern void test_socketpair_expected_failures(void);

/* in unsupported_calls.c */
extern void test_socketpair_unsupported_calls(void);

/* in fcntl.c */
extern void test_socketpair_fcntl(void);

/* in nonblock.c */
extern void test_socketpair_read_nonblock(void);
extern void test_socketpair_write_nonblock(void);

/* in block.c */
extern void test_socketpair_read_block(void);
extern void test_socketpair_write_block(void);

/* in closed_ends.c */
extern void test_socketpair_close_one_end_and_read_from_the_other(void);
extern void test_socketpair_close_one_end_and_write_to_the_other(void);

/* in poll.c */
extern void test_socketpair_poll_timeout(void);
extern void test_socketpair_poll_timeout_nonblocking(void);
extern void test_socketpair_poll_immediate_data(void);
extern void test_socketpair_poll_delayed_data(void);
extern void test_socketpair_poll_close_remote_end_POLLIN(void);
extern void test_socketpair_poll_close_remote_end_POLLOUT(void);

/* work queue for tests that need an async event */
static K_THREAD_STACK_DEFINE(test_socketpair_work_q_stack, 512);
struct k_work_q test_socketpair_work_q;

void test_main(void)
{
	k_thread_system_pool_assign(k_current_get());

	k_thread_access_grant(k_current_get(),
		&test_socketpair_work_q.queue);

	k_work_q_start(&test_socketpair_work_q, test_socketpair_work_q_stack,
		K_THREAD_STACK_SIZEOF(test_socketpair_work_q_stack),
		CONFIG_MAIN_THREAD_PRIORITY);

	ztest_test_suite(
		socketpair,

		ztest_user_unit_test(test_socketpair_AF_LOCAL__SOCK_STREAM__0),
		ztest_user_unit_test(test_socketpair_AF_UNIX__SOCK_STREAM__0),

		ztest_user_unit_test(test_socketpair_expected_failures),
		ztest_user_unit_test(test_socketpair_unsupported_calls),

		ztest_user_unit_test(test_socketpair_fcntl),

		ztest_user_unit_test(test_socketpair_read_nonblock),
		ztest_user_unit_test(test_socketpair_write_nonblock),

		ztest_user_unit_test(test_socketpair_read_block),
		ztest_user_unit_test(test_socketpair_write_block),

		ztest_user_unit_test(
			test_socketpair_close_one_end_and_read_from_the_other),
		ztest_user_unit_test(
			test_socketpair_close_one_end_and_write_to_the_other),

		ztest_user_unit_test(test_socketpair_poll_timeout),
		ztest_user_unit_test(
			test_socketpair_poll_timeout_nonblocking),
		ztest_user_unit_test(test_socketpair_poll_immediate_data),
		ztest_user_unit_test(test_socketpair_poll_delayed_data),

		ztest_user_unit_test(
			test_socketpair_poll_close_remote_end_POLLIN),
		ztest_user_unit_test(
			test_socketpair_poll_close_remote_end_POLLOUT)
	);

	ztest_run_test_suite(socketpair);
}
