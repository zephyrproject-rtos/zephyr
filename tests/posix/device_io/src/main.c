/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/ztest.h>

#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/poll.h>
#include <zephyr/posix/sys/select.h>
#include <zephyr/posix/unistd.h>

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

ZTEST(posix_device_io, test_FD_CLR)
{
	fd_set fds;

	FD_CLR(0, &fds);
}

ZTEST(posix_device_io, test_FD_ISSET)
{
	fd_set fds;

	zassert_false(FD_ISSET(0, &fds));
}

ZTEST(posix_device_io, test_FD_SET)
{
	fd_set fds;

	FD_SET(0, &fds);
	zassert_true(FD_ISSET(0, &fds));
}

ZTEST(posix_device_io, test_FD_ZERO)
{
	fd_set fds;

	FD_ZERO(&fds);
	zassert_false(FD_ISSET(0, &fds));
}

ZTEST(posix_device_io, test_close)
{
	zassert_not_ok(close(-1));
}

ZTEST(posix_device_io, test_fdopen)
{
	zassert_not_ok(fdopen(1, "r"));
}

ZTEST(posix_device_io, test_fileno)
{
	zassert_equal(fileno(stdout), STDOUT_FILENO);
}

ZTEST(posix_device_io, test_open)
{
	zassert_equal(open("/dev/null", O_RDONLY), -1);
}

ZTEST(posix_device_io, test_poll)
{
	struct pollfd fds[1] = {{.fd = STDIN_FILENO, .events = POLLIN}};

	zassert_ok(poll(fds, ARRAY_SIZE(fds), 0));
}

ZTEST(posix_device_io, test_pread)
{
	uint8_t buf[8];

	zassert_ok(pread(STDIN_FILENO, buf, sizeof(buf), 0));
}

ZTEST(posix_device_io, test_pselect)
{
	fd_set fds;
	struct timespec timeout = {.tv_sec = 0, .tv_nsec = 0};

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);

	zassert_ok(pselect(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout, NULL));
}

ZTEST(posix_device_io, test_pwrite)
{
	zassert_ok(pwrite(STDOUT_FILENO, "x", 1, 0));
}

ZTEST(posix_device_io, test_read)
{
	uint8_t buf[8];

	zassert_ok(read(STDIN_FILENO, buf, sizeof(buf)));
}

ZTEST(posix_device_io, test_select)
{
	fd_set fds;
	struct timeval timeout = {.tv_sec = 0, .tv_usec = 0};

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);

	zassert_ok(select(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout));
}

ZTEST(posix_device_io, test_write)
{
	zassert_ok(pwrite(STDOUT_FILENO, "x", 1, 0));
}

ZTEST_SUITE(posix_device_io, NULL, NULL, NULL, NULL, NULL);
