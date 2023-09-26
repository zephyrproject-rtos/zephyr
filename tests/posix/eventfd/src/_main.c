/*
 * Copyright (c) 2020 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

void reopen(int *fd, int initval, int flags)
{
	zassert_not_null(fd);
	zassert_ok(close(*fd));
	*fd = eventfd(initval, flags);
	zassert_true(*fd >= 0, "eventfd(%d, %d) failed: %d", initval, flags, errno);
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

void eventfd_poll_unset_common(int fd)
{
	eventfd_t val = 0;
	short event;
	int ret;

	event = POLLIN;
	ret = is_blocked(fd, &event);
	zassert_equal(ret, 1, "eventfd not blocked with initval == 0");

	ret = eventfd_write(fd, TESTVAL);
	zassert_equal(ret, 0, "write ret %d", ret);

	event = POLLIN;
	ret = is_blocked(fd, &event);
	zassert_equal(ret, 0, "eventfd blocked after write");
	zassert_equal(event, POLLIN, "POLLIN not set");

	ret = eventfd_write(fd, TESTVAL);
	zassert_equal(ret, 0, "write ret %d", ret);

	ret = eventfd_read(fd, &val);
	zassert_equal(ret, 0, "read ret %d", ret);
	zassert_equal(val, 2*TESTVAL, "val == %d, expected %d", val, TESTVAL);

	/* eventfd shall block on subsequent reads */

	event = POLLIN;
	ret = is_blocked(fd, &event);
	zassert_equal(ret, 1, "eventfd not blocked after read");
}

void eventfd_poll_set_common(int fd)
{
	eventfd_t val = 0;
	short event;
	int ret;

	event = POLLIN;
	ret = is_blocked(fd, &event);
	zassert_equal(ret, 0, "eventfd is blocked with initval != 0");

	ret = eventfd_read(fd, &val);
	zassert_equal(ret, 0, "read ret %d", ret);
	zassert_equal(val, TESTVAL, "val == %d", val);

	event = POLLIN;
	ret = is_blocked(fd, &event);
	zassert_equal(ret, 1, "eventfd is not blocked after read");
}

static struct eventfd_fixture efd_fixture;

static void *setup(void)
{
	efd_fixture.fd = -1;
	return &efd_fixture;
}

static void before(void *arg)
{
	struct eventfd_fixture *fixture = arg;

	fixture->fd = eventfd(0, 0);
	zassert_true(fixture->fd >= 0, "eventfd(0, 0) failed: %d", errno);
}

static void after(void *arg)
{
	struct eventfd_fixture *fixture = arg;

	close(fixture->fd);
	fixture->fd = -1;
}

ZTEST_SUITE(eventfd, NULL, setup, before, after, NULL);
