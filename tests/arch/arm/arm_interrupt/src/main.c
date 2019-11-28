/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_arm_interrupt(void);

void test_main(void)
{
	ztest_test_suite(arm_interrupt,
		ztest_unit_test(test_arm_interrupt));
	ztest_run_test_suite(arm_interrupt);
}
