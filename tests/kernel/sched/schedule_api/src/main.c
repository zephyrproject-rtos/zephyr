/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_sched.h"

/* Shared threads */
K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
K_THREAD_STACK_ARRAY_DEFINE(tstacks, MAX_NUM_THREAD, STACK_SIZE);

void spin_for_ms(int ms)
{
#if defined(CONFIG_X86_64) && defined(CONFIG_QEMU_TARGET)
	/* qemu-system-x86_64 has a known bug with the hpet device
	 * where it will drop interrupts if you try to spin on the
	 * counter.
	 */
	k_busy_wait(ms * 1000);
#else
	u32_t t32 = k_uptime_get_32();

	while (k_uptime_get_32() - t32 < ms) {
		/* In the posix arch, a busy loop takes no time, so
		 * let's make it take some
		 */
		if (IS_ENABLED(CONFIG_ARCH_POSIX)) {
			k_busy_wait(50);
		}
	}
#endif
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
