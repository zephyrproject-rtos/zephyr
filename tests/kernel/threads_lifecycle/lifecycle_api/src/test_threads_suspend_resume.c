/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_threads_lifecycle
 * @{
 * @defgroup t_threads_suspend_resume test_threads_suspend_resume
 * @}
 */
#include <ztest.h>

#define STACK_SIZE (256 + CONFIG_TEST_EXTRA_STACKSIZE)
static char __noinit __stack tstack[STACK_SIZE];
static int last_prio;

static void thread_entry(void *p1, void *p2, void *p3)
{
	last_prio = k_thread_priority_get(k_current_get());
}

static void threads_suspend_resume(int prio)
{
	int old_prio = k_thread_priority_get(k_current_get());

	/* set current thread */
	last_prio = prio;
	k_thread_priority_set(k_current_get(), last_prio);

	/* spawn thread with lower priority */
	int spawn_prio = last_prio + 1;

	k_tid_t tid = k_thread_spawn(tstack, STACK_SIZE,
				     thread_entry, NULL, NULL, NULL,
				     spawn_prio, 0, 0);
	/* checkpoint: suspend current thread */
	k_thread_suspend(tid);
	k_sleep(100);
	/* checkpoint: spawned thread shouldn't be executed after suspend */
	assert_false(last_prio == spawn_prio, NULL);
	k_thread_resume(tid);
	k_sleep(100);
	/* checkpoint: spawned thread should be executed after resume */
	assert_true(last_prio == spawn_prio, NULL);

	k_thread_abort(tid);

	/* restore environment */
	k_thread_priority_set(k_current_get(), old_prio);
}

/*test cases*/
void test_threads_suspend_resume_cooperative(void)
{
	threads_suspend_resume(-2);
}

void test_threads_suspend_resume_preemptible(void)
{
	threads_suspend_resume(1);
}
