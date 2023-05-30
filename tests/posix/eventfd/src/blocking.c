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

	close(fixture->fd);

	/* Test EFD_SEMAPHORE */

	fixture->fd = eventfd(0, EFD_SEMAPHORE);
	zassert_true(fixture->fd >= 0, "fd == %d", fixture->fd);

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
