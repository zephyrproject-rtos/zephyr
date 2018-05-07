/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_kernel_threads
 * @{
 * @defgroup t_systhreads test_systhreads
 * @brief TestPurpose: verify 2 system threads - main thread and idle thread
 * @}
 */

#include <ztest.h>
static int main_prio;

/* power hook functions if support */
int _sys_soc_suspend(s32_t ticks)
{
	/** TESTPOINT: check idle thread priority */
	zassert_true(k_thread_priority_get(k_current_get()) ==
			K_IDLE_PRIO, NULL);
	return 0;
}

/**
 * @brief System Thread Test Cases
 * @defgroup kernel_systemthreads System Threads
 * @ingroup all_tests
 * @brief Verify 2 system threads - main thread and idle thread
 * @{
 */
void test_systhreads_setup(void)
{
	main_prio = k_thread_priority_get(k_current_get());
}

void test_systhreads_main(void)
{
	zassert_true(main_prio == CONFIG_MAIN_THREAD_PRIORITY, NULL);
}

void test_systhreads_idle(void)
{
	k_sleep(100);
	/** TESTPOINT: check working thread priority should */
	zassert_true(k_thread_priority_get(k_current_get()) <
			K_IDLE_PRIO, NULL);
}
/**
 * @}
 */
void test_main(void)
{
	test_systhreads_setup();
	ztest_test_suite(systhreads,
		ztest_unit_test(test_systhreads_main),
		ztest_unit_test(test_systhreads_idle));
	ztest_run_test_suite(systhreads);
}
