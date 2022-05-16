/*
 * Copyright (c) 2017 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_poll_no_wait(void);
extern void test_poll_wait(void);
extern void test_poll_zero_events(void);
extern void test_poll_cancel_main_low_prio(void);
extern void test_poll_cancel_main_high_prio(void);
extern void test_poll_multi(void);
extern void test_poll_threadstate(void);
extern void test_poll_grant_access(void);
extern void test_poll_fail_grant_access(void);
extern void test_detect_is_polling(void);
#ifdef CONFIG_USERSPACE
extern void test_k_poll_user_num_err(void);
extern void test_k_poll_user_mem_err(void);
extern void test_k_poll_user_type_sem_err(void);
extern void test_k_poll_user_type_signal_err(void);
extern void test_k_poll_user_type_fifo_err(void);
extern void test_k_poll_user_type_msgq_err(void);
extern void test_poll_signal_init_null(void);
extern void test_poll_signal_check_obj(void);
extern void test_poll_signal_check_signal(void);
extern void test_poll_signal_check_result(void);
extern void test_poll_signal_raise_null(void);
extern void test_poll_signal_reset_null(void);
#else
#define dummy_test(_name)	   \
	static void _name(void)	   \
	{			   \
		ztest_test_skip(); \
	}

dummy_test(test_k_poll_user_num_err);
dummy_test(test_k_poll_user_mem_err);
dummy_test(test_k_poll_user_type_sem_err);
dummy_test(test_k_poll_user_type_signal_err);
dummy_test(test_k_poll_user_type_fifo_err);
dummy_test(test_k_poll_user_type_msgq_err);
dummy_test(test_poll_signal_init_null);
dummy_test(test_poll_signal_check_obj);
dummy_test(test_poll_signal_check_signal);
dummy_test(test_poll_signal_check_result);
dummy_test(test_poll_signal_raise_null);
dummy_test(test_poll_signal_reset_null);
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_64BIT
#define MAX_SZ	256
#else
#define MAX_SZ	128
#endif

K_HEAP_DEFINE(test_heap, MAX_SZ * 4);

/*test case main entry*/
void test_main(void)
{
	test_poll_grant_access();
	test_poll_fail_grant_access();

	k_thread_heap_assign(k_current_get(), &test_heap);

	ztest_test_suite(poll_api,
			 ztest_1cpu_user_unit_test(test_poll_no_wait),
			 ztest_1cpu_unit_test(test_poll_wait),
			 ztest_1cpu_unit_test(test_poll_zero_events),
			 ztest_1cpu_unit_test(test_poll_cancel_main_low_prio),
			 ztest_1cpu_unit_test(test_poll_cancel_main_high_prio),
			 ztest_unit_test(test_poll_multi),
			 ztest_1cpu_unit_test(test_poll_threadstate),
			 ztest_1cpu_unit_test(test_detect_is_polling),
			 ztest_user_unit_test(test_k_poll_user_num_err),
			 ztest_user_unit_test(test_k_poll_user_mem_err),
			 ztest_user_unit_test(test_k_poll_user_type_sem_err),
			 ztest_user_unit_test(test_k_poll_user_type_signal_err),
			 ztest_user_unit_test(test_k_poll_user_type_fifo_err),
			 ztest_user_unit_test(test_k_poll_user_type_msgq_err),
			 ztest_user_unit_test(test_poll_signal_init_null),
			 ztest_user_unit_test(test_poll_signal_check_obj),
			 ztest_user_unit_test(test_poll_signal_check_signal),
			 ztest_user_unit_test(test_poll_signal_check_result),
			 ztest_user_unit_test(test_poll_signal_raise_null),
			 ztest_user_unit_test(test_poll_signal_reset_null)
			 );
	ztest_run_test_suite(poll_api);
}
