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
K_THREAD_STACK_DECLARE(ustack, STACK_SIZE);

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

static void *threads_scheduling_tests_setup(void)
{
#ifdef CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &user_thread, &user_sem,
			      &ustack);
#endif /* CONFIG_USERSPACE */

	return NULL;
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
ZTEST_SUITE(threads_scheduling, NULL, threads_scheduling_tests_setup, NULL, NULL, NULL);
ZTEST_SUITE(threads_scheduling_1cpu, NULL, threads_scheduling_tests_setup,
			ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
