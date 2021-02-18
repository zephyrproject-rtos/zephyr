/*
 * Copyright (c) Baumer Electric AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_armv6_zl_irqs_locking(void);
extern void test_armv6_zl_irqs_lock_nesting(void);
extern void test_armv6_zl_irqs_multiple(void);
extern void test_armv6_zl_irqs_enable(void);
extern void test_armv6_zl_irqs_disable(void);
extern void test_armv6_zl_irqs_thread_specificity(void);

void test_main(void)
{
	ztest_test_suite(arm_zl_irq_advanced_features,
		ztest_unit_test(test_armv6_zl_irqs_locking),
		ztest_unit_test(test_armv6_zl_irqs_lock_nesting),
		ztest_unit_test(test_armv6_zl_irqs_multiple),
		ztest_unit_test(test_armv6_zl_irqs_enable),
		ztest_unit_test(test_armv6_zl_irqs_disable),
		ztest_unit_test(test_armv6_zl_irqs_thread_specificity)
	);

	ztest_run_test_suite(arm_zl_irq_advanced_features);
}
