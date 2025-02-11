/*
 * Copyright (c) 2020 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

ZTEST_F(eventfd, test_read_nonblock)
{
	eventfd_t val = 0;
	int ret;

	reopen(&fixture->fd, 0, EFD_NONBLOCK);

	ret = eventfd_read(fixture->fd, &val);
	zassert_true(ret == -1, "read unset ret %d", ret);
	zassert_true(errno == EAGAIN, "errno %d", errno);

	ret = eventfd_write(fixture->fd, TESTVAL);
	zassert_true(ret == 0, "write ret %d", ret);

	ret = eventfd_read(fixture->fd, &val);
	zassert_true(ret == 0, "read set ret %d", ret);
	zassert_true(val == TESTVAL, "red set val %d", val);

	ret = eventfd_read(fixture->fd, &val);
	zassert_true(ret == -1, "read subsequent ret %d val %d", ret, val);
	zassert_true(errno == EAGAIN, "errno %d", errno);
}

ZTEST_F(eventfd, test_set_poll_event_nonblock)
{
	reopen(&fixture->fd, TESTVAL, EFD_NONBLOCK);
	eventfd_poll_set_common(fixture->fd);
}

ZTEST_F(eventfd, test_unset_poll_event_nonblock)
{
	reopen(&fixture->fd, 0, EFD_NONBLOCK);
	eventfd_poll_unset_common(fixture->fd);
}

ZTEST_F(eventfd, test_overflow)
{
	short event;
	int ret;

	reopen(&fixture->fd, 0, EFD_NONBLOCK);

	event = POLLOUT;
	ret = is_blocked(fixture->fd, &event);
	zassert_equal(ret, 0, "eventfd write blocked with initval == 0");

	ret = eventfd_write(fixture->fd, UINT64_MAX);
	zassert_equal(ret, -1, "fd == %d", fixture->fd);
	zassert_equal(errno, EINVAL, "did not get EINVAL");

	ret = eventfd_write(fixture->fd, UINT64_MAX-1);
	zassert_equal(ret, 0, "fd == %d", fixture->fd);

	event = POLLOUT;
	ret = is_blocked(fixture->fd, &event);
	zassert_equal(ret, 1, "eventfd write not blocked with cnt == UINT64_MAX-1");

	ret = eventfd_write(fixture->fd, 1);
	zassert_equal(ret, -1, "fd == %d", fixture->fd);
	zassert_equal(errno, EAGAIN, "did not get EINVAL");
}
