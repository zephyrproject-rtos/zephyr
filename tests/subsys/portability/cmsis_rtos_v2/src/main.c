/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/kernel.h>
#include <cmsis_os2.h>

extern void test_kernel_apis(void);
extern void test_delay(void);
extern void test_thread_apis(void);
extern void test_thread_apis_dynamic(void);
extern void test_thread_prio(void);
extern void test_thread_prio_dynamic(void);
extern void test_timer(void);
extern void test_mutex(void);
extern void test_mutex_lock_timeout(void);
extern void test_semaphore(void);
extern void test_mempool(void);
extern void test_mempool_dynamic(void);
extern void test_messageq(void);
extern void test_event_flags_no_wait_timeout(void);
extern void test_event_flags_signalled(void);
extern void test_event_flags_isr(void);
extern void test_thread_flags_no_wait_timeout(void);
extern void test_thread_flags_signalled(void);
extern void test_thread_flags_isr(void);
extern void test_thread_join(void);
extern void test_thread_detached(void);
extern void test_thread_joinable_detach(void);
extern void test_thread_joinable_terminate(void);

void test_main(void)
{
	ztest_test_suite(test_cmsis_v2_apis,
			 ztest_unit_test(test_kernel_apis),
			 ztest_unit_test(test_delay),
			 ztest_unit_test(test_thread_apis),
			 ztest_unit_test(test_thread_apis_dynamic),
			 ztest_unit_test(test_thread_prio),
			 ztest_unit_test(test_thread_prio_dynamic),
			 ztest_unit_test(test_timer),
			 ztest_unit_test(test_mutex),
			 ztest_unit_test(test_mutex_lock_timeout),
			 ztest_unit_test(test_semaphore),
			 ztest_unit_test(test_mempool),
			 ztest_unit_test(test_mempool_dynamic),
			 ztest_unit_test(test_messageq),
			 ztest_unit_test(test_event_flags_no_wait_timeout),
			 ztest_unit_test(test_event_flags_signalled),
			 ztest_unit_test(test_event_flags_isr),
			 ztest_unit_test(test_thread_flags_no_wait_timeout),
			 ztest_unit_test(test_thread_flags_signalled),
			 ztest_unit_test(test_thread_flags_isr),
			 ztest_unit_test(test_thread_join),
			 ztest_unit_test(test_thread_detached),
			 ztest_unit_test(test_thread_joinable_detach),
			 ztest_unit_test(test_thread_joinable_terminate));

	ztest_run_test_suite(test_cmsis_v2_apis);
}
