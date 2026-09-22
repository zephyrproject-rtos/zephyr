/*
 * Copyright (c) 2025 Qualcomm Technologies, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/atomic.h>

static struct k_timer test_periodic_timer;

static int expiry_cnt;
static int stop_cnt;

struct obs_state_s {
	int init_cnt;
	int start_cnt;
	int stop_cnt;
	int expiry_cnt;
};

static struct obs_state_s obs;

static void timer_expiry_cb(struct k_timer *timer)
{
	expiry_cnt++;
}

static void timer_stop_cb(struct k_timer *timer)
{
	stop_cnt++;
}

__boot_func
static void obs_on_init(struct k_timer *timer)
{
	if (timer == &test_periodic_timer) {
		obs.init_cnt++;
	}
}

__boot_func
static void obs_on_start(struct k_timer *timer, k_timeout_t duration, k_timeout_t period)
{
	if (timer == &test_periodic_timer) {
		obs.start_cnt++;
	}
}

__boot_func
static void obs_on_stop(struct k_timer *timer)
{
	if (timer == &test_periodic_timer) {
		obs.stop_cnt++;
	}
}

__boot_func
static void obs_on_expiry(struct k_timer *timer)
{
	if (timer == &test_periodic_timer) {
		obs.expiry_cnt++;
	}
}

ZTEST(timer_observer, test_periodic_expiry_and_explicit_stop)
{
	const int dur_ms = 80;
	const int per_ms = 40;

	/* Initialize the timer to trigger on_init path */
	k_timer_init(&test_periodic_timer, timer_expiry_cb, timer_stop_cb);

	/* Start periodic timer*/
	k_timer_start(&test_periodic_timer, K_MSEC(dur_ms), K_MSEC(per_ms));

	/* Allow a few expiries to happen */
	k_busy_wait((dur_ms + (per_ms * 3)) * 1000);

	/* Now explicitly stop; observer should see on_stop */
	k_timer_stop(&test_periodic_timer);

	/* Small delay to ensure stop path completes */
	k_busy_wait(10 * 1000);

	/* Verify timer on_init was called and updated for the test timer */
	zassert_equal(obs.init_cnt, 1, "obs init count mismatch");

	/* Observer should have seen start */
	zassert_equal(obs.start_cnt, 1, "obs start count mismatch");

	zassert_equal(obs.stop_cnt, stop_cnt, "obs stop count mismatch");

	/* Observer should have seen multiple expiry callbacks */
	zassert_equal(obs.expiry_cnt, expiry_cnt, "obs expiry count mismatch");
}

K_TIMER_OBSERVER_DEFINE(observer, obs_on_init, obs_on_start, obs_on_stop, obs_on_expiry);

ZTEST_SUITE(timer_observer, NULL, NULL, NULL, NULL, NULL);
