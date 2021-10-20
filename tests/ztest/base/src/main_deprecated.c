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
	zassert_true(1, NULL);
	zassert_false(0, NULL);
	zassert_is_null(NULL, NULL);
	zassert_not_null("foo", NULL);
	zassert_equal(1, 1, NULL);
	zassert_equal_ptr(NULL, NULL, NULL);
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
	zassert_mem_equal(actual, expected, sizeof(expected), NULL);
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
