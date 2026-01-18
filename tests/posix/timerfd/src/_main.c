/*
 * Copyright (c) 2020 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

void reopen(int *fd, int clockid, int flags)
{
	zassert_not_null(fd);
	zassert_ok(close(*fd));
	*fd = timerfd_create(clockid, flags);
	zassert_true(*fd >= 0, "timerfd(%d, %d) failed: %d", clockid, flags, errno);
}

int is_blocked(int fd, short *event)
{
	struct pollfd pfd;
	int ret;

	pfd.fd = fd;
	pfd.events = *event;

	ret = poll(&pfd, 1, 0);
	zassert_true(ret >= 0, "poll failed %d", ret);

	*event = pfd.revents;

	return ret == 0;
}

void timerfd_poll_unset_common(int fd)
{
   	uint64_t val = 0;

	short event;
	int ret;

	event = POLLIN;
	ret = is_blocked(fd, &event);
	zassert_equal(ret, 1, "timerfd not blocked with initval == 0");

	zassert_ok(ioctl(fd, TFD_IOC_SET_TICKS, (uint64_t)TESTVAL));

	event = POLLIN;
	ret = is_blocked(fd, &event);
	zassert_equal(ret, 0, "timerfd blocked after write");
	zassert_equal(event, POLLIN, "POLLIN not set");

	zassert_equal(read(fd, &val, sizeof(val)), sizeof(val), "read failed");
	zassert_equal(val, TESTVAL, "val == %lld, expected %d", val, TESTVAL);

	/* timerfd shall block on subsequent reads before next interval expires */

	event = POLLIN;
	ret = is_blocked(fd, &event);
	zassert_equal(ret, 1, "timerfd not blocked after read");
}

void timerfd_poll_set_common(int fd)
{
	timerfd_t val = 0;
	short event;
	int ret;

	event = POLLIN;
	ret = is_blocked(fd, &event);
	zassert_equal(ret, 0, "timerfd is blocked with initval != 0");

	ret = read(fd, &val, sizeof(val));
	zassert_equal(ret, sizeof(val), "read ret %d", ret);
	zassert_equal(val, TESTVAL, "val == %d", (int)val);

	event = POLLIN;
	ret = is_blocked(fd, &event);
	zassert_equal(ret, 1, "timerfd is not blocked after read");
}

static struct timerfd_fixture tfd_fixture;

static void *setup(void)
{
	tfd_fixture.fd = -1;
	return &tfd_fixture;
}

static void before(void *arg)
{
	struct timerfd_fixture *fixture = arg;

	fixture->fd = timerfd_create(0, 0);
	zassert_true(fixture->fd >= 0, "timerfd(0, 0) failed: %d", errno);
}

static void after(void *arg)
{
	struct timerfd_fixture *fixture = arg;

	if (fixture->fd != -1) {
		close(fixture->fd);
		fixture->fd = -1;
	}
}

ZTEST_SUITE(timerfd, NULL, setup, before, after, NULL);
