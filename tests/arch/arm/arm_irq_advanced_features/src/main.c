/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_arm_zero_latency_irqs(void);
extern void test_arm_dynamic_direct_interrupts(void);
extern void test_arm_irq_target_state(void);

void test_main(void)
{
	ztest_test_suite(arm_irq_advanced_features,
		ztest_unit_test(test_arm_dynamic_direct_interrupts),
		ztest_unit_test(test_arm_zero_latency_irqs),
		ztest_unit_test(test_arm_irq_target_state));
	ztest_run_test_suite(arm_irq_advanced_features);
}
