/*
 * Copyright (c) 2026 Texas Instruments
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pm_companion_timer_test);

#define FIRST_SLEEP_OFFSET_MS	129
#define SECOND_SLEEP_OFFSET_MS	1283
#define THIRD_SLEEP_OFFSET_MS	1177

static bool correct_state_entered_during_sleep = false;
static enum pm_state target_pm_state;
static uint8_t target_pm_substate;

static void pm_state_entry(enum pm_state state)
{
	if (state == target_pm_state) {
		correct_state_entered_during_sleep = true;
		LOG_INF("✓ Entered correct state: %u", state);
	} else {
		LOG_ERR("✗ ERROR: Entered state %u, expected %u", state, target_pm_state);
	}
}

static void pm_state_exit(enum pm_state state)
{
	LOG_INF("✓ Exited state: %u", state);
}

static struct pm_notifier notifier = {
	.state_entry = pm_state_entry,
	.state_exit = pm_state_exit,
};

int main(void)
{
	const struct device *counter_dev =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_system_timer_companion));

	if (!device_is_ready(counter_dev)) {
		LOG_ERR("ERROR: Counter device not ready");
		return -1;
	}

	uint32_t counter_freq = counter_get_frequency(counter_dev);
	LOG_INF("Counter frequency: %u Hz", counter_freq);

	/* Get test power state from device tree */
	const struct pm_state_info target_state =
		PM_STATE_INFO_DT_INIT(DT_CHOSEN(zephyr_test_pm_state));

	target_pm_state = target_state.state;
	target_pm_substate = target_state.substate_id;

	/* Register power state notifier */
	pm_notifier_register(&notifier);

	/* Force the target power state */
	pm_state_force(0, &target_state);
	LOG_INF("Forcing PM state: %u, substate: %u", target_pm_state, target_pm_substate);
	LOG_INF("Min residency: %u us, Exit latency: %u us",
		target_state.min_residency_us, target_state.exit_latency_us);

	uint32_t min_residency_ms = target_state.min_residency_us / 1000;
	uint32_t sleep_durations_ms[] = {
		min_residency_ms + FIRST_SLEEP_OFFSET_MS,
		min_residency_ms + FIRST_SLEEP_OFFSET_MS + SECOND_SLEEP_OFFSET_MS,
		min_residency_ms + FIRST_SLEEP_OFFSET_MS + SECOND_SLEEP_OFFSET_MS + THIRD_SLEEP_OFFSET_MS
	};

	for (int i = 0; i < 3; i++) {
		LOG_INF("--- Sleep cycle %d: %u ms ---", i + 1, sleep_durations_ms[i]);

		correct_state_entered_during_sleep = false;

		uint32_t counter_before = 0;
		counter_get_value(counter_dev, &counter_before);

		k_sleep(K_MSEC(sleep_durations_ms[i]));

		uint32_t counter_after = 0;
		counter_get_value(counter_dev, &counter_after);

		uint32_t counter_delta = counter_after - counter_before;
		uint32_t actual_duration_ms = (counter_delta * 1000) / counter_freq;

		if (correct_state_entered_during_sleep) {
			LOG_INF("✓ Duration: %u ms (expected: %u ms)",
				actual_duration_ms, sleep_durations_ms[i]);
		} else {
			LOG_ERR("✗ Duration: %u ms (expected: %u ms) - WARNING: Did not enter target state",
				actual_duration_ms, sleep_durations_ms[i]);
		}
	}

	pm_notifier_unregister(&notifier);
	LOG_INF("=== Test Complete ===");

	return 0;
}
