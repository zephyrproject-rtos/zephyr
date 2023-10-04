/*
 * Copyright (c) 2020 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

ZTEST_F(eventfd, test_write_then_read)
{
	eventfd_t val;
	int ret;

	ret = eventfd_write(fixture->fd, 3);
	zassert_true(ret == 0, "write ret %d", ret);

	ret = eventfd_write(fixture->fd, 2);
	zassert_true(ret == 0, "write ret %d", ret);

	ret = eventfd_read(fixture->fd, &val);
	zassert_true(ret == 0, "read ret %d", ret);
	zassert_true(val == 5, "val == %d", val);

	/* Test EFD_SEMAPHORE */
	reopen(&fixture->fd, 0, EFD_SEMAPHORE);

	ret = eventfd_write(fixture->fd, 3);
	zassert_true(ret == 0, "write ret %d", ret);

	ret = eventfd_write(fixture->fd, 2);
	zassert_true(ret == 0, "write ret %d", ret);

	ret = eventfd_read(fixture->fd, &val);
	zassert_true(ret == 0, "read ret %d", ret);
	zassert_true(val == 1, "val == %d", val);
}

ZTEST_F(eventfd, test_zero_shall_not_unblock)
{
	short event;
	int ret;

	ret = eventfd_write(fixture->fd, 0);
	zassert_equal(ret, 0, "fd == %d", fixture->fd);

	event = POLLIN;
	ret = is_blocked(fixture->fd, &event);
	zassert_equal(ret, 1, "eventfd unblocked by zero");
}

ZTEST_F(eventfd, test_poll_timeout)
{
	struct pollfd pfd;
	int ret;

	pfd.fd = fixture->fd;
	pfd.events = POLLIN;

	ret = poll(&pfd, 1, 500);
	zassert_true(ret == 0, "poll ret %d", ret);
}

ZTEST_F(eventfd, test_set_poll_event_block)
{
	reopen(&fixture->fd, TESTVAL, 0);
	eventfd_poll_set_common(fixture->fd);
}

ZTEST_F(eventfd, test_unset_poll_event_block)
{
	eventfd_poll_unset_common(fixture->fd);
}

K_THREAD_STACK_DEFINE(thread_stack, CONFIG_TEST_STACK_SIZE);

static void thread_fun(void *arg1, void *arg2, void *arg3)
{
	eventfd_t value;
	struct eventfd_fixture *fixture = arg1;

	zassert_ok(eventfd_read(fixture->fd, &value));
	zassert_equal(value, 42);
}

ZTEST_F(eventfd, test_read_then_write_block)
{
	struct k_thread thread;

	k_thread_create(&thread, thread_stack, K_THREAD_STACK_SIZEOF(thread_stack), thread_fun,
			fixture, NULL, NULL, 0, 0, K_NO_WAIT);

	k_msleep(100);

	/* this write never occurs */
	zassert_ok(eventfd_write(fixture->fd, 42));

	/* unreachable code */
	k_thread_join(&thread, K_FOREVER);
}
