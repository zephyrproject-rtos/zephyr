/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_stack
 * @{
 * @defgroup t_stack_api test_stack_api
 * @}
 */

#include <ztest.h>
extern void test_stack_thread2thread(void);
extern void test_stack_thread2isr(void);
extern void test_stack_pop_fail(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_stack_api,
			 ztest_unit_test(test_stack_thread2thread),
			 ztest_unit_test(test_stack_thread2isr),
			 ztest_unit_test(test_stack_pop_fail));
	ztest_run_test_suite(test_stack_api);
}
