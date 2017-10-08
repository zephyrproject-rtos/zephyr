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

extern struct k_stack kstack;
extern struct k_stack stack;
extern struct k_thread thread_data;
extern struct k_sem end_sema;
K_THREAD_STACK_EXTERN(threadstack);

/*test case main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(), &kstack, &stack, &thread_data,
			      &end_sema, &threadstack, NULL);

	ztest_test_suite(test_stack_api,
			 ztest_user_unit_test(test_stack_thread2thread),
			 ztest_unit_test(test_stack_thread2isr),
			 ztest_user_unit_test(test_stack_pop_fail));
	ztest_run_test_suite(test_stack_api);
}
