/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_isr_dynamic(void);
extern void test_nested_isr(void);
extern void test_prevent_interruption(void);
extern void test_isr_regular(void);
extern void test_isr_offload_job_multiple(void);
extern void test_isr_offload_job_identi(void);
extern void test_isr_offload_job(void);
extern void test_direct_interrupt(void);

void test_main(void)
{
	ztest_test_suite(interrupt_feature,
			ztest_unit_test(test_isr_dynamic),
			ztest_unit_test(test_nested_isr),
			ztest_unit_test(test_prevent_interruption),
			ztest_unit_test(test_isr_regular),
			ztest_unit_test(test_isr_offload_job_multiple),
			ztest_unit_test(test_isr_offload_job_identi),
			ztest_unit_test(test_isr_offload_job),
			ztest_unit_test(test_direct_interrupt)
			);
	ztest_run_test_suite(interrupt_feature);
}
