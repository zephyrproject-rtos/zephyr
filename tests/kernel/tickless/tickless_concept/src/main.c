/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/pm/pm.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define NUM_THREAD 4
static K_THREAD_STACK_ARRAY_DEFINE(tstack, NUM_THREAD, STACK_SIZE);
static struct k_thread tdata[NUM_THREAD];

/* Sleep duration for the tickless / tickful cases, in milliseconds (the
 * unit kernel sleep APIs accept).
 */
#define SLEEP_TICKLESS_MS  k_ticks_to_ms_floor64(k_ms_to_ticks_floor64(200))
#define SLEEP_TICKFUL_MS   k_ticks_to_ms_floor64(k_ms_to_ticks_floor64(200) - 1)

/* Slice is half of the sleep duration.  Defined in ms (kernel slicing
 * API takes ms) and ticks (measurements compare in ticks).
 */
#define SLICE_SIZE_MS      (SLEEP_TICKLESS_MS / 2)
#define SLICE_TICKS        k_ms_to_ticks_ceil32(SLICE_SIZE_MS)

/*align to millisecond boundary*/
#define ALIGN_MS_BOUNDARY()		       \
	do {				       \
		uint32_t t = k_uptime_get_32();   \
		while (t == k_uptime_get_32()) \
			Z_SPIN_DELAY(50);       \
	} while (0)

K_SEM_DEFINE(sema, 0, NUM_THREAD);
/* Reference timestamp (in ticks) for measuring slice durations */
static uint64_t elapsed_slice;

static uint32_t ticks_delta(uint64_t *reftime)
{
	uint64_t now = k_uptime_ticks();
	uint32_t delta = now - *reftime;

	*reftime = now;
	return delta;
}

static void thread_tslice(void *p1, void *p2, void *p3)
{
	uint32_t tick_delta = ticks_delta(&elapsed_slice);

	TC_PRINT("elapsed slice %u ticks, expected ~%u\n",
		 tick_delta, (uint32_t)SLICE_TICKS);

	/* TESTPOINT: a measured slice falls on either the configured tick
	 * boundary or the next one (kernel "+1" round-up).
	 */
	zassert_between_inclusive(tick_delta, SLICE_TICKS, SLICE_TICKS + 1,
				  "elapsed %u ticks, expected ~%u",
				  tick_delta, (uint32_t)SLICE_TICKS);

	/*keep the current thread busy for more than one slice*/
	k_busy_wait(1000 * SLEEP_TICKLESS_MS);
	k_sem_give(&sema);
}
/**
 * @defgroup  kernel_tickless_tests Tickless
 * @ingroup all_tests
 * @{
 */


/**
 * @brief Verify system clock with and without tickless idle
 *
 * @details Check if system clock recovers and works as expected
 * when tickless idle is enabled and disabled.
 */
ZTEST(tickless_concept, test_tickless_sysclock)
{
	volatile uint32_t t0, t1;

	ALIGN_MS_BOUNDARY();
	t0 = k_uptime_get_32();
	k_msleep(SLEEP_TICKLESS_MS);
	t1 = k_uptime_get_32();
	TC_PRINT("time %d, %d\n", t0, t1);
	/**TESTPOINT: verify system clock recovery after exiting tickless idle*/
	zassert_true((t1 - t0) >= SLEEP_TICKLESS_MS);

	ALIGN_MS_BOUNDARY();
	t0 = k_uptime_get_32();
	k_sem_take(&sema, K_MSEC(SLEEP_TICKFUL_MS));
	t1 = k_uptime_get_32();
	TC_PRINT("time %d, %d\n", t0, t1);
	/**TESTPOINT: verify system clock recovery after exiting tickful idle*/
	zassert_true((t1 - t0) >= SLEEP_TICKFUL_MS);
}

/**
 * @brief Verify tickless functionality with time slice
 *
 * @details Create threads of equal priority and enable time
 * slice. Check if the threads execute more than a tick.
 */
ZTEST(tickless_concept, test_tickless_slice)
{
	k_tid_t tid[NUM_THREAD];

	k_sem_reset(&sema);
	/*enable time slice*/
	k_sched_time_slice_set(SLICE_SIZE_MS, K_PRIO_PREEMPT(0));

	/*create delayed threads with equal preemptive priority*/
	for (int i = 0; i < NUM_THREAD; i++) {
		tid[i] = k_thread_create(&tdata[i], tstack[i], STACK_SIZE,
					 thread_tslice, NULL, NULL, NULL,
					 K_PRIO_PREEMPT(0), 0,
					 K_MSEC(SLICE_SIZE_MS));
	}
	ticks_delta(&elapsed_slice);
	/*relinquish CPU and wait for each thread to complete*/
	for (int i = 0; i < NUM_THREAD; i++) {
		k_sem_take(&sema, K_FOREVER);
	}

	/*test case teardown*/
	for (int i = 0; i < NUM_THREAD; i++) {
		k_thread_abort(tid[i]);
	}
	/*disable time slice*/
	k_sched_time_slice_set(0, K_PRIO_PREEMPT(0));
}

/**
 * @}
 */

ZTEST_SUITE(tickless_concept, NULL, NULL,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
