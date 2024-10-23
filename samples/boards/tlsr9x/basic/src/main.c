/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/state.h>

#define SLEEP_TIME_MS   1000

static const char *pm_state_to_string(enum pm_state state)
{
	static const char * const pm_state_string[] = {
		"ACTIVE",
		"RUNTIME IDLE",
		"SUSPEND TO IDLE",
		"STANDBY",
		"SUSPEND TO RAM ",
		"SUSPEND TO DISK",
		"SOFT OFF"
	};
	const char *result = "N/A";

	if (state < PM_STATE_COUNT) {
		result = pm_state_string[state];
	}
	return result;
}

int main(void)
{
	const struct pm_state_info *pm_info;
	uint8_t num_states = pm_state_cpu_get_all(0, &pm_info);

	printk("*** list of supported PM states\n");
	for (uint8_t i = 0; i < num_states; i++) {
		printk("[%u] '%s' substate: %u residency: %uuS latency %uuS\n", i,
			pm_state_to_string(pm_info[i].state),
			pm_info[i].substate_id,
			pm_info[i].min_residency_us,
			pm_info[i].exit_latency_us);
	}
	printk("*** end of list\n");

	uint32_t cnt = 0;

	while (1) {
		uint64_t active_time_ms = k_uptime_get();

		printk("cnt: %u, mtimer: %llu\n", cnt++, sys_clock_cycle_get_64());
		k_sleep(K_TIMEOUT_ABS_MS(active_time_ms + SLEEP_TIME_MS));
	}
	return 0;
}
