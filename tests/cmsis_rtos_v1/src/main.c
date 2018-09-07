/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <cmsis_os.h>

extern void test_kernel_start(void);
extern void test_kernel_systick(void);
extern void test_thread_prio(void);
extern void test_thread_apis(void);
extern void test_thread_instances(void);
extern void test_timer(void);
extern void test_mutex(void);
extern void test_mutex_lock_timeout(void);
extern void test_semaphore(void);
extern void test_mempool(void);
extern void test_mailq(void);
extern void test_messageq(void);
extern void test_signal_events_no_wait(void);
extern void test_signal_events_timeout(void);
extern void test_signal_events_signalled(void);
extern void test_signal_events_isr(void);

void test_main(void)
{
	ztest_test_suite(test_cmsis_apis,
			ztest_unit_test(test_kernel_start),
			ztest_unit_test(test_kernel_systick),
			ztest_unit_test(test_thread_apis),
			ztest_unit_test(test_thread_prio),
			ztest_unit_test(test_thread_instances),
			ztest_unit_test(test_timer),
			ztest_unit_test(test_mutex),
			ztest_unit_test(test_mutex_lock_timeout),
			ztest_unit_test(test_semaphore),
			ztest_unit_test(test_mempool),
			ztest_unit_test(test_mailq),
			ztest_unit_test(test_messageq),
			ztest_unit_test(test_signal_events_no_wait),
			ztest_unit_test(test_signal_events_timeout),
			ztest_unit_test(test_signal_events_signalled),
			ztest_unit_test(test_signal_events_isr));
	ztest_run_test_suite(test_cmsis_apis);
}
