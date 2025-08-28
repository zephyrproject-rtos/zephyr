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

/*
 * FIXME: this corrupts stdin for some reason
ZTEST(posix_device_io, test_fdopen)
{
	zassert_not_null(fdopen(1, "r"));
}
*/

ZTEST(posix_device_io, test_fileno)
{
#define DECL_TDATA(_stream, _fd)                                                                   \
	{.stream_name = #_stream, .stream = _stream, .fd_name = #_fd, .fd = _fd}

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

ZTEST(posix_device_io, test_poll)
{
	struct pollfd fds[1] = {{.fd = STDIN_FILENO, .events = POLLIN}};

	/*
	 * Note: poll() is already exercised extensively in tests/posix/eventfd, but we should test
	 * it here on device nodes as well.
	 */
	zexpect_equal(poll(fds, ARRAY_SIZE(fds), 0), 1);
}

ZTEST(posix_device_io, test_pread)
{
	uint8_t buf[8];

	zexpect_equal(pread(STDIN_FILENO, buf, sizeof(buf), 0), -1);
	zexpect_equal(errno, ENOTSUP);
}

ZTEST(posix_device_io, test_pselect)
{
	fd_set fds;
	struct timespec timeout = {.tv_sec = 0, .tv_nsec = 0};

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);

	/* Zephyr does not yet support select or poll on stdin */
	zexpect_equal(pselect(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout, NULL), -1);
	zexpect_equal(errno, EBADF);
}

ZTEST(posix_device_io, test_pwrite)
{
	/* Zephyr does not yet support writing through a file descriptor */
	zexpect_equal(pwrite(STDOUT_FILENO, "x", 1, 0), -1);
	zexpect_equal(errno, ENOTSUP, "%d", errno);
}

ZTEST(posix_device_io, test_read)
{
	uint8_t buf[8];

	/* reading from stdin does not work in Zephyr */
	zassert_equal(read(STDIN_FILENO, buf, sizeof(buf)), 0);
}

ZTEST(posix_device_io, test_select)
{
	fd_set fds;
	struct timeval timeout = {.tv_sec = 0, .tv_usec = 0};

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);

	/* Zephyr does not yet support select or poll on stdin */
	zassert_equal(select(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout), -1);
	zexpect_equal(errno, EBADF, "%d", errno);
}

ZTEST(posix_device_io, test_write)
{
	zexpect_equal(write(STDOUT_FILENO, "x", 1), 1);
}

ZTEST_SUITE(posix_device_io, NULL, NULL, NULL, NULL, NULL);
