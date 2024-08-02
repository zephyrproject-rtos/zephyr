/*
 * Copyright (c) 2024 Celina Sophie Kalus
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/sys/ioctl.h>

#define EFD_IN_USE_INTERNAL 0x1

ZTEST_F(eventfd, test_set_flags)
{
	eventfd_t val;
	int ret;
	int flags;
	short event;

	/* Get current flags; Expect blocking, non-semaphore. */
	flags = ioctl(fixture->fd, F_GETFL, 0);
	zassert_equal(flags, 0, "flags == %d", flags);

	event = POLLIN;
	ret = is_blocked(fixture->fd, &event);
	zassert_equal(ret, 1, "eventfd read not blocked");

	/* Try writing and reading. Should not fail. */
	ret = eventfd_write(fixture->fd, 3);
	zassert_ok(ret);

	ret = eventfd_read(fixture->fd, &val);
	zassert_ok(ret);
	zassert_equal(val, 3, "val == %d", val);


	/* Set nonblocking without reopening. */
	ret = ioctl(fixture->fd, F_SETFL, O_NONBLOCK);
	zassert_ok(ret);

	flags = ioctl(fixture->fd, F_GETFL, 0);
	zassert_equal(flags, O_NONBLOCK, "flags == %d", flags);

	event = POLLOUT;
	ret = is_blocked(fixture->fd, &event);
	zassert_equal(ret, 0, "eventfd write blocked");


	/* Try writing and reading again. */
	ret = eventfd_write(fixture->fd, 19);
	zassert_ok(ret);

	ret = eventfd_read(fixture->fd, &val);
	zassert_ok(ret);
	zassert_equal(val, 19, "val == %d", val);


	/* Set back to blocking. */
	ret = ioctl(fixture->fd, F_SETFL, 0);
	zassert_ok(ret);

	flags = ioctl(fixture->fd, F_GETFL, 0);
	zassert_equal(flags, 0, "flags == %d", flags);

	event = POLLIN;
	ret = is_blocked(fixture->fd, &event);
	zassert_equal(ret, 1, "eventfd read not blocked");


	/* Try writing and reading again. */
	ret = eventfd_write(fixture->fd, 10);
	zassert_ok(ret);

	ret = eventfd_read(fixture->fd, &val);
	zassert_ok(ret);
	zassert_equal(val, 10, "val == %d", val);


	/* Test setting internal in-use-flag. Should fail. */
	ret = ioctl(fixture->fd, F_SETFL, EFD_IN_USE_INTERNAL);
	zassert_not_ok(ret);


	/* File descriptor should still be valid and working. */
	ret = eventfd_write(fixture->fd, 97);
	zassert_ok(ret);

	ret = eventfd_read(fixture->fd, &val);
	zassert_ok(ret);
	zassert_equal(val, 97, "val == %d", val);
}
