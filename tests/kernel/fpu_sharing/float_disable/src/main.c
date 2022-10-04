/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_k_float_disable_common(void);
extern void test_k_float_disable_syscall(void);
extern void test_k_float_disable_irq(void);


void test_main(void)
{
	ztest_test_suite(suite_k_float_disable,
		ztest_unit_test(test_k_float_disable_common),
		ztest_unit_test(test_k_float_disable_irq),
		ztest_unit_test(test_k_float_disable_syscall));
	ztest_run_test_suite(suite_k_float_disable);
}
