/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <syscall_handler.h>
#include <ztest.h>
#include "test_syscalls.h"

#define BUF_SIZE	32

__kernel char kernel_string[BUF_SIZE];
__kernel char kernel_buf[BUF_SIZE];
char user_string[BUF_SIZE];

size_t _impl_string_nlen(char *src, size_t maxlen, int *err)
{
	return z_user_string_nlen(src, maxlen, err);
}

Z_SYSCALL_HANDLER(string_nlen, src, maxlen, err)
{
	int err_copy;
	size_t ret;

	ret = _impl_string_nlen((char *)src, maxlen, &err_copy);
	if (!err_copy && Z_SYSCALL_MEMORY_READ(src, ret + 1)) {
		err_copy = -1;
	}

	Z_OOPS(z_user_to_copy((int *)err, &err_copy, sizeof(err_copy)));

	return ret;
}

int _impl_string_alloc_copy(char *src)
{
	if (!strcmp(src, kernel_string)) {
		return 0;
	} else {
		return -2;
	}
}

Z_SYSCALL_HANDLER(string_alloc_copy, src)
{
	char *src_copy;
	int ret;

	src_copy = z_user_string_alloc_copy((char *)src, BUF_SIZE);
	if (!src_copy) {
		return -1;
	}

	ret = _impl_string_alloc_copy(src_copy);
	k_free(src_copy);

	return ret;
}

int _impl_string_copy(char *src)
{
	if (!strcmp(src, kernel_string)) {
		return 0;
	} else {
		return ESRCH;
	}
}

Z_SYSCALL_HANDLER(string_copy, src)
{
	int ret = z_user_string_copy(kernel_buf, (char *)src, BUF_SIZE);

	if (ret) {
		return ret;
	}

	return _impl_string_copy(kernel_buf);
}

/* Not actually used, but will copy wrong string if called by mistake instead
 * of the handler
 */
int _impl_to_copy(char *dest)
{
	memcpy(dest, kernel_string, BUF_SIZE);
	return 0;
}

Z_SYSCALL_HANDLER(to_copy, dest)
{
	return z_user_to_copy((char *)dest, user_string, BUF_SIZE);
}

/**
 * @brief Test to demonstrate usage of z_user_string_nlen()
 *
 * @details The test will be called from user mode and kernel
 * mode to check the behavior of z_user_string_nlen()
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see z_user_string_nlen()
 */
void test_string_nlen(void)
{
	int err;
	size_t ret;

	ret = string_nlen(kernel_string, BUF_SIZE, &err);
	if (_arch_is_user_context()) {
		zassert_equal(err, -1,
			      "kernel string did not fault on user access");
	} else {
		zassert_equal(err, 0, "kernel string faulted in kernel mode");
		zassert_equal(ret, strlen(kernel_string),
			      "incorrect length returned");
	}

	/* Valid usage */
	ret = string_nlen(user_string, BUF_SIZE, &err);
	zassert_equal(err, 0, "user string faulted");
	zassert_equal(ret, strlen(user_string), "incorrect length returned");

	/* Try to blow up the kernel */
	ret = string_nlen((char *)0xFFFFFFF0, BUF_SIZE, &err);
	zassert_equal(err, -1, "nonsense string address did not fault");
}

/**
 * @brief Test to verify syscall for string alloc copy
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see z_user_string_alloc_copy(), strcmp()
 */
void test_user_string_alloc_copy(void)
{
	int ret;

	ret = string_alloc_copy("asdkajshdazskjdh");
	zassert_equal(ret, -2, "got %d", ret);

	ret = string_alloc_copy(
	    "asdkajshdazskjdhikfsdjhfskdjfhsdkfjhskdfjhdskfjhs");
	zassert_equal(ret, -1, "got %d", ret);

	ret = string_alloc_copy(kernel_string);
	zassert_equal(ret, -1, "got %d", ret);

	ret = string_alloc_copy("this is a kernel string");
	zassert_equal(ret, 0, "string should have matched");
}

/**
 * @brief Test sys_call for string copy
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see z_user_string_copy(), strcmp()
 */
void test_user_string_copy(void)
{
	int ret;

	ret = string_copy("asdkajshdazskjdh");
	zassert_equal(ret, ESRCH, "got %d", ret);

	ret = string_copy("asdkajshdazskjdhikfsdjhfskdjfhsdkfjhskdfjhdskfjhs");
	zassert_equal(ret, EINVAL, "got %d", ret);

	ret = string_copy(kernel_string);
	zassert_equal(ret, EFAULT, "got %d", ret);

	ret = string_copy("this is a kernel string");
	zassert_equal(ret, 0, "string should have matched");
}

/**
 * @brief Test to demonstrate system call for copy
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see memcpy(), z_user_to_copy()
 */
void test_to_copy(void)
{
	char buf[BUF_SIZE];
	int ret;

	ret = to_copy(kernel_buf);
	zassert_equal(ret, EFAULT, "should have faulted");

	ret = to_copy(buf);
	zassert_equal(ret, 0, "copy should have been a success");
	ret = strcmp(buf, user_string);
	zassert_equal(ret, 0, "string should have matched");
}

K_MEM_POOL_DEFINE(test_pool, BUF_SIZE, BUF_SIZE, 4, 4);

void test_main(void)
{
	sprintf(kernel_string, "this is a kernel string");
	sprintf(user_string, "this is a user string");
	k_thread_resource_pool_assign(k_current_get(), &test_pool);

	ztest_test_suite(syscalls,
			 ztest_unit_test(test_string_nlen),
			 ztest_user_unit_test(test_string_nlen),
			 ztest_user_unit_test(test_to_copy),
			 ztest_user_unit_test(test_user_string_copy),
			 ztest_user_unit_test(test_user_string_alloc_copy));
	ztest_run_test_suite(syscalls);
}
