/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/atomic.h>

#define CYC_PER_TICK                                                                               \
	((uint64_t)sys_clock_hw_cycles_per_sec() / (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define SLEEP_TIME_MS                200U
#define TIMER_COUNT_TIME_MS          10U
#define WAIT_FOR_TIMER_EVENT_TIME_MS (TIMER_COUNT_TIME_MS + 5U)
#define NUMBER_OF_TRIES              2000U

static atomic_t g_cnt;
static void inc_cb(struct k_timer *t)
{
	atomic_inc(&g_cnt);
}

K_TIMER_DEFINE(oneshot_t, inc_cb, NULL);
K_TIMER_DEFINE(abort_t, inc_cb, NULL);
K_TIMER_DEFINE(period_t, inc_cb, NULL);

static inline uint32_t ticks_to_ms(uint32_t ticks)
{
	uint32_t tps = CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	uint32_t num = ticks * 1000U + (tps - 1U);

	return num / tps;
}

ZTEST(nxp_utick_timer, test_sleep_ms_accuracy)
{
	uint32_t now = k_uptime_get_32();

	k_msleep(SLEEP_TIME_MS);
	uint32_t delta = k_uptime_get_32() - now;

	zassert_true(delta == SLEEP_TIME_MS, "Real slept time %d not equal to %d", delta,
		     SLEEP_TIME_MS);
}

ZTEST(nxp_utick_timer, test_timer_count_in_oneshot_mode)
{
	atomic_clear(&g_cnt);
	k_timer_start(&oneshot_t, K_MSEC(TIMER_COUNT_TIME_MS), K_NO_WAIT);

	k_msleep(WAIT_FOR_TIMER_EVENT_TIME_MS);
	zassert_equal((uint32_t)atomic_get(&g_cnt), 1U, "oneshot not fired exactly once");
	k_msleep(50);
	zassert_equal((uint32_t)atomic_get(&g_cnt), 1U, "oneshot fired more than once");
}

ZTEST(nxp_utick_timer, test_timer_abort_in_oneshot_mode)
{
	atomic_clear(&g_cnt);
	k_timer_start(&abort_t, K_MSEC(TIMER_COUNT_TIME_MS), K_NO_WAIT);
	k_timer_stop(&abort_t);

	k_msleep(WAIT_FOR_TIMER_EVENT_TIME_MS);
	zassert_equal((uint32_t)atomic_get(&g_cnt), 0U, "abort should not fire");
}

ZTEST(nxp_utick_timer, test_timer_count_in_periodic_mode)
{
	const uint32_t per_ms = 10U, run_ms = 300U;
	uint32_t p_cnt, exp;

	atomic_clear(&g_cnt);
	k_timer_start(&period_t, K_MSEC(per_ms), K_MSEC(per_ms));
	k_msleep(run_ms);
	k_timer_stop(&period_t);
	p_cnt = (uint32_t)atomic_get(&g_cnt);
	exp = run_ms / per_ms;
	zassert_true(p_cnt == exp, "period count %u not equal to %u", p_cnt, exp);
}

/* For UTICK timer, sys_clock_elapsed() may always return 0 */
ZTEST(nxp_utick_timer, test_sys_clock_elapsed_zero)
{
	zassert_equal(sys_clock_elapsed(), 0U, NULL);
	k_yield();
	zassert_equal(sys_clock_elapsed(), 0U, NULL);
}

ZTEST_SUITE(nxp_utick_timer, NULL, NULL, NULL, NULL, NULL);
