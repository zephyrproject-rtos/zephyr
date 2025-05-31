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
	zassert_not_null(fdopen(1, "r"));
}

ZTEST(posix_device_io, test_fileno)
{
#define DECL_TDATA(_stream, _fd)                                                                   \
	{                                                                                          \
		.stream_name = #_stream,                                                           \
		.stream = _stream,                                                                 \
		.fd_name = #_fd,                                                                   \
		.fd = _fd,                                                                         \
	}

	const struct fileno_test_data {
		const char *stream_name;
		FILE *stream;
		const char *fd_name;
		int fd;
	} test_data[] = {
		DECL_TDATA(stdin, STDIN_FILENO),
		DECL_TDATA(stdout, STDOUT_FILENO),
		DECL_TDATA(stderr, STDERR_FILENO),
	};

	ARRAY_FOR_EACH(test_data, i) {
		FILE *stream = test_data[i].stream;
		int expect_fd = test_data[i].fd;
		int actual_fd = fileno(stream);

		if ((i == STDERR_FILENO) &&
		    (IS_ENABLED(CONFIG_PICOLIBC) || IS_ENABLED(CONFIG_NEWLIB_LIBC))) {
			TC_PRINT("Note: stderr not enabled\n");
			continue;
		}

		zexpect_equal(actual_fd, expect_fd, "fileno(%s) (%d) != %s (%d)",
			      test_data[i].stream_name, actual_fd, test_data[i].fd_name, expect_fd);
	}
}

ZTEST(posix_device_io, test_open)
{
	/*
	 * Note: open() is already exercised extensively in tests/posix/fs, but we should test it
	 * here on device nodes as well.
	 */
	zexpect_equal(open("/dev/null", O_RDONLY), -1);
	zexpect_equal(errno, ENOENT);
}

#if (defined(CONFIG_ZVFS_STDIO_CONSOLE) &&                                                         \
	(CONFIG_ZVFS_STDIN_BUFSIZE + CONFIG_ZVFS_STDOUT_BUFSIZE > 0))
#define STDIO_POLL
#endif

ZTEST(posix_device_io, test_poll)
{
	struct pollfd fds[] = {
		{.fd = STDIN_FILENO, .events = POLLIN},
		{.fd = STDOUT_FILENO, .events = POLLOUT},
		{.fd = STDERR_FILENO, .events = POLLOUT},
	};

	/*
	 * Note: poll() is already exercised extensively in tests/posix/eventfd, but we should test
	 * it here on device nodes as well.
	 */
#ifdef STDIO_POLL
	zexpect_equal(poll(fds, ARRAY_SIZE(fds), 0), 2);
	/* stdin shouldn't have any bytes ready to read */
	zexpect_equal(fds[0].revents, 0);
	zexpect_equal(fds[1].revents, POLLOUT);
	zexpect_equal(fds[2].revents, POLLOUT);
#else
	zexpect_equal(poll(fds, ARRAY_SIZE(fds), 0), 3);
	zexpect_equal(fds[0].revents, POLLNVAL);
	zexpect_equal(fds[1].revents, POLLNVAL);
	zexpect_equal(fds[2].revents, POLLNVAL);
#endif
}

ZTEST(posix_device_io, test_pread)
{
	uint8_t buf[8];

	/* stdio is non-seekable */
	zexpect_equal(pread(STDIN_FILENO, buf, sizeof(buf), 0), -1);
	zexpect_equal(errno, ESPIPE);
}

ZTEST(posix_device_io, test_pselect)
{
	fd_set readfds;
	fd_set writefds;
	struct timespec timeout = {.tv_sec = 0, .tv_nsec = 0};

	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);
	FD_ZERO(&writefds);
	FD_SET(STDOUT_FILENO, &writefds);
	FD_SET(STDERR_FILENO, &writefds);

#ifdef STDIO_POLL
	zexpect_equal(pselect(STDERR_FILENO + 1, &readfds, &writefds, NULL, &timeout, NULL), 2);
	zassert_false(FD_ISSET(STDIN_FILENO, &readfds));
	zassert_true(FD_ISSET(STDOUT_FILENO, &writefds));
	zassert_true(FD_ISSET(STDERR_FILENO, &writefds));
#else
	zexpect_equal(pselect(STDERR_FILENO + 1, &readfds, &writefds, NULL, &timeout, NULL), -1);
	zassert_equal(errno, EBADF);
#endif
}

ZTEST(posix_device_io, test_pwrite)
{
	/* stdio is non-seekable */
	zexpect_equal(pwrite(STDOUT_FILENO, "x", 1, 0), -1);
	zexpect_equal(errno, ESPIPE, "%d", errno);
}

ZTEST(posix_device_io, test_select)
{
	fd_set readfds;
	fd_set writefds;
	struct timeval timeout = {.tv_sec = 0, .tv_usec = 0};

	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);
	FD_ZERO(&writefds);
	FD_SET(STDOUT_FILENO, &writefds);
	FD_SET(STDERR_FILENO, &writefds);

#ifdef STDIO_POLL
	zassert_equal(select(STDERR_FILENO + 1, &readfds, &writefds, NULL, &timeout), 2);
	zassert_false(FD_ISSET(STDIN_FILENO, &readfds));
	zassert_true(FD_ISSET(STDOUT_FILENO, &writefds));
	zassert_true(FD_ISSET(STDERR_FILENO, &writefds));
#else
	zassert_equal(select(STDERR_FILENO + 1, &readfds, &writefds, NULL, &timeout), -1);
	zassert_equal(errno, EBADF);
#endif
}

ZTEST(posix_device_io, test_write)
{
	static const char msg[] = "Hello world!\n";

#if defined(CONFIG_ZVFS_STDIO_CONSOLE) || defined(CONFIG_ARCMWDT_LIBC) ||                          \
	defined(CONFIG_MINIMAL_LIBC) || defined(CONFIG_NEWLIB_LIBC) || defined(CONFIG_PICOLIBC)
	zexpect_equal(write(STDOUT_FILENO, msg, ARRAY_SIZE(msg)), ARRAY_SIZE(msg));
#else
	zexpect_equal(write(STDOUT_FILENO, msg, ARRAY_SIZE(msg)), 0);
#endif
}

ZTEST_SUITE(posix_device_io, NULL, NULL, NULL, NULL, NULL);
