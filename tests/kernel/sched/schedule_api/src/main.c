/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_sched.h"

/**
 * @brief Test scheduling
 *
 * @defgroup kernel_sched_tests Scheduling Tests
 *
 * @ingroup all_tests
 *
 * @{
 * @}
 */
/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(threads_scheduling,
			 ztest_unit_test(test_priority_cooperative),
			 ztest_unit_test(test_priority_preemptible),
			 ztest_unit_test(test_yield_cooperative),
			 ztest_unit_test(test_sleep_cooperative),
			 ztest_unit_test(test_sleep_wakeup_preemptible),
			 ztest_unit_test(test_pending_thread_wakeup),
			 ztest_unit_test(test_time_slicing_preemptible),
			 ztest_unit_test(test_time_slicing_disable_preemptible),
			 ztest_unit_test(test_lock_preemptible),
			 ztest_unit_test(test_unlock_preemptible),
			 ztest_unit_test(test_sched_is_preempt_thread),
			 ztest_unit_test(test_slice_reset),
			 ztest_unit_test(test_slice_scheduling),
			 ztest_unit_test(test_priority_scheduling),
			 ztest_unit_test(test_wakeup_expired_timer_thread)
			 );
	ztest_run_test_suite(threads_scheduling);
}
