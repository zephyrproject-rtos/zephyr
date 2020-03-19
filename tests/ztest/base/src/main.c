/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

static void test_empty_test(void)
{
}

static void test_assert_tests(void)
{
	ztest_true(1, NULL);
	ztest_false(0, NULL);
	ztest_is_null(NULL, NULL);
	ztest_not_null("foo", NULL);
	ztest_equal(1, 1, NULL);
	ztest_equal_ptr(NULL, NULL, NULL);
}

static void test_assert_mem_equal(void)
{
	static const uint32_t expected[4] = {
		0x1234,
		0x5678,
		0x9ABC,
		0xDEF0
	};
	uint32_t actual[4] = {0};
	memcpy(actual, expected, sizeof(actual));
	ztest_mem_equal(actual, expected, sizeof(expected), NULL);
}

void test_main(void)
{
	ztest_test_suite(framework_tests,
			 ztest_unit_test(test_empty_test),
			 ztest_unit_test(test_assert_tests),
			 ztest_unit_test(test_assert_mem_equal)
			 );

	ztest_run_test_suite(framework_tests);
}
