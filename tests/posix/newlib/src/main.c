/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_posix_newlib(void);

void test_main(void)
{
	ztest_test_suite(posix_newlib_test,
			ztest_unit_test(test_posix_newlib)
			);
	ztest_run_test_suite(posix_newlib_test);
}
