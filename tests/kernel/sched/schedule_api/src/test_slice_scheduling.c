/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "test_sched.h"

#ifdef CONFIG_TIMESLICING

/* nrf 51 has lower ram, so creating less number of threads */
#if CONFIG_SRAM_SIZE <= 24
	#define NUM_THREAD 2
#elif (CONFIG_SRAM_SIZE <= 32) \
	|| defined(CONFIG_SOC_EMSK_EM7D)
	#define NUM_THREAD 3
#else
	#define NUM_THREAD 10
#endif
#define BASE_PRIORITY 0
#define ITRERATION_COUNT 5

BUILD_ASSERT(NUM_THREAD <= MAX_NUM_THREAD);
/* slice size in millisecond */
#define SLICE_SIZE 200
#define PERTHREAD_SLICE_TICKS 64
#define TICK_SLOP 4
/* busy for more than one slice */
#define BUSY_MS (SLICE_SIZE + 20)
static struct k_thread t[NUM_THREAD];

static K_SEM_DEFINE(sema1, 0, NUM_THREAD);
/* elapsed_slice taken by last thread */
static int64_t elapsed_slice;

static int thread_idx;

static void thread_tslice(void *p1, void *p2, void *p3)
{
	int idx = POINTER_TO_INT(p1);

	/* Print New line for last thread */
	int thread_parameter = (idx == (NUM_THREAD - 1)) ? '\n' :
			       (idx + 'A');

	int64_t expected_slice_min = k_ticks_to_ms_floor64(k_ms_to_ticks_ceil32(SLICE_SIZE) - 1);
	int64_t expected_slice_max = k_ticks_to_ms_ceil64(k_ms_to_ticks_ceil32(SLICE_SIZE) + 1);

	/* Clumsy, but need to handle the precision loss with
	 * submillisecond ticks.  It's always possible to alias and
	 * produce a tdelta of "1", no matter how fast ticks are.
	 */
	if (expected_slice_max == expected_slice_min) {
		expected_slice_max = expected_slice_min + 1;
	}

	while (1) {
		int64_t tdelta = k_uptime_delta(&elapsed_slice);
		TC_PRINT("%c", thread_parameter);
		/* Test Fails if thread exceed allocated time slice or
		 * Any thread is scheduled out of order.
		 */
		zassert_true(((tdelta >= expected_slice_min) &&
			      (tdelta <= expected_slice_max) &&
			      (idx == thread_idx)), NULL);
		thread_idx = (thread_idx + 1) % (NUM_THREAD);

		/* Keep the current thread busy for more than one slice,
		 * even though, when timeslice used up the next thread
		 * should be scheduled in.
		 */
		spin_for_ms(BUSY_MS);
		k_sem_give(&sema1);
	}
}

/* test cases */

/**
 * @brief Check the behavior of preemptive threads when the
 * time slice is disabled and enabled
 *
 * @details Create multiple preemptive threads with same priorities
 * and few with same priorities and enable the time slice.
 * Ensure that each thread is given the time slice period to execute.
 *
 * @ingroup kernel_sched_tests
 */
ZTEST(threads_scheduling, test_slice_scheduling)
{
	k_tid_t tid[NUM_THREAD];
	int old_prio = k_thread_priority_get(k_current_get());
	int count = 0;

	thread_idx = 0;

	/* disable timeslice */
	k_sched_time_slice_set(0, K_PRIO_PREEMPT(0));

	/* update priority for current thread */
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(BASE_PRIORITY));

	/* create threads with equal preemptive priority */
	for (int i = 0; i < NUM_THREAD; i++) {
		tid[i] = k_thread_create(&t[i], tstacks[i], STACK_SIZE,
					 thread_tslice,
					 INT_TO_POINTER(i), NULL, NULL,
					 K_PRIO_PREEMPT(BASE_PRIORITY), 0,
					 K_NO_WAIT);
	}

	/* enable time slice */
	k_sched_time_slice_set(SLICE_SIZE, K_PRIO_PREEMPT(BASE_PRIORITY));

	while (count < ITRERATION_COUNT) {
		k_uptime_delta(&elapsed_slice);

		/* Keep the current thread busy for more than one slice,
		 * even though, when timeslice used up the next thread
		 * should be scheduled in.
		 */
		spin_for_ms(BUSY_MS);

		/* relinquish CPU and wait for each thread to complete */
		for (int i = 0; i < NUM_THREAD; i++) {
			k_sem_take(&sema1, K_FOREVER);
		}
		count++;
	}


	/* test case teardown */
	for (int i = 0; i < NUM_THREAD; i++) {
		k_thread_abort(tid[i]);
	}

	/* disable time slice */
	k_sched_time_slice_set(0, K_PRIO_PREEMPT(0));

	k_thread_priority_set(k_current_get(), old_prio);
}

static volatile int32_t perthread_count;
static volatile uint32_t last_cyc;
static volatile bool perthread_running;
static K_SEM_DEFINE(perthread_sem, 0, 1);

static void slice_expired(struct k_thread *thread, void *data)
{
	zassert_equal(thread, data, "wrong callback data pointer");

	uint32_t now = k_cycle_get_32();
	uint32_t dt = k_cyc_to_ticks_near32(now - last_cyc);

	zassert_true(perthread_running, "thread didn't start");
	zassert_true(dt >= (PERTHREAD_SLICE_TICKS - TICK_SLOP),
		     "slice expired >%d ticks too soon (dt=%d)", TICK_SLOP, dt);
	zassert_true((dt - PERTHREAD_SLICE_TICKS) <= TICK_SLOP,
		     "slice expired >%d ticks late (dt=%d)", TICK_SLOP, dt);

	last_cyc = now;

	/* First time through, just let the slice expire and keep
	 * running.  Second time, abort the thread and wake up the
	 * main test function.
	 */
	if (perthread_count++ != 0) {
		k_thread_abort(thread);
		perthread_running = false;
		k_sem_give(&perthread_sem);
	}
}

static void slice_perthread_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	while (true) {
		perthread_running = true;
		k_busy_wait(10);
	}
}

ZTEST(threads_scheduling, test_slice_perthread)
{
	if (!IS_ENABLED(CONFIG_TIMESLICE_PER_THREAD)) {
		ztest_test_skip();
		return;
	}

	/* Create the thread but don't start it */
	k_thread_create(&t[0], tstacks[0], STACK_SIZE,
			slice_perthread_fn, NULL, NULL, NULL,
			1, 0, K_FOREVER);
	k_thread_time_slice_set(&t[0], PERTHREAD_SLICE_TICKS, slice_expired, &t[0]);

	/* Tick align, set up, then start */
	k_usleep(1);
	last_cyc = k_cycle_get_32();
	k_thread_start(&t[0]);

	k_sem_take(&perthread_sem, K_FOREVER);
	zassert_false(perthread_running, "thread failed to suspend");
}

#else /* CONFIG_TIMESLICING */
ZTEST(threads_scheduling, test_slice_scheduling)
{
	ztest_test_skip();
}
ZTEST(threads_scheduling, test_slice_perthread)
{
	ztest_test_skip();
}
#endif /* CONFIG_TIMESLICING */
