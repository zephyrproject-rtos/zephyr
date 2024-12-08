/*
 * Copyright (c) 2024 Akaiwa Wataru <akaiwa@sonas.co.jp>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static k_tid_t thread_id;

static void alarm_callback(struct k_timer *timer)
{
	k_wakeup(thread_id);
}

K_TIMER_DEFINE(alarm, alarm_callback, NULL);

/**
 * @brief Test 32-bit tick wraparound during k_sleep() execution
 */
ZTEST(wraparound, test_tick_wraparound_in_sleep)
{
	static const uint32_t start_ticks = 0xffffff00; /* It wraps around after 256 ticks! */
	static const uint32_t timeout_ticks = 300;      /* 3 seconds @ 100Hz tick */
	static const uint32_t wakeup_ticks = 10;        /* 100 ms @ 100Hz tick */

	sys_clock_tick_set(start_ticks);

	/* Wake up myself by alarm */
	thread_id = k_current_get();
	k_timer_start(&alarm, K_TICKS(wakeup_ticks), K_FOREVER);

	/* Waiting alarm's k_wakeup() call */
	int32_t left_ms = k_sleep(K_TICKS(timeout_ticks));

	zassert(left_ms > 0, "k_sleep() timed out");

	k_timer_stop(&alarm);
}

ZTEST_SUITE(wraparound, NULL, NULL, NULL, NULL, NULL);
