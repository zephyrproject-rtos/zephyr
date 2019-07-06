/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_CPU_CORTEX_M)
  #error test can only run on Cortex-M MCUs
#endif

#include <ztest.h>

extern void test_arm_ramfunc(void);

void test_main(void)
{
	ztest_test_suite(arm_ramfunc,
		ztest_unit_test(test_arm_ramfunc));
	ztest_run_test_suite(arm_ramfunc);
}
