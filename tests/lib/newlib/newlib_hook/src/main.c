/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/libc-hooks.h>

/* define the function prototype for sbrk */
void *sbrk(ptrdiff_t __incr);

/**
 * @brief Verify _open() stub in newlib libc-hooks.c
 *
 * @details The default weak function return -1.
 */
void test_newlib_stub_open(void)
{
	int fd;

	fd = open("../prj.conf", O_RDWR | O_CREAT, 0664);
	zassert_equal(fd, -1, "open an invalid fd");
}

/**
 * @brief Verify _close() stub in newlib libc-hooks.c
 *
 * @details The default weak function return -1.
 */
void test_newlib_stub_close(void)
{
	int ret = 0;

	ret = close(-1);
	zassert_equal(ret, -1, "closed an invalid fd");
}

/**
 * @brief Verify _getpid() stub in newlib libc-hooks.c
 *
 * @details The default weak function return 0.
 */
void test_newlib_stub_getpid(void)
{
	pid_t pid;

	pid = getpid();
	zassert_equal(pid, 0, "getpid failed");
}

/**
 * @brief Verify _fstat() stub in newlib libc-hooks.c
 *
 * @details The default weak function return 0.
 */
void test_newlib_stub_fstat(void)
{
	struct stat status;
	int ret;

	ret = fstat(-1, &status);
	zassert_equal(ret, 0, "fstat failed");
}

/**
 * @brief Verify _kill() stub in newlib libc-hooks.c
 *
 * @details The default weak function return 0.
 */
void test_newlib_stub_kill(void)
{
	int ret;

	ret = raise(SIGILL);
	zassert_equal(ret, 0, "kill failed");
}

/**
 * @brief Verify _read() stub in newlib libc-hooks.c
 *
 * @details Use read() functions to test _read().
 */
void test_newlib_stub_read(void)
{
	int ret;
	char buf;

	ret = read(-1, &buf, 1);
	zassert_not_equal(ret, 0, "read failed");
}

/**
 * @brief Verify _write() stub in newlib libc-hooks.c
 *
 * @details Use write() functions to test _write().
 */
void test_newlib_stub_write(void)
{
	int ret;
	char buf = 'a';

	ret = write(-1, &buf, 1);
	zassert_equal(ret, 1, "write failed!");
}

/**
 * @brief Verify _sbrk() stub in newlib libc-hooks.c
 *
 * @details Use sbrk() to test _sbrk().
 */
void test_newlib_stub_sbrk(void)
{
	void *ret = NULL;

	ret = sbrk(0);
	zassert_not_null(ret, "sbrk failed");
}

/**
 * @brief Verify _lseek() stub in newlib libc-hooks.c
 *
 * @details The default weak function return 0.
 */
void test_newlib_stub_lseek(void)
{
	int ret;

	ret = lseek(-1, 0, SEEK_END);
	zassert_equal(ret, 0, "lseek failed");
}

/**
 * @brief Verify _isatty() stub in newlib libc-hooks.c
 *
 * @details The default weak function return <= 2.
 */
void test_newlib_stub_isatty(void)
{
	int ret;

	ret = isatty(-1);
	zassert_true(ret <= 2, "isatty failed");
}

void test_main(void)
{
	ztest_test_suite(test_newlib_stub,
			 ztest_user_unit_test(test_newlib_stub_open),
			 ztest_user_unit_test(test_newlib_stub_close),
			 ztest_user_unit_test(test_newlib_stub_getpid),
			 ztest_user_unit_test(test_newlib_stub_fstat),
			 ztest_user_unit_test(test_newlib_stub_kill),
			 ztest_user_unit_test(test_newlib_stub_read),
			 ztest_user_unit_test(test_newlib_stub_write),
			 ztest_user_unit_test(test_newlib_stub_sbrk),
			 ztest_user_unit_test(test_newlib_stub_lseek),
			 ztest_user_unit_test(test_newlib_stub_isatty));
	ztest_run_test_suite(test_newlib_stub);
}
