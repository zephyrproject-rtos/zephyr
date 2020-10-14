/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_sched.h"
#include <ksched.h>

#define THREAD_NUM 4

static struct k_thread tdata_prio[THREAD_NUM];
static struct k_thread tdata;
static int last_prio;
static uint8_t tid_num[4];
static struct k_sem sync_sema;


static void thread_entry(void *p1, void *p2, void *p3)
{
	last_prio = k_thread_priority_get(k_current_get());
}

static void thread_entry_prio(void *p1, void *p2, void *p3)
{
	static int i;

	k_sem_take(&sync_sema, K_MSEC(100));

	tid_num[i++] = POINTER_TO_INT(p1);
}


/* test cases */

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
				      spawn_prio, 0, K_NO_WAIT);
	/* checkpoint: current thread shouldn't preempted by higher thread */
	zassert_true(last_prio == k_thread_priority_get(k_current_get()), NULL);
	k_sleep(K_MSEC(100));
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
				      spawn_prio, 0, K_NO_WAIT);
	/* checkpoint: thread is preempted by higher thread */
	zassert_true(last_prio == spawn_prio, NULL);

	k_sleep(K_MSEC(100));
	k_thread_abort(tid);

	spawn_prio = last_prio + 1;
	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      thread_entry, NULL, NULL, NULL,
			      spawn_prio, 0, K_NO_WAIT);
	/* checkpoint: thread is not preempted by lower thread */
	zassert_false(last_prio == spawn_prio, NULL);
	k_thread_abort(tid);

	/* restore environment */
	k_thread_priority_set(k_current_get(), old_prio);
}

/**
 * @brief Validate scheduling sequence of preemptive threads with start delay
 *
 * @details Create four preemptive threads which are of priority
 * higher than current thread. Make sure that the highest priority
 * and longest waiting thread is scheduled first.
 *
 * @ingroup kernel_sched_tests
 */
void test_priority_preemptible_wait_prio(void)
{
	int old_prio = k_thread_priority_get(k_current_get());
	k_tid_t tid[THREAD_NUM];
	uint8_t tid_chk[4] =  { 0, 1, 2, 3 };

	k_sem_init(&sync_sema, 0, THREAD_NUM);

	/* Ensure that this code starts running at the start of a system tick */
	k_usleep(1);

	/* set current thread to a non-negative priority */
	last_prio = K_PRIO_PREEMPT(2);
	k_thread_priority_set(k_current_get(), last_prio);

	/* the highest-priority thread that has waited the longest */
	tid[0] = k_thread_create(&tdata_prio[0], tstacks[0], STACK_SIZE,
			thread_entry_prio, INT_TO_POINTER(0), NULL, NULL,
			K_PRIO_PREEMPT(0), 0, K_MSEC(10));
	/* the highest-priority thread that has waited the shorter */
	tid[1] = k_thread_create(&tdata_prio[1], tstacks[1], STACK_SIZE,
			thread_entry_prio, INT_TO_POINTER(1), NULL, NULL,
			K_PRIO_PREEMPT(0), 0, K_MSEC(20));
	/* the lowest-priority thread that has waited longest */
	tid[2] = k_thread_create(&tdata_prio[2], tstacks[2], STACK_SIZE,
			thread_entry_prio, INT_TO_POINTER(2), NULL, NULL,
			K_PRIO_PREEMPT(1), 0, K_MSEC(10));
	/* the lowest-priority thread that has waited shorter */
	tid[3] = k_thread_create(&tdata_prio[3], tstacks[3], STACK_SIZE,
			thread_entry_prio, INT_TO_POINTER(3), NULL, NULL,
			K_PRIO_PREEMPT(1), 0, K_MSEC(20));

	/* relinquish CPU for above threads to start */
	k_sleep(K_MSEC(30));

	for (int i = 0; i < THREAD_NUM; i++) {
		k_sem_give(&sync_sema);
	}

	zassert_true((memcmp(tid_num, tid_chk, 4) == 0),
		     "scheduling priority failed");

	/* test case tear down */
	for (int i = 0; i < THREAD_NUM; i++) {
		k_thread_abort(tid[i]);
	}

	/* restore environment */
	k_thread_priority_set(k_current_get(), old_prio);
}

extern void idle(void *p1, void *p2, void *p3);

/**
 * Validate checking priority values
 *
 * Our test cases don't cover every outcome of whether a priority is valid,
 * do so here.
 *
 * @ingroup kernel_sched_tests
 */
void test_bad_priorities(void)
{
	struct prio_test {
		int prio;
		void *entry;
		bool result;
	} testcases[] = {
		{ K_IDLE_PRIO, idle, true },
		{ K_IDLE_PRIO, NULL, false },
		{ K_HIGHEST_APPLICATION_THREAD_PRIO - 1, NULL, false },
		{ K_LOWEST_APPLICATION_THREAD_PRIO + 1, NULL, false },
		{ K_HIGHEST_APPLICATION_THREAD_PRIO, NULL, true },
		{ K_LOWEST_APPLICATION_THREAD_PRIO, NULL, true },
		{ CONFIG_MAIN_THREAD_PRIORITY, NULL, true }
	};

	for (int i = 0; i < ARRAY_SIZE(testcases); i++) {
		zassert_equal(_is_valid_prio(testcases[i].prio,
					     testcases[i].entry),
			      testcases[i].result, "failed check %d", i);
		/* XXX why are these even separate APIs? */
		zassert_equal(Z_VALID_PRIO(testcases[i].prio,
					   testcases[i].entry),
			      testcases[i].result, "failed check %d", i);
	}
}
