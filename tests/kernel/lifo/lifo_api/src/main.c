/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_lifo_thread2thread(void);
extern void test_lifo_thread2isr(void);
extern void test_lifo_isr2thread(void);
extern void test_lifo_get_fail(void);
extern void test_lifo_loop(void);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(lifo_api,
		ztest_unit_test(test_lifo_thread2thread),
		ztest_unit_test(test_lifo_thread2isr),
		ztest_unit_test(test_lifo_isr2thread),
		ztest_unit_test(test_lifo_get_fail),
		ztest_unit_test(test_lifo_loop));
	ztest_run_test_suite(lifo_api);
}
