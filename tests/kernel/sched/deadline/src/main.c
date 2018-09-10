/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <ztest.h>

#define NUM_THREADS 8
#define STACK_SIZE (256 + CONFIG_TEST_EXTRA_STACKSIZE)

struct k_thread worker_threads[NUM_THREADS];

K_THREAD_STACK_ARRAY_DEFINE(worker_stacks, NUM_THREADS, STACK_SIZE);

int thread_deadlines[NUM_THREADS];

/* The number of worker threads that ran, and and array of their
 * indices in execution order
 */
int n_exec;
int exec_order[NUM_THREADS];

void worker(void *p1, void *p2, void *p3)
{
	int tidx = (int) p1;

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
		k_sleep(1000000);
	}
}

void test_deadline(void)
{
	int i;

	/* Create a bunch of threads at a single lower priority.  Give
	 * them each a random deadline.  Sleep, and check that they
	 * were executed in the right order.
	 */
	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_create(&worker_threads[i],
				worker_stacks[i], STACK_SIZE,
				worker, (void *)i, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO,
				0, 0);

		/* Positive-definite number with the bottom 8 bits
		 * masked off to prevent aliasing where "very close"
		 * deadlines end up in the opposite order due to the
		 * changing "now" between calls to
		 * k_thread_deadline_set().
		 */
		thread_deadlines[i] = sys_rand32_get() & 0x7fffff00;
	}

	zassert_true(n_exec == 0, "threads ran too soon");

	/* Similarly do the deadline setting in one quick pass to
	 * minimize aliasing with "now"
	 */
	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_deadline_set(&worker_threads[i], thread_deadlines[i]);
	}

	zassert_true(n_exec == 0, "threads ran too soon");

	k_sleep(100);

	zassert_true(n_exec == NUM_THREADS, "not enough threads ran");

	for (i = 1; i < NUM_THREADS; i++) {
		int d0 = thread_deadlines[exec_order[i-1]];
		int d1 = thread_deadlines[exec_order[i]];

		zassert_true(d0 <= d1, "threads ran in wrong order");
	}
}

void test_main(void)
{
	ztest_test_suite(suite_deadline,
			 ztest_unit_test(test_deadline));
	ztest_run_test_suite(suite_deadline);
}
