/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include <zephyr/ztest.h>

ZTEST_SUITE(expect, NULL, NULL, NULL, NULL, NULL);

ZTEST_EXPECT_FAIL(expect, test_fail_later);
ZTEST(expect, test_fail_later)
{
	void *empty_ptr = NULL;
	uint32_t val = 5;

	zexpect_equal(val, 2);
	zexpect_not_equal(val, 5);

	zexpect_not_null(empty_ptr);

	zassert_true(true);
}

ZTEST(expect, test_pass_expect_true)
{
	zexpect_true(true);
}

ZTEST_EXPECT_FAIL(expect, test_fail_expect_true);
ZTEST(expect, test_fail_expect_true)
{
	zexpect_true(false);
}

ZTEST(expect, test_expect_false)
{
	zexpect_false(false);
}

ZTEST_EXPECT_FAIL(expect, test_fail_expect_false);
ZTEST(expect, test_fail_expect_false)
{
	zexpect_false(true);
}

ZTEST(expect, test_expect_ok)
{
	zexpect_ok(0);
}

ZTEST_EXPECT_FAIL(expect, test_fail_expect_ok);
ZTEST(expect, test_fail_expect_ok)
{
	zexpect_ok(5);
}

ZTEST(expect, test_expect_not_ok)
{
	zexpect_not_ok(-EIO);
}

ZTEST_EXPECT_FAIL(expect, test_fail_expect_not_ok);
ZTEST(expect, test_fail_expect_not_ok)
{
	zexpect_not_ok(0);
}

ZTEST(expect, test_expect_is_null)
{
	void *ptr = NULL;

	zexpect_is_null(ptr);
}

ZTEST_EXPECT_FAIL(expect, test_fail_expect_is_null);
ZTEST(expect, test_fail_expect_is_null)
{
	void *ptr = (void *)0x32137899;

	zexpect_is_null(ptr);
}

ZTEST(expect, test_expect_not_null)
{
	void *ptr = (void *)0x91517141;

	zexpect_not_null(ptr);
}

ZTEST_EXPECT_FAIL(expect, test_fail_expect_not_null);
ZTEST(expect, test_fail_expect_not_null)
{
	zexpect_not_null(NULL);
}

ZTEST(expect, test_expect_equal)
{
	zexpect_equal(5, 5);
}

ZTEST_EXPECT_FAIL(expect, test_fail_expect_equal);
ZTEST(expect, test_fail_expect_equal)
{
	zexpect_equal(5, 1);
}

ZTEST(expect, test_expect_not_equal)
{
	zexpect_not_equal(5, 1);
}

ZTEST_EXPECT_FAIL(expect, test_fail_expect_not_equal);
ZTEST(expect, test_fail_expect_not_equal)
{
	zexpect_not_equal(5, 5);
}

ZTEST(expect, test_expect_equal_ptr)
{
	int v = 9;
	int *a = &v;
	int *b = &v;

	zexpect_equal_ptr(a, b);
}

ZTEST_EXPECT_FAIL(expect, test_fail_expect_equal_ptr);
ZTEST(expect, test_fail_expect_equal_ptr)
{
	int v = 9;
	int *a = &v;
	int *b = NULL;

	zexpect_equal_ptr(a, b);
}

ZTEST(expect, test_expect_within)
{
	zexpect_within(7, 5, 2);
	zexpect_within(7, 7, 0);
	zexpect_within(7, 7, 3);
	zexpect_within(7, 7 + 3, 3);
}

ZTEST_EXPECT_FAIL(expect, test_fail_expect_within);
ZTEST(expect, test_fail_expect_within)
{
	zexpect_within(7, 5, 1);
}

ZTEST(expect, test_expect_between_inclusive)
{
	zexpect_between_inclusive(-5, -10, 0);

	zexpect_between_inclusive(5, 0, 10);
	zexpect_between_inclusive(0, 0, 10);
	zexpect_between_inclusive(10, 0, 10);
}

ZTEST_EXPECT_FAIL(expect, test_fail_expect_between_inclusive);
ZTEST(expect, test_fail_expect_between_inclusive)
{
	zexpect_between_inclusive(-50, -20, 30);

	zexpect_between_inclusive(5, 6, 10);
	zexpect_between_inclusive(5, 0, 4);
	zexpect_between_inclusive(5, 0, 4);
	zexpect_between_inclusive(5, 6, 10);
}
