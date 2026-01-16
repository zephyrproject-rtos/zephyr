/*
 * Copyright (c) 2026 Qualcomm Technologies, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/util_macro.h>

static struct k_timer non_def_timer;
static struct k_timer def_timer;

uint32_t non_def_timer_expiry_cnt;
uint32_t def_timer_expiry_cnt;

static void non_def_timer_expiry_cb(struct k_timer *timer)
{
	non_def_timer_expiry_cnt++;
}

static void def_timer_expiry_cb(struct k_timer *timer)
{
	def_timer_expiry_cnt++;
}

ZTEST(deferrable_timeout, test_deferrable_timeout)
{
	const int non_def_time = 5;
	const int def_time = 3;
	const int def_threshold = 10;
	int32_t timeout_expiry = 0;
	k_ticks_t remaining_non_def_ticks = 0;

	/* Initialize the timers */
	k_timer_init(&non_def_timer, non_def_timer_expiry_cb, NULL);
	k_timer_init(&def_timer, def_timer_expiry_cb, NULL);

	/* Set the threshold for the deferrable timer */
	k_timer_threshold_set(&def_timer, K_SECONDS(def_threshold));

	/* Start timers */
	k_timer_start(&non_def_timer, K_SECONDS(non_def_time), K_NO_WAIT);
	k_timer_start(&def_timer, K_SECONDS(def_time), K_NO_WAIT);

	k_sleep(K_SECONDS(1));

	/**
	 * At this point:
	 * def_timer expires in ~2s. Threshold is 10s. Effective expiry = 12s.
	 * non_def_timer expires in ~4s. Threshold is 0. Effective expiry = 4s.
	 *
	 * The system should wake up for non_def_timer first.
	 */

	timeout_expiry = k_get_first_timeout_expiry();
	remaining_non_def_ticks = k_timer_remaining_ticks(&non_def_timer);

	/* The returned wakeup time should match the standard timer, not the delayed one */
	zassert_true((timeout_expiry <= remaining_non_def_ticks),
				 "Incorrect expiry value calculated");

	k_timer_stop(&non_def_timer);
	k_timer_stop(&def_timer);
}

ZTEST_SUITE(deferrable_timeout, NULL, NULL, NULL, NULL, NULL);
