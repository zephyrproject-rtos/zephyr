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
	int32_t timeout_expiry = 0;
	k_ticks_t remaining_def_ticks = 0;
	k_ticks_t remaining_non_def_ticks = 0;

	/* Initialize the timers */
	k_timer_init(&non_def_timer, non_def_timer_expiry_cb, NULL);
	k_timer_init(&def_timer, def_timer_expiry_cb, NULL);

	/* Set the timers as deferrable mode timers */
	k_timer_deferrable_set(&def_timer);

	/* Start timers */
	k_timer_start(&non_def_timer, K_SECONDS(non_def_time), K_NO_WAIT);
	k_timer_start(&def_timer, K_SECONDS(def_time), K_NO_WAIT);

	k_sleep(K_SECONDS(2));

	timeout_expiry = k_get_next_non_deferrable_timeout_expiry();
	remaining_def_ticks = k_timer_remaining_ticks(&def_timer);
	remaining_non_def_ticks = k_timer_remaining_ticks(&non_def_timer);

	/* timeout_expiry should not have expiry details of def_timer */
	zassert_true((timeout_expiry != remaining_def_ticks), "def_timer expiry set");
	zassert_true((timeout_expiry <= remaining_non_def_ticks), "Incorrect expiry value");

	k_timer_stop(&non_def_timer);
	k_timer_stop(&def_timer);
}

ZTEST_SUITE(deferrable_timeout, NULL, NULL, NULL, NULL, NULL);
