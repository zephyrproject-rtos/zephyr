/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
  #error test can only run on Cortex-M3/M4/M7/M33 MCUs
#endif

#include <ztest.h>

extern void test_arm_zero_latency_irqs(void);

void test_main(void)
{
	ztest_test_suite(zero_latency_irqs,
		ztest_unit_test(test_arm_zero_latency_irqs));
	ztest_run_test_suite(zero_latency_irqs);
}
