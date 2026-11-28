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

/* Independent hardware-time reference for the drift check below. */
static inline uint64_t drift_hw_cycles(void)
{
#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
	return k_cycle_get_64();
#else
	return k_cycle_get_32();
#endif
}

static void drift_timer_fn(struct k_timer *timer)
{
	ARG_UNUSED(timer);
}
static K_TIMER_DEFINE(drift_timer, drift_timer_fn, NULL);

/**
 * @brief Uptime must track the hardware clock under late tick announcements
 *
 * @details With a periodic timer running, keep the CPU busy in interrupt-
 * locked bursts so several ticks elapse between announcements. A tickless
 * driver that drops the sub-tick remainder when it advances its last-
 * announced cycle count makes k_uptime_get() fall behind the free-running
 * hardware counter. Compare the two over a fixed window of hardware time and
 * require them to agree. Platforms whose uptime and cycle counter share one
 * time base see no drift and pass; the check only bites a driver that loses
 * cycles on every late interrupt.
 */
ZTEST(tickless_concept, test_tickless_clock_tracks_hw)
{
	const uint32_t hz = sys_clock_hw_cycles_per_sec();
	const uint64_t window = (uint64_t)hz * 2U; /* ~2 s of hardware time */
	uint32_t lcg = 0x1234567U;
	uint32_t prev;
	uint64_t cyc_elapsed = 0U;
	int64_t up0, up_ms, real_ms, diff_ms, tol_ms;

	k_timer_start(&drift_timer, K_MSEC(1), K_MSEC(1));

	up0 = k_uptime_get();
	prev = (uint32_t)drift_hw_cycles();

	do {
		unsigned int key;
		uint32_t now;

		key = irq_lock();
		lcg = lcg * 1103515245U + 12345U;
		/* 1..3 ms locked: every burst crosses at least one tick so a
		 * driver that drops the sub-tick remainder loses cycles here.
		 */
		k_busy_wait(1000U + (lcg >> 16) % 2000U);
		irq_unlock(key);

		now = (uint32_t)drift_hw_cycles();
		cyc_elapsed += (uint32_t)(now - prev); /* wrap-safe per burst */
		prev = now;
	} while (cyc_elapsed < window);

	up_ms = k_uptime_get() - up0;
	k_timer_stop(&drift_timer);

	real_ms = (int64_t)(cyc_elapsed * 1000U / hz);
	diff_ms = up_ms - real_ms;
	if (diff_ms < 0) {
		diff_ms = -diff_ms;
	}
	tol_ms = real_ms * 3 / 100; /* 3 % */

	TC_PRINT("uptime=%lld ms, hw=%lld ms, drift=%lld ms (tolerance %lld ms)\n",
		 (long long)up_ms, (long long)real_ms, (long long)diff_ms, (long long)tol_ms);

	zassert_true(diff_ms <= tol_ms,
		     "k_uptime drifted %lld ms from the hardware clock over %lld ms",
		     (long long)diff_ms, (long long)real_ms);
}

/**
 * @}
 */

ZTEST_SUITE(tickless_concept, NULL, NULL,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
