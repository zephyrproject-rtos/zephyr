/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ztest_error_hook.h>
#include <toolchain.h>
#include <stdio.h>
#include <stddef.h>

extern int _gettimeofday(struct timeval *__tp, void *__tzp);
extern int *__errno(void);
extern void __chk_fail(void);
extern void *_sbrk(intptr_t count);
extern int _read(int fd, char *buf, int nbytes);
extern int _write(int fd, const void *buf, int nbytes);
extern int _open(const char *name, int mode);
extern int _close(int file);
extern int _lseek(int file, int ptr, int dir);
extern int _isatty(int file);
extern int _kill(int i, int j);
extern int _getpid(void);
extern int _fstat(int file, struct stat *st);
extern void _exit(int status);
extern void __stdin_hook_install(unsigned char (*hook)(void));
extern int _stdout_hook_default(int c);

#define TEST_INPUT 1

/**
 * @brief Test newlib apis with CONFIG_POSIX_API configuration
 *
 * @details Verify the apis which are implemented in libc/newlib/.
 * And check it could be invoked which is default implemented with
 * different input.
 *
 * @see _gettimeofday(), __errno(), _isatty(), _kill()
 * _getpid(), _fstat()
 */
void test_newlib_api(void)
{
#ifndef CONFIG_POSIX_API
	ztest_test_skip();
#else
	struct timeval tp;
	int *ptr;
	int ret;
	struct stat st;

	ret = _gettimeofday(&tp, NULL);
	zassert_equal(ret, 0, NULL);

	ptr = __errno();
	zassert_equal(*ptr, 0, NULL);

	ret = _isatty(12);
	zassert_false(ret, NULL);

	ret = _isatty(-12);
	zassert_true(ret, NULL);

	ret = _isatty(0);
	zassert_true(ret, NULL);

	ret = _kill(123, 3);
	zassert_equal(ret, 0, NULL);

	ret = _kill(-123, 3);
	zassert_equal(ret, 0, NULL);

	ret = _kill(123, -3);
	zassert_equal(ret, 0, NULL);

	ret = _kill(-123, -3);
	zassert_equal(ret, 0, NULL);

	ret = _getpid();
	zassert_equal(ret, 0, NULL);

	ret = _fstat(13, &st);
	zassert_equal(ret, 0, NULL);

	ret = _fstat(-13, &st);
	zassert_equal(ret, 0, NULL);

	ztest_set_fault_valid(true);
	ret = _fstat(-13, NULL);
	zassert_equal(ret, 0, NULL);
#endif
}

/**
 * @brief Test _sbrk API
 *
 * @details Invoke _sbrk to check newlib api behavior is expected
 * or not. Give it with normal input and error input to verify
 * this API.
 *
 * @see _sbrk().
 */
void test_sbrk_error(void)
{
	intptr_t count;
	void *p;
	int ret;

	/* Invoke _sbrk with input less that MAX_HEAP_SIZE
	 * Verify the return value is expected
	 */
	count = 1;
	p = _sbrk(count);
	ret = *(int *)p;
	zassert_equal(ret, 0, NULL);


	/* The Max heap size are different on different platforms
	 * The count of value(0x1fffff) is work on qemu_x86 to
	 * verify the condition of input more than max heap size
	 */
#ifdef CONFIG_BOARD_QEMU_X86
	/* Use the count more than max heap size */
	ztest_set_fault_valid(true);
	count = 0x1fffff;
	p = _sbrk(count);
	ret = *(int *)p;
	zassert_equal(ret, -1, NULL);
#endif
}

/**
 * @brief Test __chk_fail API
 *
 * @details Invoke __chk_fail directly to check whether
 * it will cause fatal error.
 *
 * @see __chk_fail()
 */
void test_chk_fail(void)
{
	/* __chk_fail will invoke z_oops(), should enable this framework */
	ztest_set_fault_valid(true);
	__chk_fail();
	ztest_test_fail();
}

#ifdef CONFIG_POSIX_API
void test_newlib_read(void)
{
	ztest_test_skip();
}
#else
/**
 * @brief Test _read of newlib API
 *
 * @details Invoke _read directly to check whether
 * it will be invoked from the default implementation
 * with different input.
 *
 * @see _read()
 */
void test_newlib_read(void)
{
	int fd, byte, ret;
	char *buf;
	char str[3] = {'\n', '\r', 'c'};

	fd = -1;
	byte = -1;
	buf = str;
	ret = _read(fd, buf, byte);
	zassert_equal(ret, 0, NULL);

	fd = 3;
	byte = 2;
	buf = str;
	ret = _read(fd, buf, byte);
	zassert_equal(ret, 2, NULL);

	buf = NULL;
	ztest_set_fault_valid(true);
	ret = _read(fd, buf, byte);
}
#endif

#ifdef CONFIG_POSIX_API
void test_newlib_no_posix_config(void)
{
	ztest_test_skip();
}
#else

/**
 * @brief Test newlib APIs without CONFIG_POSIX_API configuration
 *
 * @details Invoke newlib APIs directly which are implemented
 * in libc/newlib/. And verify those apis with different input
 * when the CONFIG_POSIX_API is disable.
 *
 * @see _open(), _close(), _lseek()
 */
void test_newlib_no_posix_config(void)
{
	int ret;
	int file;
	char *name;
	char str[1];

	/* Normal usage */
	name = str;
	file = 1;
	ret = _open(name, 'w');
	zassert_equal(ret, -1, NULL);

	ret = _close(file);
	zassert_equal(ret, -1, NULL);

	ret = _lseek(file, 123, TEST_INPUT);
	zassert_equal(ret, 0, NULL);

	/* Control input with different error param */
	ret = _lseek(file, -1, TEST_INPUT);
	zassert_equal(ret, 0, NULL);

	ret = _lseek(file, 123, -1);
	zassert_equal(ret, 0, NULL);

	ret = _open(NULL, 'w');
	zassert_equal(ret, -1, NULL);

	file = -1;
	ret = _lseek(file, 123, TEST_INPUT);
	zassert_equal(ret, 0, NULL);

	ret = _lseek(file, -1, TEST_INPUT);
	zassert_equal(ret, 0, NULL);

	ret = _lseek(file, 123, -1);
	zassert_equal(ret, 0, NULL);

	ret = _lseek(file, -1, -1);
	zassert_equal(ret, 0, NULL);

	name = NULL;
	ret = _open(name, 0);
	zassert_equal(ret, -1, NULL);

	ret = _open(name, -1);
	zassert_equal(ret, -1, NULL);

	ret = _close(file);
	zassert_equal(ret, -1, NULL);
}
#endif
static unsigned char hook_install(void)
{
	return 0;
}

/**
 * @brief Test __stdin_hook_install api
 *
 * @details Invoke newlib hook install directly which are implemented
 * in libc/newlib/. And verify it could be invoked which is default
 * implemented.
 *
 * @see __stdin_hook_install()
 */
void test_hook_install(void)
{
	__stdin_hook_install(hook_install);
}

void test_main(void)
{
	ztest_test_suite(test_newlib_hooks,
			ztest_unit_test(test_sbrk_error),
			ztest_unit_test(test_chk_fail),
			ztest_unit_test(test_newlib_api),
			ztest_unit_test(test_newlib_read),
			ztest_unit_test(test_newlib_no_posix_config),
			ztest_unit_test(test_hook_install)
			);
	ztest_run_test_suite(test_newlib_hooks);

}
