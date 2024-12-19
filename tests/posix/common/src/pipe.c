/*
 * Copyright (c) Bruce ROSIER
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* Test case: Create and close pipes */
ZTEST(posix_pipe, test_create_close_pipe)
{
	int fds[2];
	int ret;

	ret = pipe(fds);
	zassert_true(ret == 0, "pipe creation failed");

	zassert_true(fds[0] >= 0, "read descriptor is invalid");
	zassert_true(fds[1] >= 0, "write descriptor is invalid");

	close(fds[0]);
	close(fds[1]);

	ret = close(fds[0]);
	zassert_true(ret == -1 && errno == EBADF, "double close should fail");
}

/* Test case: Blocking read and write */
ZTEST(posix_pipe, test_blocking_read_write)
{
	int fds[2];
	char write_data[] = "Hello, Zephyr!";
	char read_buffer[20];
	ssize_t n;

	zassert_true(pipe(fds) == 0, "pipe creation failed");

	n = write(fds[1], write_data, sizeof(write_data));
	zassert_true(n == sizeof(write_data), "write failed");

	n = read(fds[0], read_buffer, sizeof(write_data));
	zassert_true(n == sizeof(write_data), "read failed");

	zassert_mem_equal(write_data, read_buffer, sizeof(write_data), "data mismatch");

	close(fds[0]);
	close(fds[1]);
}

/* Test case: Non-blocking read/write with O_NONBLOCK */
ZTEST(posix_pipe, test_nonblocking_read_write)
{
	int fds[2];
	char write_data[] = "Test non-blocking";
	char read_buffer[20];
	ssize_t n;
	int flags;

	zassert_true(pipe(fds) == 0, "pipe creation failed");

	flags = fcntl(fds[0], F_GETFL, 0);
	fcntl(fds[0], F_SETFL, flags | O_NONBLOCK);

	n = read(fds[0], read_buffer, sizeof(read_buffer));
	zassert_true(n == -1 && errno == EAGAIN, "read from empty pipe should return EAGAIN");

	n = write(fds[1], write_data, sizeof(write_data));
	zassert_true(n == sizeof(write_data), "non-blocking write failed");

	n = read(fds[0], read_buffer, sizeof(write_data));
	zassert_true(n == sizeof(write_data), "non-blocking read failed");

	close(fds[0]);
	close(fds[1]);
}

/* Test case: Pipe full scenario */
ZTEST(posix_pipe, test_pipe_full)
{
	int fds[2];
	char write_data[_POSIX_PIPE_BUF + 1];
	ssize_t n;

	zassert_true(pipe(fds) == 0, "pipe creation failed");

	memset(write_data, 'A', _POSIX_PIPE_BUF);
	n = write(fds[1], write_data, _POSIX_PIPE_BUF);
	zassert_true(n == _POSIX_PIPE_BUF, "write to pipe failed");

	fcntl(fds[1], F_SETFL, O_NONBLOCK);
	n = write(fds[1], write_data, 1);
	zassert_true(n == -1 && errno == EAGAIN, "pipe should be full");

	close(fds[0]);
	close(fds[1]);
}

/* Test case: Pipe multiple readers and writers */
ZTEST(posix_pipe, test_multiple_readers_writers)
{
	int fds[2];
	char write_data1[] = "Writer 1";
	char write_data2[] = "Writer 2";
	char read_buffer[20];
	ssize_t n;

	zassert_true(pipe(fds) == 0, "pipe creation failed");

	n = write(fds[1], write_data1, sizeof(write_data1));
	zassert_true(n == sizeof(write_data1), "write 1 failed");

	n = write(fds[1], write_data2, sizeof(write_data2));
	zassert_true(n == sizeof(write_data2), "write 2 failed");

	n = read(fds[0], read_buffer, sizeof(write_data1));
	zassert_true(n == sizeof(write_data1), "read 1 failed");
	zassert_mem_equal(write_data1, read_buffer, sizeof(write_data1), "data 1 mismatch");

	n = read(fds[0], read_buffer, sizeof(write_data2));
	zassert_true(n == sizeof(write_data2), "read 2 failed");
	zassert_mem_equal(write_data2, read_buffer, sizeof(write_data2), "data 2 mismatch");

	close(fds[0]);
	close(fds[1]);
}

ZTEST_SUITE(posix_pipe, NULL, NULL, NULL, NULL, NULL);
