/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_sched.h"

/* Shared threads */
K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
K_THREAD_STACK_ARRAY_DEFINE(tstacks, MAX_NUM_THREAD, STACK_SIZE);

/* Not in header file intentionally, see #16760 */
K_THREAD_STACK_EXTERN(ustack);

void spin_for_ms(int ms)
{
	uint32_t t32 = k_uptime_get_32();

	while (k_uptime_get_32() - t32 < ms) {
		/* In the posix arch, a busy loop takes no time, so
		 * let's make it take some
		 */
		if (IS_ENABLED(CONFIG_ARCH_POSIX)) {
			k_busy_wait(50);
		}
	}
}

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
#ifdef CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &user_thread, &user_sem,
			      &ustack);
#endif /* CONFIG_USERSPACE */

	ztest_test_suite(threads_scheduling,
			 ztest_unit_test(test_bad_priorities),
			 ztest_unit_test(test_priority_cooperative),
			 ztest_unit_test(test_priority_preemptible),
			 ztest_1cpu_unit_test(test_priority_preemptible_wait_prio),
			 ztest_unit_test(test_yield_cooperative),
			 ztest_unit_test(test_sleep_cooperative),
			 ztest_unit_test(test_sleep_wakeup_preemptible),
			 ztest_unit_test(test_pending_thread_wakeup),
			 ztest_unit_test(test_time_slicing_preemptible),
			 ztest_unit_test(test_time_slicing_disable_preemptible),
			 ztest_unit_test(test_lock_preemptible),
			 ztest_unit_test(test_unlock_preemptible),
			 ztest_unit_test(test_unlock_nested_sched_lock),
			 ztest_unit_test(test_sched_is_preempt_thread),
			 ztest_unit_test(test_slice_reset),
			 ztest_unit_test(test_slice_scheduling),
			 ztest_unit_test(test_priority_scheduling),
			 ztest_unit_test(test_wakeup_expired_timer_thread),
			 ztest_user_unit_test(test_user_k_wakeup),
			 ztest_user_unit_test(test_user_k_is_preempt)
			 );
	ztest_run_test_suite(threads_scheduling);
}
