/*
 * Copyright (c) 2024 Celina Sophie Kalus
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"
#include "sys/stat.h"

#define TFD_IN_USE_INTERNAL 0x1

ZTEST_F(timerfd, test_fstat)
{
    struct stat statbuf;
	zassert_ok(fstat(fixture->fd, &statbuf));
}

ZTEST_F(timerfd, test_set_flags)
{
	timerfd_t val;
	int ret;
	int flags;
	short event;

	/* Get current flags; Expect blocking, non-semaphore. */
	flags = ioctl(fixture->fd, F_GETFL, 0);
	zassert_equal(flags, 0, "flags == %d", flags);

	event = POLLIN;
	ret = is_blocked(fixture->fd, &event);
	zassert_equal(ret, 1, "timerfd read not blocked");

	/* Try writing and reading. Should not fail. */
	zassert_ok(ioctl(fixture->fd, TFD_IOC_SET_TICKS, (uint64_t)3));

	ret = read(fixture->fd, &val, sizeof(val));
	zassert_equal(ret, sizeof(val), "read failed");
	zassert_equal(val, 3, "val == %lld", val);


	/* Set nonblocking without reopening. */
	ret = ioctl(fixture->fd, F_SETFL, O_NONBLOCK);
	zassert_ok(ret);

	flags = ioctl(fixture->fd, F_GETFL, 0);
	zassert_equal(flags, O_NONBLOCK, "flags == %d", flags);

	event = POLLIN;
	ret = is_blocked(fixture->fd, &event);
	zassert_equal(ret, 1, "timerfd read not blocked");


	/* Try writing and reading again. */
	zassert_ok(ioctl(fixture->fd, TFD_IOC_SET_TICKS, (uint64_t)19));

	ret = read(fixture->fd, &val, sizeof(val));
	zassert_equal(ret, sizeof(val), "read failed");
	zassert_equal(val, 19, "val == %lld", val);


	/* Set back to blocking. */
	ret = ioctl(fixture->fd, F_SETFL, 0);
	zassert_ok(ret);

	flags = ioctl(fixture->fd, F_GETFL, 0);
	zassert_equal(flags, 0, "flags == %d", flags);

	event = POLLIN;
	ret = is_blocked(fixture->fd, &event);
	zassert_equal(ret, 1, "timerfd read not blocked");


	/* Try writing and reading again. */
	zassert_ok(ioctl(fixture->fd, TFD_IOC_SET_TICKS, (uint64_t)10));

	ret = read(fixture->fd, &val, sizeof(val));
	zassert_equal(ret, sizeof(val), "read failed");
	zassert_equal(val, 10, "val == %lld", val);


	/* Test setting internal in-use-flag. Should fail. */
	ret = ioctl(fixture->fd, F_SETFL, TFD_IN_USE_INTERNAL);
	zassert_not_ok(ret);


	/* File descriptor should still be valid and working. */
	zassert_ok(ioctl(fixture->fd, TFD_IOC_SET_TICKS, (uint64_t)97));

	ret = read(fixture->fd, &val, sizeof(val));
	zassert_equal(ret, sizeof(val), "read failed");
	zassert_equal(val, 97, "val == %lld", val);
}
