/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/random/random.h>

#define NUM_THREADS 8
/* this should be large enough for us
 * to print a failing assert if necessary
 */
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

struct k_thread worker_threads[NUM_THREADS];
k_tid_t worker_tids[NUM_THREADS];

K_THREAD_STACK_ARRAY_DEFINE(worker_stacks, NUM_THREADS, STACK_SIZE);

int thread_deadlines[NUM_THREADS];

/* The number of worker threads that ran, and array of their
 * indices in execution order
 */
int n_exec;
int exec_order[NUM_THREADS];

void worker(void *p1, void *p2, void *p3)
{
	int tidx = POINTER_TO_INT(p1);

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	zassert_true(tidx >= 0 && tidx < NUM_THREADS, "");
	zassert_true(n_exec >= 0 && n_exec < NUM_THREADS, "");

	exec_order[n_exec++] = tidx;

	/* Sleep, don't exit.  It's not implausible that some
	 * platforms implement a thread-based cleanup step for threads
	 * that exit (pthreads does this already) which might muck
	 * with the scheduling.
	 */
	while (1) {
		k_sleep(K_MSEC(1000000));
	}
}

ZTEST(suite_deadline, test_deadline)
{
	int i;

	n_exec = 0;

	/* Create a bunch of threads at a single lower priority.  Give
	 * them each a random deadline.  Sleep, and check that they
	 * were executed in the right order.
	 */
	for (i = 0; i < NUM_THREADS; i++) {
		worker_tids[i] = k_thread_create(&worker_threads[i],
				worker_stacks[i], STACK_SIZE,
				worker, INT_TO_POINTER(i), NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO,
				0, K_NO_WAIT);

		/* Positive-definite number with the bottom 8 bits
		 * masked off to prevent aliasing where "very close"
		 * deadlines end up in the opposite order due to the
		 * changing "now" between calls to
		 * k_thread_deadline_set().
		 *
		 * Use only 30 bits of significant value.  The API
		 * permits 31 (strictly: the deadline time of the
		 * "first" runnable thread in any given priority and
		 * the "last" must be less than 2^31), but because the
		 * time between our generation here and the set of the
		 * deadline below takes non-zero time, it's possible
		 * to see rollovers.  Easier than using a modulus test
		 * or whatnot to restrict the values.
		 */
		thread_deadlines[i] = sys_rand32_get() & 0x3fffff00;
	}

	zassert_true(n_exec == 0, "threads ran too soon");

	/* Similarly do the deadline setting in one quick pass to
	 * minimize aliasing with "now"
	 */
	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_deadline_set(&worker_threads[i], thread_deadlines[i]);
	}

	zassert_true(n_exec == 0, "threads ran too soon");

	k_sleep(K_MSEC(100));

	zassert_true(n_exec == NUM_THREADS, "not enough threads ran");

	for (i = 1; i < NUM_THREADS; i++) {
		int d0 = thread_deadlines[exec_order[i-1]];
		int d1 = thread_deadlines[exec_order[i]];

		zassert_true(d0 <= d1, "threads ran in wrong order");
	}
	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_abort(worker_tids[i]);
	}
}

void yield_worker(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	zassert_true(n_exec >= 0 && n_exec < NUM_THREADS, "");

	n_exec += 1;

	k_yield();

	/* should not get here until all threads have started */
	zassert_true(n_exec == NUM_THREADS, "");

	k_thread_abort(k_current_get());

	CODE_UNREACHABLE;
}

ZTEST(suite_deadline, test_yield)
{
	/* Test that yield works across threads with the
	 * same deadline and priority. This currently works by
	 * simply not setting a deadline, which results in a
	 * deadline of 0.
	 */

	int i;

	n_exec = 0;

	/* Create a bunch of threads at a single lower priority
	 * and deadline.
	 * Each thread increments its own variable, then yields
	 * to the next. Sleep. Check that all threads ran.
	 */
	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_create(&worker_threads[i],
				worker_stacks[i], STACK_SIZE,
				yield_worker, NULL, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO,
				0, K_NO_WAIT);
	}

	zassert_true(n_exec == 0, "threads ran too soon");

	k_sleep(K_MSEC(100));

	zassert_true(n_exec == NUM_THREADS, "not enough threads ran");
}

void unqueue_worker(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	zassert_true(n_exec >= 0 && n_exec < NUM_THREADS, "");

	n_exec += 1;
}

/**
 * @brief Validate the behavior of deadline_set when the thread is not queued
 *
 * @details Create a bunch of threads with scheduling delay which make the
 * thread in unqueued state. The k_thread_deadline_set() call should not make
 * these threads run before there delay time pass.
 *
 * @ingroup kernel_sched_tests
 */
ZTEST(suite_deadline, test_unqueued)
{
	int i;

	n_exec = 0;

	for (i = 0; i < NUM_THREADS; i++) {
		worker_tids[i] = k_thread_create(&worker_threads[i],
				worker_stacks[i], STACK_SIZE,
				unqueue_worker, NULL, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO,
				0, K_MSEC(100));
	}

	zassert_true(n_exec == 0, "threads ran too soon");

	for (i = 0; i < NUM_THREADS; i++) {
		thread_deadlines[i] = sys_rand32_get() & 0x3fffff00;
		k_thread_deadline_set(&worker_threads[i], thread_deadlines[i]);
	}

	k_sleep(K_MSEC(50));

	zassert_true(n_exec == 0, "deadline set make the unqueued thread run");

	k_sleep(K_MSEC(100));

	zassert_true(n_exec == NUM_THREADS, "not enough threads ran");

	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_abort(worker_tids[i]);
	}
}

ZTEST_SUITE(suite_deadline, NULL, NULL, NULL, NULL, NULL);
