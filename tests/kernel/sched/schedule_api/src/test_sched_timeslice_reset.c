/*
 * Copyright (c) 2017,2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "test_sched.h"

#define NUM_THREAD 3

#define SLICE_SIZE 200

/* a half timeslice */
#define HALF_SLICE_SIZE (SLICE_SIZE >> 1)
#define HALF_SLICE_SIZE_CYCLES                                                 \
	((uint64_t)(HALF_SLICE_SIZE)*sys_clock_hw_cycles_per_sec() / 1000)

/* 1/4 of a time slice */
#define QUARTER_SLICE_SIZE  (SLICE_SIZE / 4)
#define QUARTER_SLICE_SIZE_CYCLES                                              \
	((uint64_t)(QUARTER_SLICE_SIZE) * sys_clock_hw_cycles_per_sec() / 1000)

/* 3/4 of a time slice */
#define THREE_QUARTER_SLICE_SIZE  ((SLICE_SIZE * 3) / 4)
#define THREE_QUARTER_SLICE_SIZE_CYCLES                                        \
	((uint64_t)(THREE_QUARTER_SLICE_SIZE) * sys_clock_hw_cycles_per_sec() / 1000)

/* Task switch tolerance ... */
#if CONFIG_SYS_CLOCK_TICKS_PER_SEC >= 1000
/* ... will not take more than 1 ms. */
#define TASK_SWITCH_TOLERANCE (1)
#else
/* ... 1ms is faster than a tick, loosen tolerance to 1 tick */
#define TASK_SWITCH_TOLERANCE (1000 / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#endif

#if defined(CONFIG_TIMESLICE_AUTO_RESET)

BUILD_ASSERT(NUM_THREAD <= MAX_NUM_THREAD);

/* slice size in millisecond */
/* busy for more than one slice */
#define BUSY_MS (SLICE_SIZE + 20)

/* Task switch tolerance ... */
#if CONFIG_SYS_CLOCK_TICKS_PER_SEC >= 1000
/* ... will not take more than 1 ms. */
#define TASK_SWITCH_TOLERANCE (1)
#else
/* ... 1ms is faster than a tick, loosen tolerance to 1 tick */
#define TASK_SWITCH_TOLERANCE (1000 / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#endif

K_SEM_DEFINE(sema, 0, NUM_THREAD);
/* elapsed_slice taken by last thread */
static uint32_t elapsed_slice;
static int thread_idx;

static uint32_t cycles_delta(uint32_t *reftime)
{
	uint32_t now, delta;

	now = k_cycle_get_32();
	delta = now - *reftime;
	*reftime = now;

	return delta;
}

static void thread_time_slice(void *p1, void *p2, void *p3)
{
	uint32_t t = cycles_delta(&elapsed_slice);
	uint32_t expected_slice_min, expected_slice_max;
	uint32_t switch_tolerance_ticks =
		k_ms_to_ticks_ceil32(TASK_SWITCH_TOLERANCE);

	if (thread_idx == 0) {
		/*
		 * Thread number 0 releases CPU after HALF_SLICE_SIZE, and
		 * expected to switch in less than the switching tolerance.
		 */
		expected_slice_min =
			(uint64_t)(HALF_SLICE_SIZE - TASK_SWITCH_TOLERANCE) *
			sys_clock_hw_cycles_per_sec() / 1000;
		expected_slice_max =
			(uint64_t)(HALF_SLICE_SIZE + TASK_SWITCH_TOLERANCE) *
			sys_clock_hw_cycles_per_sec() / 1000;
	} else {
		/*
		 * Other threads are sliced with tick granularity. Here, we
		 * also expecting task switch below the switching tolerance.
		 */
		expected_slice_min =
			(k_ms_to_ticks_floor32(SLICE_SIZE)
			 - switch_tolerance_ticks)
			* k_ticks_to_cyc_floor32(1);
		expected_slice_max =
			(k_ms_to_ticks_ceil32(SLICE_SIZE)
			 + switch_tolerance_ticks)
			* k_ticks_to_cyc_ceil32(1);
	}

#ifdef CONFIG_DEBUG
	TC_PRINT("thread[%d] elapsed slice: %d, expected: <%d, %d>\n",
		 thread_idx, t, expected_slice_min, expected_slice_max);
#endif

	/* Before the assert, otherwise in case of fail the output
	 * will give the impression that the same thread ran more than
	 * once
	 */
	thread_idx = (thread_idx + 1) % NUM_THREAD;

	/** TESTPOINT: timeslice should be reset for each preemptive thread */
#ifndef CONFIG_COVERAGE_GCOV
	zassert_true(t >= expected_slice_min,
		     "timeslice too small, expected %u got %u",
		     expected_slice_min, t);
	zassert_true(t <= expected_slice_max,
		     "timeslice too big, expected %u got %u",
		     expected_slice_max, t);
#else
	(void)t;
#endif /* CONFIG_COVERAGE_GCOV */

	/* Keep the current thread busy for more than one slice, even though,
	 * when timeslice used up the next thread should be scheduled in.
	 */
	spin_for_ms(BUSY_MS);
	k_sem_give(&sema);
}

/* test cases */
/**
 * @brief Check the behavior of preemptive threads when the
 * time slice is disabled and enabled
 *
 * @details Create multiple preemptive threads with few different
 * priorities and few with same priorities and enable the time slice.
 * Ensure that each thread is given the time slice period to execute.
 *
 * @see k_sched_time_slice_set(), k_sem_reset(), k_cycle_get_32(),
 *      k_uptime_get_32()
 *
 * @ingroup kernel_sched_tests
 */
ZTEST(threads_scheduling, test_slice_reset)
{
	uint32_t t32;
	k_tid_t tid[NUM_THREAD];
	struct k_thread t[NUM_THREAD];
	int old_prio = k_thread_priority_get(k_current_get());

	thread_idx = 0;
	/* disable timeslice */
	k_sched_time_slice_set(0, K_PRIO_PREEMPT(0));

	/* The slice size needs to be set in ms (which get converted
	 * into ticks internally), but we want to loop over a half
	 * slice in cycles. That requires a bit of care to be sure the
	 * value divides properly.
	 */
	uint32_t slice_ticks = k_ms_to_ticks_ceil32(SLICE_SIZE);
	uint32_t half_slice_cyc = k_ticks_to_cyc_ceil32(slice_ticks / 2);

	if (slice_ticks % 2 != 0) {
		uint32_t deviation = k_ticks_to_cyc_ceil32(1);
		/* slice_ticks can't be divisible by two, so we add the
		 * (slice_ticks / 2) floating part back to half_slice_cyc.
		 */
		half_slice_cyc = half_slice_cyc + (deviation / 2);
	}

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
		k_sched_time_slice_set(SLICE_SIZE, K_PRIO_PREEMPT(0));

		/* initialize reference timestamp */
		cycles_delta(&elapsed_slice);

		/* current thread (ztest native) consumed a half timeslice */
		t32 = k_cycle_get_32();
		while (k_cycle_get_32() - t32 < half_slice_cyc) {
			Z_SPIN_DELAY(50);
		}

		/* relinquish CPU and wait for each thread to complete */
		k_sleep(K_TICKS(slice_ticks * (NUM_THREAD + 1)));
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
}
#elif defined(CONFIG_TIMESLICING)

static uint32_t saved_cycles[30];
static uint32_t saved_cycles_index;
static struct k_spinlock lock;
static bool do_yield;

/*
 * 3 threads running in parallel on a UP system.
 * 1st thread consumes 3/4 of the time slice.
 * 2nd thread consumes remaining 1/4 time slice before time slice expires.
 * 3rd thread consumes 3/4 of the time slice
 * 1st thread consumes remaining 1/4 time slice before time slice expires.
 * 2nd thread consumes 3/4 of the time slice.
 * ...
 */
static void thread_entry(void *p1, void *p2, void *p3)
{
	k_spinlock_key_t  key;
	bool saved_yield;

	for (unsigned int i = 0; i < 10; i++) {
		key = k_spin_lock(&lock);
		/* Save cycle info for post-processing */
		saved_cycles[saved_cycles_index] = k_cycle_get_32();
		saved_cycles_index++;

		/* Toggle the yield decision and save it */
		do_yield ^= true;
		saved_yield = do_yield;
		k_spin_unlock(&lock, key);

		spin_for_ms(THREE_QUARTER_SLICE_SIZE);
		if (saved_yield) {
			k_yield();
		}
	}
}

ZTEST(threads_scheduling, test_slice_reset)
{
	struct k_thread t[NUM_THREAD];
	uint32_t  diff;
	uint32_t  tolerance = sys_clock_hw_cycles_per_sec() / 500;
	uint32_t  expected_consumption;
	unsigned int i;

	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(5));
	k_sched_time_slice_set(0, K_PRIO_PREEMPT(0));

	/* Align to a tick boundary */
	k_sleep(K_TICKS(1));

	for (i = 0; i < NUM_THREAD; i++) {
		k_thread_create(&t[i], tstacks[i], STACK_SIZE,
				thread_entry, NULL, NULL, NULL,
				K_PRIO_PREEMPT(6), 0, K_NO_WAIT);
	}

	k_sched_time_slice_set(SLICE_SIZE, K_PRIO_PREEMPT(0));

	for (i = 0; i < NUM_THREAD; i++) {
		k_thread_join(&t[i], K_FOREVER);
	}

	for (i = 1; i < 30; i++) {
		diff = saved_cycles[i] - saved_cycles[i - 1];

		expected_consumption = (i & 1) ? THREE_QUARTER_SLICE_SIZE_CYCLES
					       : QUARTER_SLICE_SIZE_CYCLES;

		/*
		 * Allow for +/- 2 msec. (It has been observed on heavily
		 * loaded systems that +/- 1 msec is sometimes ever so
		 * slightly too tight.)
		 */

		zassert_true(diff >= expected_consumption - tolerance,
			     "Consumed too little of slice %u. Expected at least %u. Got %u.",
			     i, expected_consumption - tolerance, diff);

		zassert_true(diff <= expected_consumption + tolerance,
			     "Consumed too much of slice %u. Expected at most %u. Got %u.",
			     i, expected_consumption + tolerance, diff);
	}
}
#else /* !CONFIG_TIMESLICING */
ZTEST(threads_scheduling, test_slice_reset)
{
	ztest_test_skip();
}
#endif /* CONFIG_TIMESLICING */
