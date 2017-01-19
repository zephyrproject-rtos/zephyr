/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

static void empty_test(void)
{
}

static void assert_tests(void)
{
	assert_true(1, NULL);
	assert_false(0, NULL);
	assert_is_null(NULL, NULL);
	assert_not_null("foo", NULL);
	assert_equal(1, 1, NULL);
	assert_equal_ptr(NULL, NULL, NULL);
}

void test_main(void)
{
	ztest_test_suite(framework_tests,
			 ztest_unit_test(empty_test),
			 ztest_unit_test(assert_tests)
			 );

	ztest_run_test_suite(framework_tests);
}
