/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup kernel_message_queue Message Queue
 * @ingroup all_tests
 * @{
 * @}
 */

#include <ztest.h>
extern void test_msgq_thread(void);
extern void test_msgq_thread_overflow(void);
extern void test_msgq_isr(void);
extern void test_msgq_put_fail(void);
extern void test_msgq_get_fail(void);
extern void test_msgq_purge_when_put(void);
extern void test_msgq_attrs_get(void);

extern struct k_msgq kmsgq;
extern struct k_msgq msgq;
extern struct k_sem end_sema;
extern struct k_thread tdata;
K_THREAD_STACK_EXTERN(tstack);

/*test case main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(), &kmsgq, &msgq, &end_sema,
			      &tdata, &tstack, NULL);

	ztest_test_suite(msgq_api,
			 ztest_user_unit_test(test_msgq_thread),
			 ztest_user_unit_test(test_msgq_thread_overflow),
			 ztest_unit_test(test_msgq_isr),
			 ztest_user_unit_test(test_msgq_put_fail),
			 ztest_user_unit_test(test_msgq_get_fail),
			 ztest_user_unit_test(test_msgq_attrs_get),
			 ztest_user_unit_test(test_msgq_purge_when_put));
	ztest_run_test_suite(msgq_api);
}
