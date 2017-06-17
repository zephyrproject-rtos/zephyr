/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_kernel_threads
 * @{
 * @defgroup t_threads_scheduling test_threads_scheduling
 * @brief TestPurpose: verify threads scheduling based on priority, time
 * slice and lock/unlock
 * @}
 */

#include "test_sched.h"

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_threads_scheduling,
		ztest_unit_test(test_priority_cooperative),
		ztest_unit_test(test_priority_preemptible),
		ztest_unit_test(test_yield_cooperative),
		ztest_unit_test(test_sleep_cooperative),
		ztest_unit_test(test_sleep_wakeup_preemptible),
		ztest_unit_test(test_time_slicing_preemptible),
		ztest_unit_test(test_time_slicing_disable_preemptible),
		ztest_unit_test(test_lock_preemptible),
		ztest_unit_test(test_unlock_preemptible),
		ztest_unit_test(test_sched_is_preempt_thread),
		ztest_unit_test(test_slice_reset)
		);
	ztest_run_test_suite(test_threads_scheduling);
}
