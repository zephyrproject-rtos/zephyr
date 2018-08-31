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

void test_main(void)
{
	ztest_test_suite(framework_tests,
			 ztest_unit_test(test_empty_test),
			 ztest_unit_test(test_assert_tests)
			 );

	ztest_run_test_suite(framework_tests);
}
