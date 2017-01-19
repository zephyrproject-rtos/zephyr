/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

static void assert_tests(void)
{
	assert_true(1, "1 was false");
	assert_false(0, "0 was true");
	assert_is_null(NULL, "NULL was not NULL");
	assert_not_null("foo", "\"foo\" was NULL");
	assert_equal(1, 1, "1 was not equal to 1");
	assert_equal_ptr(NULL, NULL, "NULL was not equal to NULL");
}

void test_main(void)
{
	ztest_test_suite(framework_tests,
		ztest_unit_test(assert_tests)
	);

	ztest_run_test_suite(framework_tests);
}
