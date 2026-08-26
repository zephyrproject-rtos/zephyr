/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System timer low-power companion tests.
 *
 * Validate CONFIG_SYSTEM_TIMER_LPM_COMPANION_*: keeping time while the system
 * timer is stopped in a low-power state. Uses the public PM API only, so it
 * runs on any vendor's companion (Counter- or hook-based).
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/state.h>
#include <zephyr/sys/atomic.h>

/* Sleep durations (ms) from the zephyr,user "sleep-durations-ms" property.
 * Default in app.overlay; override per board via boards/<board>.overlay.
 */
#define SLEEP_DURATIONS_NODE DT_PATH(zephyr_user)

BUILD_ASSERT(DT_NODE_HAS_PROP(SLEEP_DURATIONS_NODE, sleep_durations_ms),
	     "define sleep-durations-ms on the zephyr,user node (see app.overlay)");

static const uint32_t sleep_durations_ms[] = DT_PROP(SLEEP_DURATIONS_NODE, sleep_durations_ms);

/* Busy-wait (us) after each announce so the console drains before the sleep
 * gates its clocks. Console readability only; never affects results. Override
 * via the zephyr,user "console-drain-us" property; default suits 115200 baud.
 */
#define CONSOLE_DRAIN_US DT_PROP_OR(SLEEP_DURATIONS_NODE, console_drain_us, 20000)

/* Allowed k_msleep() overshoot; derived from PM-state exit latencies at runtime. */
static uint32_t overshoot_tolerance_ms;

/* Shortest sleep (ms) that should trigger a low-power entry: the shallowest PM
 * state's min_residency + exit_latency (see policy_default.c). Runtime-derived.
 */
static uint32_t lpm_threshold_ms;

static atomic_t lpm_entries;
static atomic_t lpm_exits;

static void lpm_state_entry(enum pm_state state)
{
	ARG_UNUSED(state);
	atomic_inc(&lpm_entries);
}

static void lpm_state_exit(enum pm_state state)
{
	ARG_UNUSED(state);
	atomic_inc(&lpm_exits);
}

static struct pm_notifier lpm_notifier = {
	.state_entry = lpm_state_entry,
	.state_exit = lpm_state_exit,
};

static void reset_counters(void)
{
	atomic_clear(&lpm_entries);
	atomic_clear(&lpm_exits);
}

/*
 * @brief Timekeeping stays accurate across low-power sleeps.
 *
 * Elapsed uptime must be at least the request (never early) and overshoot by no
 * more than the exit latency, proving the companion kept time while the system
 * timer was stopped. Entry count: none below the threshold, exactly one above.
 * Skipped if nothing reached a low-power state.
 */
ZTEST(system_timer_lpm, test_timekeeping_accuracy)
{
	const size_t count = ARRAY_SIZE(sleep_durations_ms);
	int64_t elapsed[ARRAY_SIZE(sleep_durations_ms)];
	int32_t entries[ARRAY_SIZE(sleep_durations_ms)];
	int32_t exits[ARRAY_SIZE(sleep_durations_ms)];
	int32_t total_wakeups = 0;

	for (size_t i = 0; i < count; i++) {
		int64_t start;

		TC_PRINT("[%u/%u] sleeping %u ms\n", (unsigned int)(i + 1), (unsigned int)count,
			 sleep_durations_ms[i]);

		/* Busy-wait so the announce drains before the sleep gates clocks. */
		k_busy_wait(CONSOLE_DRAIN_US);

		reset_counters();
		start = k_uptime_get();
		k_msleep(sleep_durations_ms[i]);
		elapsed[i] = k_uptime_get() - start;
		entries[i] = (int32_t)atomic_get(&lpm_entries);
		exits[i] = (int32_t)atomic_get(&lpm_exits);
	}

	for (size_t i = 0; i < count; i++) {
		uint32_t requested = sleep_durations_ms[i];
		uint32_t guard_ms = k_ticks_to_ms_ceil32(2) + 2U;
		bool below = (requested + guard_ms <= lpm_threshold_ms);
		bool above = (requested >= lpm_threshold_ms + guard_ms);

		if (below) {
			TC_PRINT("requested %u ms, elapsed %lld ms, wakeups %d "
				 "(expected 0)\n",
				 requested, elapsed[i], entries[i]);
		} else if (above) {
			TC_PRINT("requested %u ms, elapsed %lld ms, wakeups %d "
				 "(expected 1)\n",
				 requested, elapsed[i], entries[i]);
		} else {
			TC_PRINT("requested %u ms, elapsed %lld ms, wakeups %d "
				 "(near %u ms threshold)\n",
				 requested, elapsed[i], entries[i], lpm_threshold_ms);
		}

		zassert_true(elapsed[i] >= (int64_t)requested - 1,
			     "woke early: requested %u ms, elapsed %lld ms", requested, elapsed[i]);
		zassert_true(elapsed[i] <= (int64_t)requested + overshoot_tolerance_ms,
			     "overshoot too large: requested %u ms, elapsed %lld ms "
			     "(tolerance %u ms)",
			     requested, elapsed[i], overshoot_tolerance_ms);
		zassert_equal(entries[i], exits[i],
			      "unbalanced transitions on %u ms sleep: %d entries, "
			      "%d exits",
			      requested, entries[i], exits[i]);

		/* None below the threshold; exactly one above. */
		if (below) {
			zassert_equal(entries[i], 0,
				      "%u ms sleep made %d low-power entries, "
				      "expected none",
				      requested, entries[i]);
		} else if (above) {
			zassert_equal(entries[i], 1,
				      "%u ms sleep made %d low-power entries, "
				      "expected exactly 1",
				      requested, entries[i]);
		}

		total_wakeups += entries[i];
	}

	if (total_wakeups == 0) {
		TC_PRINT("companion never engaged (no sleep reached a low-power "
			 "state); skipping\n");
		ztest_test_skip();
	}
}

static void *setup(void)
{
	const struct pm_state_info *states;
	uint8_t count;
	uint32_t max_exit_latency_us = 0U;

	count = pm_state_cpu_get_all(0U, &states);
	zassert_true(count > 0, "no PM states defined for CPU 0");

	for (uint8_t i = 0; i < count; i++) {
		if (states[i].exit_latency_us > max_exit_latency_us) {
			max_exit_latency_us = states[i].exit_latency_us;
		}
	}

	/* Overshoot budget: deepest exit latency + two ticks + small margin. */
	overshoot_tolerance_ms = (max_exit_latency_us / 1000U) + k_ticks_to_ms_ceil32(2) + 5U;

	/* States are sorted shallowest-first; states[0] sets the entry threshold. */
	lpm_threshold_ms =
		DIV_ROUND_UP(states[0].min_residency_us + states[0].exit_latency_us, 1000U);

	TC_PRINT("PM states %u, overshoot tolerance %u ms, low-power entry "
		 "threshold %u ms\n",
		 count, overshoot_tolerance_ms, lpm_threshold_ms);

	pm_notifier_register(&lpm_notifier);

	return NULL;
}

static void teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)pm_notifier_unregister(&lpm_notifier);
}

ZTEST_SUITE(system_timer_lpm, NULL, setup, NULL, NULL, teardown);
