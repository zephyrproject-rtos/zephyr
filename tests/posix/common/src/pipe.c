/*
 * Copyright (c) 2019 Linaro Limited
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <unistd.h>

#include <ztest.h>
#include <ztest_assert.h>

#include <sys/util.h>

void test_posix_pipe_read_write(void)
{
	int res;
	int fildes[2];

	const char *expected_msg = "Hello, pipe(2) world!";
	char actual_msg[32];

	res = pipe(fildes);
	zassert_true(res == -1 || res == 0, "pipe returned an unspecified value");
	zassert_equal(res, 0, "pipe failed");

	res = write(fildes[1], expected_msg, strlen(expected_msg));
	zassert_equal(res, strlen(expected_msg), "did not write entire message");

	memset(actual_msg, 0, sizeof(actual_msg));
	res = read(fildes[0], actual_msg, sizeof(actual_msg));
	zassert_not_equal(res, -1, "read(2) encountered an error");
	zassert_equal(res, strlen(expected_msg), "wrong return value");

	res = close(fildes[0]);
	zassert_equal(res, 0, "close failed");

	res = close(fildes[1]);
	zassert_equal(res, 0, "close failed");

	zassert_true(0 == strncmp(expected_msg, actual_msg,
		MIN(strlen(expected_msg), sizeof(actual_msg) - 1)),
		"the wrong message was passed through the pipe");

	/* This is not a POSIX requirement, but should be safe */
	res = pipe(NULL);
	zassert_equal(res, -1, "pipe should fail when passed an invalid pointer");
	zassert_equal(errno, EFAULT, "errno should be EFAULT with invalid pointer");
}

void test_posix_pipe_select(void)
{
}

void test_posix_pipe_poll(void)
{
}
