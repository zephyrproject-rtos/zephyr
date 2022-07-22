/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

static void test_assert_pass(void)
{
	ztest_test_pass();
}

static void test_assert_skip(void)
{
	ztest_test_skip();
}

/*
 * Same tests called several times to show custom handling of output.
 */
void test_main(void)
{
	ztest_test_suite(framework_tests,
		ztest_unit_test(test_assert_pass),
		ztest_unit_test(test_assert_pass),
		ztest_unit_test(test_assert_pass),
		ztest_unit_test(test_assert_skip),
		ztest_unit_test(test_assert_skip)
	);

	ztest_run_test_suite(framework_tests);
}
