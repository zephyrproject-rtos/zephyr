/*
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr.h>
#include <ztest.h>


static void test_stub_for_build_only_test(void)
{
	ztest_test_pass();
}

void test_main(void)
{
	ztest_test_suite(cmake_overlay_var_expansions_test_suite,
			 ztest_unit_test(test_stub_for_build_only_test));
	ztest_run_test_suite(cmake_overlay_var_expansions_test_suite);
}
