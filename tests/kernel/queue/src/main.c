/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_queue_thread2thread(void);
extern void test_queue_thread2isr(void);
extern void test_queue_isr2thread(void);
extern void test_queue_get_2threads(void);
extern void test_queue_get_fail(void);
extern void test_queue_loop(void);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(queue_api,
			 ztest_unit_test(test_queue_thread2thread),
			 ztest_unit_test(test_queue_thread2isr),
			 ztest_unit_test(test_queue_isr2thread),
			 ztest_unit_test(test_queue_get_2threads),
			 ztest_unit_test(test_queue_get_fail),
			 ztest_unit_test(test_queue_loop));
	ztest_run_test_suite(queue_api);
}
