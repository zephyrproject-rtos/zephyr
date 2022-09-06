/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#define ztest_unreachable	zassert_unreachable
#define ztest_true		API_TYPE(true)
#define ztest_false		API_TYPE(false)
#define ztest_ok		API_TYPE(ok)
#define ztest_is_null		API_TYPE(is_null)
#define ztest_not_null		API_TYPE(not_null)
#define ztest_equal		API_TYPE(equal)
#define ztest_not_equal		API_TYPE(not_equal)
#define ztest_not_equal		API_TYPE(not_equal)
#define ztest_within		API_TYPE(within)
#define ztest_between_inclusive	API_TYPE(between_inclusive)
#define ztest_mem_equal		API_TYPE(mem_equal)

ZTEST_EXPECT_FAIL(SUITE_NAME, test_unreachable);
ZTEST(SUITE_NAME, test_unreachable)
{
	ztest_unreachable();
}

ZTEST_EXPECT_FAIL_OR_SKIP(SUITE_NAME, test_true);
ZTEST(SUITE_NAME, test_true)
{
	bool var = false;
	ztest_true(var);
}

ZTEST_EXPECT_FAIL_OR_SKIP(SUITE_NAME, test_false);
ZTEST(SUITE_NAME, test_false)
{
	bool var = true;
	ztest_false(var);
}

ZTEST_EXPECT_FAIL_OR_SKIP(SUITE_NAME, test_ok);
ZTEST(SUITE_NAME, test_ok)
{
	int res = -EIO;
	ztest_ok(res);
}

ZTEST_EXPECT_FAIL_OR_SKIP(SUITE_NAME, test_is_null);
ZTEST(SUITE_NAME, test_is_null)
{
	int var = 2;
	int *var_ptr = &var;
	ztest_is_null(var_ptr);
}

ZTEST_EXPECT_FAIL_OR_SKIP(SUITE_NAME, test_not_null);
ZTEST(SUITE_NAME, test_not_null)
{
	int *var_ptr = NULL;
	ztest_not_null(var_ptr);
}

ZTEST_EXPECT_FAIL_OR_SKIP(SUITE_NAME, test_equal);
ZTEST(SUITE_NAME, test_equal)
{
	bool var = true;
	bool not_var = !var;
	ztest_equal(var, not_var);
}

ZTEST_EXPECT_FAIL_OR_SKIP(SUITE_NAME, test_not_equal);
ZTEST(SUITE_NAME, test_not_equal)
{
	bool var = true;
	bool var_copy = var;
	ztest_not_equal(var, var_copy);
}

ZTEST_EXPECT_FAIL_OR_SKIP(SUITE_NAME, test_equal_ptr);
ZTEST(SUITE_NAME, test_equal_ptr)
{
	int v = 5;
	int *ptr1 = &v;
	int *ptr2 = &v;
	ztest_not_equal(ptr1, ptr2);
}

ZTEST_EXPECT_FAIL_OR_SKIP(SUITE_NAME, test_within);
ZTEST(SUITE_NAME, test_within)
{
	int v = 5;
	ztest_within(v, 10, 4);
}

ZTEST_EXPECT_FAIL_OR_SKIP(SUITE_NAME, test_between_inclusive);
ZTEST(SUITE_NAME, test_between_inclusive)
{
	int v = 5;
	ztest_between_inclusive(v, 6, 2);
}

ZTEST_EXPECT_FAIL_OR_SKIP(SUITE_NAME, test_mem_equal);
ZTEST(SUITE_NAME, test_mem_equal)
{
	char buf1[] = { 0, 1, 2, 3, 4, 5 };
	char buf2[] = { 5, 4, 3, 2, 1, 0 };
	ztest_mem_equal(buf1, buf2, sizeof(buf1));
}
