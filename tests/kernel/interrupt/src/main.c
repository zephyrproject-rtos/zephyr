/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_nested_isr(void);
extern void test_prevent_interruption(void);

void test_main(void)
{
	ztest_test_suite(interrupt_feature,
			ztest_unit_test(test_nested_isr),
			ztest_unit_test(test_prevent_interruption)
			);
	ztest_run_test_suite(interrupt_feature);
}
