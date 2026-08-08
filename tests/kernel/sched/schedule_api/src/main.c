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
 * @defgroup tests_kernel_sched Scheduling tests
 *
 * @ingroup all_tests
 *
 * @{
 * @}
 */
ZTEST_SUITE(threads_scheduling, NULL, threads_scheduling_tests_setup, NULL, NULL, NULL);
ZTEST_SUITE(threads_scheduling_1cpu, NULL, threads_scheduling_tests_setup,
			ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
