/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_sched.h"

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;
static int last_prio;

static void thread_entry(void *p1, void *p2, void *p3)
{
	last_prio = k_thread_priority_get(k_current_get());
}

/*test cases*/

/**
 * @brief Validate that the cooperative thread will
 * not be preempted
 *
 * @details Create a cooperative thread with priority higher
 * than the current cooperative thread. Make sure that the higher
 * priority thread will not preempt the lower priority cooperative
 * thread.
 *
 * @ingroup kernel_sched_tests
 */
void test_priority_cooperative(void)
{
	int old_prio = k_thread_priority_get(k_current_get());

	/* set current thread to a negative priority */
	last_prio = -1;
	k_thread_priority_set(k_current_get(), last_prio);

	/* spawn thread with higher priority */
	int spawn_prio = last_prio - 1;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      spawn_prio, 0, 0);
	/* checkpoint: current thread shouldn't preempted by higher thread */
	zassert_true(last_prio == k_thread_priority_get(k_current_get()), NULL);
	k_sleep(100);
	/* checkpoint: spawned thread get executed */
	zassert_true(last_prio == spawn_prio, NULL);
	k_thread_abort(tid);

	/* restore environment */
	k_thread_priority_set(k_current_get(), old_prio);
}

/**
 * @brief Validate preemptiveness of preemptive thread
 *
 * @details Create a preemptive thread which is of priority
 * lower than current thread. Current thread is made has preemptive.
 * Make sure newly created thread is not preempted. Now create a
 * preemptive thread which is of priority higher than current
 * thread. Make sure newly created thread is preempted
 *
 * @ingroup kernel_sched_tests
 */
void test_priority_preemptible(void)
{
	int old_prio = k_thread_priority_get(k_current_get());

	/* set current thread to a non-negative priority */
	last_prio = 2;
	k_thread_priority_set(k_current_get(), last_prio);

	int spawn_prio = last_prio - 1;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      spawn_prio, 0, 0);
	/* checkpoint: thread is preempted by higher thread */
	zassert_true(last_prio == spawn_prio, NULL);

	k_sleep(100);
	k_thread_abort(tid);

	spawn_prio = last_prio + 1;
	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      thread_entry, NULL, NULL, NULL,
			      spawn_prio, 0, 0);
	/* checkpoint: thread is not preempted by lower thread */
	zassert_false(last_prio == spawn_prio, NULL);
	k_thread_abort(tid);

	/* restore environment */
	k_thread_priority_set(k_current_get(), old_prio);
}
