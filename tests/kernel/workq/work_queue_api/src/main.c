/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_workq_start_before_submit(void);
extern void test_work_submit_to_queue_thread(void);
extern void test_work_submit_to_queue_isr(void);
extern void test_work_submit_thread(void);
extern void test_work_submit_isr(void);
extern void test_delayed_work_submit_to_queue_thread(void);
extern void test_delayed_work_submit_to_queue_isr(void);
extern void test_delayed_work_submit_thread(void);
extern void test_delayed_work_submit_isr(void);
extern void test_delayed_work_cancel_from_queue_thread(void);
extern void test_delayed_work_cancel_from_queue_isr(void);
extern void test_delayed_work_cancel_thread(void);
extern void test_delayed_work_cancel_isr(void);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_workq_api,
			 ztest_unit_test(test_workq_start_before_submit),/*keep first!*/
			 ztest_unit_test(test_work_submit_to_queue_thread),
			 ztest_unit_test(test_work_submit_to_queue_isr),
			 ztest_unit_test(test_work_submit_thread),
			 ztest_unit_test(test_work_submit_isr),
			 ztest_unit_test(test_delayed_work_submit_to_queue_thread),
			 ztest_unit_test(test_delayed_work_submit_to_queue_isr),
			 ztest_unit_test(test_delayed_work_submit_thread),
			 ztest_unit_test(test_delayed_work_submit_isr),
			 ztest_unit_test(test_delayed_work_cancel_from_queue_thread),
			 ztest_unit_test(test_delayed_work_cancel_from_queue_isr),
			 ztest_unit_test(test_delayed_work_cancel_thread),
			 ztest_unit_test(test_delayed_work_cancel_isr));
	ztest_run_test_suite(test_workq_api);
}
