/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

/*declare test cases */
extern void test_kdefine_preempt_thread(void);
extern void test_kdefine_coop_thread(void);
extern void test_kinit_preempt_thread(void);
extern void test_kinit_coop_thread(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_thread_init,
			 ztest_unit_test(test_kdefine_preempt_thread),
			 ztest_unit_test(test_kdefine_coop_thread),
			 ztest_unit_test(test_kinit_preempt_thread),
			 ztest_unit_test(test_kinit_coop_thread));
	ztest_run_test_suite(test_thread_init);
}
