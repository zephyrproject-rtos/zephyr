/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_CPU_CORTEX_M)
  #error project can only run on Cortex-M
#endif

#include <ztest.h>

extern void test_arm_irq_vector_table(void);

void test_main(void)
{
	ztest_test_suite(vector_table,
		ztest_unit_test(test_arm_irq_vector_table));
	ztest_run_test_suite(vector_table);
}
