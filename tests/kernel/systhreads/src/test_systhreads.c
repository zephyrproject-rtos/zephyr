/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
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

/* test cases */
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
