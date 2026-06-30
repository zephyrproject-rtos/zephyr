/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "test_sched.h"

#ifdef CONFIG_TIMESLICING

#define NUM_THREAD 3

BUILD_ASSERT(NUM_THREAD <= MAX_NUM_THREAD);

/* slice size in milliseconds (kernel slicing API takes ms) */
#define SLICE_SIZE_MS 200
/* busy-wait duration: more than one slice so the slicer fires */
#define BUSY_MS (SLICE_SIZE_MS + 20)

/* All measurement bounds are expressed in ticks: that's the granularity
 * at which preemptive scheduling happens. Each measured slice lands on
 * exactly one of two tick values -- the configured slice or one tick
 * later, due to z_add_timeout()'s "+1" round-up that guarantees an
 * at-least-N tick delay.
 */
#define SLICE_TICKS k_ms_to_ticks_ceil32(SLICE_SIZE_MS)
#define HALF_SLICE_TICKS (SLICE_TICKS / 2)

K_SEM_DEFINE(sema, 0, NUM_THREAD);
/* Reference timestamp (in ticks) for each measurement */
static uint64_t elapsed_slice;
static int thread_idx;

static void thread_time_slice(void *p1, void *p2, void *p3)
{
	uint32_t tick_delta = ticks_delta(&elapsed_slice);
	/*
	 * Thread 0 picks up CPU when the main test thread voluntarily
	 * yields halfway through its slice, so its elapsed measurement
	 * spans the busy-wait of half a slice. The remaining threads see
	 * the previous thread's full slice between successive wakeups.
	 */
	uint32_t expected = (thread_idx == 0) ? HALF_SLICE_TICKS : SLICE_TICKS;

#ifdef CONFIG_DEBUG
	TC_PRINT("thread[%d] elapsed: %u ticks, expected ~%u\n",
		 thread_idx, tick_delta, expected);
#endif

	/* Update before the assert so a failure log doesn't suggest the
	 * same thread ran more than once.
	 */
	thread_idx = (thread_idx + 1) % NUM_THREAD;

#ifndef CONFIG_COVERAGE_GCOV
	/* TESTPOINT: a measured slice falls on either the configured tick
	 * boundary or the next one (kernel "+1" round-up).
	 */
	zassert_between_inclusive(tick_delta, expected, expected + 1,
				  "elapsed %u ticks, expected ~%u",
				  tick_delta, expected);
#else
	(void)tick_delta;
#endif /* CONFIG_COVERAGE_GCOV */

	/* Keep this thread busy past one slice so the slicer fires and
	 * hands the CPU to the next thread.
	 */
	spin_for_ms(BUSY_MS);
	k_sem_give(&sema);
}

#endif /* CONFIG_TIMESLICING */

/* test cases */
/**
 * @brief Check the behavior of preemptive threads when the
 * time slice is disabled and enabled
 *
 * @details Create multiple preemptive threads with few different
 * priorities and few with same priorities and enable the time slice.
 * Ensure that each thread is given the time slice period to execute.
 *
 * Skipped when CONFIG_TIMESLICING is disabled.
 *
 * @see k_sched_time_slice_set(), k_sem_reset(), k_cycle_get_32(),
 *      k_uptime_get_32()
 *
 * @ingroup tests_kernel_sched
 */
ZTEST(threads_scheduling, test_slice_reset)
{
#ifdef CONFIG_TIMESLICING
	uint32_t t32;
	k_tid_t tid[NUM_THREAD];
	struct k_thread t[NUM_THREAD];
	int old_prio = k_thread_priority_get(k_current_get());

	thread_idx = 0;
	/* disable timeslice */
	k_sched_time_slice_set(0, K_PRIO_PREEMPT(0));

	/* Size the busy-wait as a whole number of ticks using the driver's
	 * real cycles per tick, floor(HW/ticks). Converting the half-slice
	 * with the fractional ratio (k_ticks_to_cyc_ceil32()) would skew the
	 * measured ticks where a tick isn't a whole number of cycles.
	 */
	uint32_t cyc_per_tick = k_ticks_to_cyc_floor32(1);
	uint32_t half_slice_cyc = HALF_SLICE_TICKS * cyc_per_tick;

	for (int j = 0; j < 2; j++) {
		k_sem_reset(&sema);

		/* update priority for current thread */
		k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(j));

		/* synchronize to tick boundary */
		k_usleep(1);

		/* create delayed threads with equal preemptive priority */
		for (int i = 0; i < NUM_THREAD; i++) {
			tid[i] = k_thread_create(&t[i], tstacks[i], STACK_SIZE,
						 thread_time_slice, NULL, NULL,
						 NULL, K_PRIO_PREEMPT(j), 0,
						 K_NO_WAIT);
		}

		/* enable time slice (and reset the counter!) */
		k_sched_time_slice_set(SLICE_SIZE_MS, K_PRIO_PREEMPT(0));

		/* initialize reference timestamp */
		ticks_delta(&elapsed_slice);

		/* consume half a timeslice, timed on the free-running cycle
		 * counter (independent of the tick interrupt)
		 */
		t32 = k_cycle_get_32();
		while (k_cycle_get_32() - t32 < half_slice_cyc) {
			Z_SPIN_DELAY(50);
		}

		/* relinquish CPU and wait for each thread to complete */
		k_sleep(K_TICKS(SLICE_TICKS * (NUM_THREAD + 1)));
		for (int i = 0; i < NUM_THREAD; i++) {
			k_sem_take(&sema, K_FOREVER);
		}

		/* test case teardown */
		for (int i = 0; i < NUM_THREAD; i++) {
			k_thread_abort(tid[i]);
		}
		/* disable time slice */
		k_sched_time_slice_set(0, K_PRIO_PREEMPT(0));
	}
	k_thread_priority_set(k_current_get(), old_prio);
#else
	ztest_test_skip();
#endif /* CONFIG_TIMESLICING */
}
