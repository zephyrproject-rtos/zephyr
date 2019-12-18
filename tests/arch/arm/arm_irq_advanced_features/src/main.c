/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_arm_zero_latency_irqs(void);

void test_main(void)
{
	ztest_test_suite(zero_latency_irqs,
		ztest_unit_test(test_arm_zero_latency_irqs));
	ztest_run_test_suite(zero_latency_irqs);
}
