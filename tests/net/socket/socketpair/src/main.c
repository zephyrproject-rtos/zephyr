/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <zephyr/ztest.h>

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
extern void test_socketpair_poll_signalling_POLLIN(void);
extern void test_socketpair_poll_signalling_POLLOUT(void);

static void *setup(void)
{
	k_thread_system_pool_assign(k_current_get());
	return NULL;
}

ZTEST_SUITE(net_socketpair, NULL, setup, NULL, NULL, NULL);
