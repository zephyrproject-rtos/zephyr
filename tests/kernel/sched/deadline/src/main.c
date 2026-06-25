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

#define MSEC_TO_CYCLES(msec)  (int)(((uint64_t)(msec) * \
				     (uint64_t)sys_clock_hw_cycles_per_sec()) / \
				    (uint64_t)MSEC_PER_SEC)

struct k_thread worker_threads[NUM_THREADS];
volatile struct k_thread *expected_thread;
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

/**
 * @brief Verify EDF threads run in earliest-deadline-first order.
 *
 * @details
 * At equal priority the scheduler must order runnable threads by their deadline
 * (set via k_thread_deadline_set()), running the earliest deadline first.
 * Several equal-priority threads are given random deadlines and must execute in
 * ascending-deadline order.
 *
 * Test steps:
 * - Create NUM_THREADS equal-priority threads, each recording its run order.
 * - Assign each a random deadline, then sleep to let them run.
 * - Verify their execution order matches ascending deadline order.
 *
 * Expected result:
 * - Threads execute earliest-deadline-first.
 *
 * @ingroup tests_kernel_sched
 *
 * @see k_thread_deadline_set()
 * @verifies ZEP-SRS-2-5
 * @verifies ZEP-SRS-2-17
 */
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

/**
 * @brief Verify k_yield() rotates among equal deadline/priority threads.
 *
 * @details
 * Threads at the same priority and deadline (0) must each get to run when they
 * cooperatively yield. Each worker increments a counter and yields; all must
 * have run by the time the test wakes.
 *
 * Test steps:
 * - Create NUM_THREADS equal-priority/deadline threads that bump a counter and
 *   k_yield().
 * - Sleep and verify all threads ran.
 *
 * Expected result:
 * - Every thread runs, confirming yield rotates among equal peers.
 *
 * @ingroup tests_kernel_sched
 *
 * @see k_yield()
 */
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
 * @brief Verify k_thread_deadline_set() does not wake a not-yet-started thread.
 *
 * @details
 * Setting a deadline on a thread that is still in its start delay (unqueued)
 * must not make it run before its delay elapses. Threads are created with a
 * start delay, given deadlines while unqueued, and must stay dormant until the
 * delay passes.
 *
 * Test steps:
 * - Create NUM_THREADS threads with a start delay (unqueued).
 * - Set a random deadline on each and sleep less than the delay; none must run.
 * - Sleep past the delay and confirm all threads then run.
 *
 * Expected result:
 * - Deadline-set does not start an unqueued thread early; threads run only after
 *   their start delay.
 *
 * @ingroup tests_kernel_sched
 *
 * @see k_thread_deadline_set()
 * @verifies ZEP-SRS-2-17
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

#if (CONFIG_MP_MAX_NUM_CPUS == 1)
static void reschedule_wrapper(const void *param)
{
	ARG_UNUSED(param);

	k_reschedule();
}

static void test_reschedule_helper0(void *p1, void *p2, void *p3)
{
	/* 4. Reschedule brings us here */

	zassert_true(expected_thread == _current, "");

	expected_thread = &worker_threads[1];
}

static void test_reschedule_helper1(void *p1, void *p2, void *p3)
{
	void (*offload)(void (*f)(const void *p), const void *param) = p1;

	/* 1. First helper expected to execute */

	zassert_true(expected_thread == _current, "");

	offload(reschedule_wrapper, NULL);

	/* 2. Deadlines have not changed. Expected no changes */

	zassert_true(expected_thread == _current, "");

	k_thread_deadline_set(_current, MSEC_TO_CYCLES(1000));

	/* 3. Deadline changed, but there was no reschedule */

	zassert_true(expected_thread == _current, "");

	expected_thread = &worker_threads[0];
	offload(reschedule_wrapper, NULL);

	/* 5. test_thread_reschedule_helper0 executed */

	zassert_true(expected_thread == _current, "");
}

static void thread_offload(void (*f)(const void *p), const void *param)
{
	f(param);
}

/**
 * @brief Verify a deadline change takes effect only on k_reschedule().
 *
 * @details
 * Changing a thread's deadline does not by itself trigger a context switch; an
 * explicit k_reschedule() must re-evaluate the ready queue and switch to the now
 * earliest-deadline thread. The test orchestrates two EDF helpers and checks the
 * expected thread is selected only after k_reschedule() (exercised via a direct
 * call and, on non-SMP, via irq_offload). Built for single-CPU configs only.
 *
 * Test steps:
 * - Create two EDF helper threads with different deadlines.
 * - Confirm changing a deadline alone causes no switch.
 * - Invoke k_reschedule() and confirm the expected thread is then selected.
 *
 * Expected result:
 * - The scheduler switches to the earliest-deadline thread only on reschedule.
 *
 * @ingroup tests_kernel_sched
 *
 * @see k_reschedule()
 * @see k_thread_deadline_set()
 */
ZTEST(suite_deadline, test_thread_reschedule)
{
	k_thread_create(&worker_threads[0], worker_stacks[0], STACK_SIZE,
			test_reschedule_helper0,
			thread_offload, NULL, NULL,
			K_LOWEST_APPLICATION_THREAD_PRIO,
			0, K_NO_WAIT);

	k_thread_create(&worker_threads[1], worker_stacks[1], STACK_SIZE,
			test_reschedule_helper1,
			thread_offload, NULL, NULL,
			K_LOWEST_APPLICATION_THREAD_PRIO,
			0, K_NO_WAIT);

	k_thread_deadline_set(&worker_threads[0], MSEC_TO_CYCLES(500));
	k_thread_deadline_set(&worker_threads[1], MSEC_TO_CYCLES(10));

	expected_thread = &worker_threads[1];

	k_thread_join(&worker_threads[1], K_FOREVER);
	k_thread_join(&worker_threads[0], K_FOREVER);

#ifndef CONFIG_SMP
	/*
	 * When SMP is enabled, there is always a reschedule performed
	 * at the end of the ISR.
	 */
	k_thread_create(&worker_threads[0], worker_stacks[0], STACK_SIZE,
			test_reschedule_helper0,
			irq_offload, NULL, NULL,
			K_LOWEST_APPLICATION_THREAD_PRIO,
			0, K_NO_WAIT);

	k_thread_create(&worker_threads[1], worker_stacks[1], STACK_SIZE,
			test_reschedule_helper1,
			irq_offload, NULL, NULL,
			K_LOWEST_APPLICATION_THREAD_PRIO,
			0, K_NO_WAIT);

	k_thread_deadline_set(&worker_threads[0], MSEC_TO_CYCLES(500));
	k_thread_deadline_set(&worker_threads[1], MSEC_TO_CYCLES(10));

	expected_thread = &worker_threads[1];

	k_thread_join(&worker_threads[1], K_FOREVER);
	k_thread_join(&worker_threads[0], K_FOREVER);

#endif /* !CONFIG_SMP */
}
#endif /* CONFIG_MP_MAX_NUM_CPUS == 1 */

ZTEST_SUITE(suite_deadline, NULL, NULL, NULL, NULL, NULL);
