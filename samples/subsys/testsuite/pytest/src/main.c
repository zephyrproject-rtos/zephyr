/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
void test_pytest(void)
{
	TC_PRINT("Hello world\n");
}

void test_main(void)
{
	ztest_test_suite(test_pytest,
			 ztest_unit_test(test_pytest)
			 );

	ztest_run_test_suite(test_pytest);
}
