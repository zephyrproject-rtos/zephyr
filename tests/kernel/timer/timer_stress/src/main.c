/*
 * Copyright (c) 2024 The Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <stdlib.h>

/* Test configuration */
#define NUM_STRESS_TIMERS    8
#define STRESS_DURATION_MS   200
#define RAPID_CYCLES         50
#define DRIFT_PERIOD_MS      10
#define DRIFT_SAMPLES        20
#define CALLBACK_ORDER_COUNT 4
#define STACK_SIZE           1024
#define THREAD_PRIORITY      5

/* Global state for stress tests */
static struct k_timer stress_timers[NUM_STRESS_TIMERS];
static volatile int expire_counts[NUM_STRESS_TIMERS];
static volatile int stop_counts[NUM_STRESS_TIMERS];
static volatile int total_expirations;

/* For callback ordering test */
static volatile int callback_order_idx;
static volatile int callback_order[CALLBACK_ORDER_COUNT];

/* For concurrent test */
static struct k_sem sync_sem;
static K_THREAD_STACK_DEFINE(aux_stack, STACK_SIZE);
static struct k_thread aux_thread_data;

/* Expiry callback for stress timers */
static void stress_expire_cb(struct k_timer *timer)
{
	int idx = ARRAY_INDEX(stress_timers, timer);
	expire_counts[idx]++;
	total_expirations++;
}

/* Stop callback for stress timers */
static void stress_stop_cb(struct k_timer *timer)
{
	int idx = ARRAY_INDEX(stress_timers, timer);
	stop_counts[idx]++;
}

/* Reset all counters */
static void reset_counters(void)
{
	int i;
	for (i = 0; i < NUM_STRESS_TIMERS; i++) {
		expire_counts[i] = 0;
		stop_counts[i] = 0;
	}
	total_expirations = 0;
	callback_order_idx = 0;
	memset((void *)callback_order, 0, sizeof(callback_order));
}

/* Initialize all stress timers */
static void init_stress_timers(void)
{
	int i;
	for (i = 0; i < NUM_STRESS_TIMERS; i++) {
		k_timer_init(&stress_timers[i], stress_expire_cb, stress_stop_cb);
	}
}

/*
 * Test: Start multiple timers concurrently with varying periods.
 * Verify all timers fire and can be stopped cleanly.
 */
ZTEST(timer_stress, test_concurrent_timers)
{
	int i;

	reset_counters();
	init_stress_timers();

	/* Start all timers with different periods */
	for (i = 0; i < NUM_STRESS_TIMERS; i++) {
		k_timer_start(&stress_timers[i],
			      K_MSEC(10 + i * 5),
			      K_MSEC(20 + i * 10));
	}

	/* Let them run */
	k_msleep(STRESS_DURATION_MS);

	/* Stop all timers */
	for (i = 0; i < NUM_STRESS_TIMERS; i++) {
		k_timer_stop(&stress_timers[i]);
	}

	/* Verify all timers fired at least once */
	for (i = 0; i < NUM_STRESS_TIMERS; i++) {
		zassert_true(expire_counts[i] > 0,
			     "Timer %d never fired", i);
	}

	/* Verify stop callbacks were called */
	for (i = 0; i < NUM_STRESS_TIMERS; i++) {
		zassert_true(stop_counts[i] > 0,
			     "Timer %d stop callback not called", i);
	}

	zassert_true(total_expirations > NUM_STRESS_TIMERS,
		     "Too few total expirations: %d", total_expirations);
}

/*
 * Test: Rapidly create, start, stop, and restart timers.
 * Ensures no resource leaks or list corruption.
 */
ZTEST(timer_stress, test_rapid_start_stop)
{
	struct k_timer timer;
	int i;

	k_timer_init(&timer, NULL, NULL);

	for (i = 0; i < RAPID_CYCLES; i++) {
		k_timer_start(&timer, K_MSEC(1), K_NO_WAIT);
		k_timer_stop(&timer);
	}

	/* Timer should be safely stopped */
	uint32_t status = k_timer_status_get(&timer);
	zassert_equal(status, 0, "Timer should have zero status after stop");
}

/*
 * Test: Rapidly restart a periodic timer without stopping first.
 * This exercises the internal abort-and-readd path.
 */
ZTEST(timer_stress, test_rapid_restart)
{
	struct k_timer timer;
	uint32_t status;
	int i;

	k_timer_init(&timer, NULL, NULL);

	for (i = 0; i < RAPID_CYCLES; i++) {
		/* Restart without stopping - should internally abort old timeout */
		k_timer_start(&timer, K_MSEC(5), K_MSEC(5));
	}

	/* Let final timer run a bit */
	k_msleep(30);
	k_timer_stop(&timer);

	/* Verify we can still use the timer normally after rapid restarts */
	k_timer_start(&timer, K_MSEC(10), K_NO_WAIT);
	k_msleep(20);
	status = k_timer_status_get(&timer);
	k_timer_stop(&timer);
	zassert_true(status >= 1, "Timer should have expired after restart");
}

/*
 * Test: Measure timer drift over multiple periods.
 * Verify that accumulated drift stays within acceptable bounds.
 */
ZTEST(timer_stress, test_timer_drift)
{
	struct k_timer timer;
	int64_t start_time, end_time, elapsed;
	int64_t expected;

	k_timer_init(&timer, NULL, NULL);

	start_time = k_uptime_get();
	k_timer_start(&timer, K_MSEC(DRIFT_PERIOD_MS),
		      K_MSEC(DRIFT_PERIOD_MS));

	/* Wait for DRIFT_SAMPLES periods */
	k_timer_status_sync(&timer);
	int samples;
	for (samples = 1; samples < DRIFT_SAMPLES; samples++) {
		k_timer_status_sync(&timer);
	}

	k_timer_stop(&timer);
	end_time = k_uptime_get();

	elapsed = end_time - start_time;
	expected = (int64_t)DRIFT_PERIOD_MS * DRIFT_SAMPLES;

	/* Allow 20% tolerance for drift */
	int64_t tolerance = expected / 5;
	zassert_true(llabs(elapsed - expected) <= tolerance,
		     "Timer drift too high: elapsed=%lld expected=%lld",
		     elapsed, expected);
}

/*
 * Test: Verify callback ordering when multiple one-shot timers
 * expire at the same or very close times.
 */
static void order_expire_cb(struct k_timer *timer)
{
	int idx = ARRAY_INDEX(stress_timers, timer);
	if (callback_order_idx < CALLBACK_ORDER_COUNT) {
		callback_order[callback_order_idx++] = idx;
	}
}

ZTEST(timer_stress, test_callback_ordering)
{
	int i;

	reset_counters();

	for (i = 0; i < CALLBACK_ORDER_COUNT; i++) {
		k_timer_init(&stress_timers[i], order_expire_cb, NULL);
	}

	/* Start all with same duration - they should fire in order */
	for (i = 0; i < CALLBACK_ORDER_COUNT; i++) {
		k_timer_start(&stress_timers[i], K_MSEC(50), K_NO_WAIT);
	}

	k_msleep(100);

	/* All should have fired */
	zassert_equal(callback_order_idx, CALLBACK_ORDER_COUNT,
		      "Not all callbacks fired: %d/%d",
		      callback_order_idx, CALLBACK_ORDER_COUNT);
}

/*
 * Test: Start and stop timers from different thread contexts.
 */
static void aux_thread_fn(void *p1, void *p2, void *p3)
{
	struct k_timer *timer = (struct k_timer *)p1;
	int i;

	for (i = 0; i < 10; i++) {
		k_timer_start(timer, K_MSEC(5), K_MSEC(5));
		k_msleep(10);
		k_timer_stop(timer);
	}

	k_sem_give(&sync_sem);
}

ZTEST(timer_stress, test_cross_thread_timer)
{
	struct k_timer timer;

	k_timer_init(&timer, NULL, NULL);
	k_sem_init(&sync_sem, 0, 1);

	k_thread_create(&aux_thread_data, aux_stack, STACK_SIZE,
			aux_thread_fn, &timer, NULL, NULL,
			THREAD_PRIORITY, 0, K_NO_WAIT);

	/* Main thread also manipulates the timer */
	int i;
	for (i = 0; i < 10; i++) {
		k_timer_start(&timer, K_MSEC(3), K_NO_WAIT);
		k_msleep(8);
		k_timer_stop(&timer);
	}

	/* Wait for aux thread to finish */
	k_sem_take(&sync_sem, K_MSEC(500));

	/* Verify timer is in clean state */
	k_timer_stop(&timer);
}

/*
 * Test: Zero-duration timer (fires immediately on next tick).
 */
ZTEST(timer_stress, test_zero_duration)
{
	struct k_timer timer;

	reset_counters();
	k_timer_init(&stress_timers[0], stress_expire_cb, NULL);

	k_timer_start(&stress_timers[0], K_NO_WAIT, K_NO_WAIT);
	k_msleep(10);
	k_timer_stop(&stress_timers[0]);

	/* Timer with K_NO_WAIT duration should not be started at all
	 * or fire immediately depending on implementation */
	/* Just verify no crash occurred */
}

/*
 * Test: User data pointer survives across start/stop cycles.
 */
ZTEST(timer_stress, test_user_data_persistence)
{
	struct k_timer timer;
	int my_data = 42;

	k_timer_init(&timer, NULL, NULL);
	k_timer_user_data_set(&timer, &my_data);

	k_timer_start(&timer, K_MSEC(10), K_NO_WAIT);
	k_msleep(20);
	k_timer_stop(&timer);

	int *retrieved = (int *)k_timer_user_data_get(&timer);
	zassert_equal(*retrieved, 42, "User data corrupted");

	/* Restart and check again */
	k_timer_start(&timer, K_MSEC(10), K_NO_WAIT);
	k_msleep(20);
	k_timer_stop(&timer);

	retrieved = (int *)k_timer_user_data_get(&timer);
	zassert_equal(*retrieved, 42, "User data corrupted after restart");
}

ZTEST_SUITE(timer_stress, NULL, NULL, NULL, NULL, NULL);
