/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "test_sched.h"

#ifdef CONFIG_TIMESLICING

/* nrf 51 has lower ram, so creating less number of threads */
#if (DT_CHOSEN_SRAM_SIZE / 1024) <= 24
	#define NUM_THREAD 2
#elif ((DT_CHOSEN_SRAM_SIZE / 1024) <= 32) \
	|| defined(CONFIG_SOC_EMSK_EM7D)
	#define NUM_THREAD 3
#else
	#define NUM_THREAD 10
#endif
#define BASE_PRIORITY 0
#define ITERATION_COUNT 5

BUILD_ASSERT(NUM_THREAD <= MAX_NUM_THREAD);
/* slice size in milliseconds (kernel slicing API takes ms) */
#define SLICE_SIZE_MS 200
/* busy-wait duration: more than one slice so the slicer fires */
#define BUSY_MS (SLICE_SIZE_MS + 20)

/* Measurement bound is in ticks: slicer-driven preemption lands on
 * either the configured tick boundary or one tick later (kernel "+1"
 * round-up in z_add_timeout()).
 */
#define SLICE_TICKS k_ms_to_ticks_ceil32(SLICE_SIZE_MS)

#define PERTHREAD_SLICE_TICKS 64

static struct k_thread t[NUM_THREAD];

static K_SEM_DEFINE(sema1, 0, NUM_THREAD);
/* Reference timestamp (in ticks) for measuring slice durations */
static uint64_t elapsed_slice;

static int thread_idx;

static void thread_tslice(void *p1, void *p2, void *p3)
{
	int idx = POINTER_TO_INT(p1);

	/* Print New line for last thread */
	int thread_parameter = (idx == (NUM_THREAD - 1)) ? '\n' :
			       (idx + 'A');

	while (1) {
		uint32_t tick_delta = ticks_delta(&elapsed_slice);

		TC_PRINT("%c", thread_parameter);
		/* Threads must run in round-robin order. */
		zassert_equal(idx, thread_idx,
			      "out of order: idx=%d thread_idx=%d",
			      idx, thread_idx);
		/* Each measured slice falls on either the configured tick
		 * boundary or the next one (kernel "+1" round-up).
		 */
		zassert_between_inclusive(tick_delta, SLICE_TICKS,
					  SLICE_TICKS + 1,
					  "tick_delta=%u expected ~%u",
					  tick_delta, SLICE_TICKS);
		thread_idx = (thread_idx + 1) % (NUM_THREAD);

		k_sem_give(&sema1);

		/* Keep the current thread busy for more than one slice,
		 * even though, when timeslice used up the next thread
		 * should be scheduled in.
		 */
		spin_for_ms(BUSY_MS);
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

	/* align to tick edge */
	k_usleep(1);

	/* create threads with equal preemptive priority */
	for (int i = 0; i < NUM_THREAD; i++) {
		tid[i] = k_thread_create(&t[i], tstacks[i], STACK_SIZE,
					 thread_tslice,
					 INT_TO_POINTER(i), NULL, NULL,
					 K_PRIO_PREEMPT(BASE_PRIORITY), 0,
					 K_NO_WAIT);
	}

	/* enable time slice */
	k_sched_time_slice_set(SLICE_SIZE_MS, K_PRIO_PREEMPT(BASE_PRIORITY));
	ticks_delta(&elapsed_slice);

	while (count < ITERATION_COUNT) {
		/* Keep the current thread busy for more than one slice,
		 * even though, when timeslice used up the next thread
		 * should be scheduled in.
		 */
		spin_for_ms(BUSY_MS);

		/* all threads should have run by now */
		ticks_delta(&elapsed_slice);
		for (int i = 0; i < NUM_THREAD; i++) {
			zassert_equal(k_sem_take(&sema1, K_NO_WAIT), 0,
				      "not all threads gave their sem");
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
	/* Slice fire lands on either the configured tick boundary or
	 * the next one due to z_add_timeout()'s "+1" round-up.
	 */
	zassert_between_inclusive(dt, PERTHREAD_SLICE_TICKS,
				  PERTHREAD_SLICE_TICKS + 1,
				  "slice expired at dt=%u, expected ~%u",
				  dt, PERTHREAD_SLICE_TICKS);

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
