/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_arm_hardfault(void);

/**test case main entry*/
void test_main(void)
{
	ztest_test_suite(arm_hardfault_validation,
		ztest_unit_test(test_arm_hardfault));
	ztest_run_test_suite(arm_hardfault_validation);
}
