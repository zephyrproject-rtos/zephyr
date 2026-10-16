/*
 * Copyright (c) 2026 Texas Instruments
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/atomic.h>

LOG_MODULE_REGISTER(pm_companion_timer_test);

#define FIRST_SLEEP_OFFSET_US  129923
#define SECOND_SLEEP_OFFSET_US 1413080
#define THIRD_SLEEP_OFFSET_US  2590173

const struct device *lpm_timer_dev;
atomic_t correct_state_entered_during_sleep;
enum pm_state target_pm_state;
uint8_t target_pm_substate;

static void pm_state_entry(enum pm_state state)
{
	if (state == target_pm_state) {
		atomic_set(&correct_state_entered_during_sleep, 1);
	}
}

static void pm_state_exit(enum pm_state state)
{
	ARG_UNUSED(state);
}

static struct pm_notifier notifier = {
	.state_entry = pm_state_entry,
	.state_exit = pm_state_exit,
};

void get_lpm_timer_count(uint32_t *count)
{
	if (lpm_timer_dev == NULL) {
		*count = (uint32_t)k_uptime_ticks();
	} else {
		counter_get_value(lpm_timer_dev, count);
	}
}

uint32_t get_lpm_timer_duration(uint32_t *start, uint32_t *end)
{
	if (lpm_timer_dev == NULL) {
		uint32_t tick_delta = *end - *start;

		return (uint64_t)tick_delta * 1000000 / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	} else {
		return counter_ticks_to_us(lpm_timer_dev, (*end - *start));
	}
}

uint32_t get_lpm_timer_frequency(void)
{
	if (lpm_timer_dev == NULL) {
		return CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	} else {
		return counter_get_frequency(lpm_timer_dev);
	}
}

const struct device *get_lpm_timer(void)
{
#if DT_HAS_CHOSEN(zephyr_system_timer_companion)
	return DEVICE_DT_GET(DT_CHOSEN(zephyr_system_timer_companion));
#else
	return NULL;
#endif
}

int main(void)
{
	lpm_timer_dev = get_lpm_timer();

	if (lpm_timer_dev == NULL) {
		LOG_INF("Using Systick timer");
	} else {
		LOG_INF("Using Companion Counter");
		if (!device_is_ready(lpm_timer_dev)) {
			LOG_ERR("ERROR: Counter device not ready");
			return -1;
		}
	}

	uint32_t counter_freq = get_lpm_timer_frequency();

	LOG_INF("Low Power Mode Clock frequency: %u Hz", counter_freq);

	/* Get test power state from device tree */
	const struct pm_state_info target_state =
		PM_STATE_INFO_DT_INIT(DT_CHOSEN(zephyr_test_pm_state));

	target_pm_state = target_state.state;
	target_pm_substate = target_state.substate_id;

	/* Get power state node name */
	const char *pm_state_name = DT_NODE_FULL_NAME(DT_CHOSEN(zephyr_test_pm_state));

	/* Register power state notifier */
	pm_notifier_register(&notifier);

	/* Force the target power state */
	pm_state_force(0, &target_state);

	LOG_INF("Forcing LPM '%s' (mapped to system power state: '%s')", pm_state_name,
		pm_state_to_str(target_pm_state));
	LOG_INF("Min residency: %u us, Exit latency: %u us", target_state.min_residency_us,
		target_state.exit_latency_us);

	uint32_t min_residency_us = target_state.min_residency_us;
	uint32_t sleep_durations_us[] = {min_residency_us + FIRST_SLEEP_OFFSET_US,
					 min_residency_us + SECOND_SLEEP_OFFSET_US,
					 min_residency_us + THIRD_SLEEP_OFFSET_US};

	for (int i = 0; i < 3; i++) {
		LOG_INF("--- Sleep cycle %d: %u us ---", i + 1, sleep_durations_us[i]);
		log_process();

		uint32_t end_time = 0;
		uint32_t start_time = 0;

		atomic_set(&correct_state_entered_during_sleep, 0);

		get_lpm_timer_count(&start_time);

		k_sleep(K_USEC(sleep_durations_us[i]));

		get_lpm_timer_count(&end_time);

		uint32_t actual_duration_us = get_lpm_timer_duration(&start_time, &end_time);

		if (atomic_get(&correct_state_entered_during_sleep)) {
			LOG_INF("✓ Duration: %u us (expected: %u us)", actual_duration_us,
				sleep_durations_us[i]);
		} else {
			LOG_ERR("✗ Duration: %u us (expected: %u us) - "
				"WARNING: Did not enter target state",
				actual_duration_us, sleep_durations_us[i]);
		}
	}

	pm_notifier_unregister(&notifier);
	LOG_INF("=== Test Complete ===");

	return 0;
}
