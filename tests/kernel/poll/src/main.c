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

	k_thread_heap_assign(k_current_get(), &test_heap);

	ztest_test_suite(poll_api,
			 ztest_1cpu_user_unit_test(test_poll_no_wait),
			 ztest_1cpu_unit_test(test_poll_wait),
			 ztest_1cpu_unit_test(test_poll_zero_events),
			 ztest_1cpu_unit_test(test_poll_cancel_main_low_prio),
			 ztest_1cpu_unit_test(test_poll_cancel_main_high_prio),
			 ztest_unit_test(test_poll_multi),
			 ztest_1cpu_unit_test(test_poll_threadstate));
	ztest_run_test_suite(poll_api);
}
