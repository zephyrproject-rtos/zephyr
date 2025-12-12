/*
 * Copyright (c) 2020 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

ZTEST_F(timerfd, test_read_nonblock)
{
	uint64_t val = 0;
	int ret;

	struct itimerspec ts = {
		.it_interval = {0, 100 * NSEC_PER_MSEC},
		.it_value = {0, 100 * NSEC_PER_MSEC},
	};

	reopen(&fixture->fd, 0, TFD_NONBLOCK);

	ret = read(fixture->fd, &val, sizeof(val));
	zassert_true(ret == -1, "read unset ret %d", ret);
	zassert_true(errno == EAGAIN, "errno %d", errno);

	// ret = eventfd_write(fixture->fd, TESTVAL);
	// zassert_true(ret == 0, "write ret %d", ret);
	zassert_ok(timerfd_settime(fixture->fd, 0, &ts, NULL));
	k_sleep(K_MSEC(550));

	ret = read(fixture->fd, &val, sizeof(val));
	zassert_true(ret == sizeof(val), "read set ret %d", ret);
	zassert_true(val == 5, "red set val %lld", val);

	ret = read(fixture->fd, &val, sizeof(val));
	zassert_true(ret == -1, "read subsequent ret %d val %lld", ret, val);
	zassert_true(errno == EAGAIN, "errno %d", errno);
}

ZTEST_F(timerfd, test_set_poll_event_nonblock)
{
	reopen(&fixture->fd, CLOCK_MONOTONIC, TFD_NONBLOCK);
	zassert_ok(ioctl(fixture->fd, TFD_IOC_SET_TICKS, (uint64_t)TESTVAL));
	timerfd_poll_set_common(fixture->fd);
}

ZTEST_F(timerfd, test_unset_poll_event_nonblock)
{
	reopen(&fixture->fd, CLOCK_MONOTONIC, TFD_NONBLOCK);
	timerfd_poll_unset_common(fixture->fd);
}
