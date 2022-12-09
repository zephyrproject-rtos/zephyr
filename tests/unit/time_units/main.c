/*
 * Copyright 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_z_tmcvt_for_overflow(void);

void test_main(void)
{
	ztest_test_suite(test_time_units, ztest_unit_test(test_z_tmcvt_for_overflow));
	ztest_run_test_suite(test_time_units);
}
