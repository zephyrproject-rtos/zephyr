/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/policy.h>
#include <zephyr/sys_clock.h>
#include <zephyr/pm/device.h>

const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	int64_t cyc = -1;
	uint8_t num_cpu_states;
	const struct pm_state_info *cpu_states;
	const struct pm_state_info *out_state = NULL;

#ifdef CONFIG_PM_NEED_ALL_DEVICES_IDLE
	if (pm_device_is_any_busy()) {
		return NULL;
	}
#endif

	if (ticks != K_TICKS_FOREVER) {
		cyc = k_ticks_to_cyc_ceil32(ticks);
	}

	num_cpu_states = pm_state_cpu_get_all(cpu, &cpu_states);

	for (uint32_t i = 0; i < num_cpu_states; i++) {
		const struct pm_state_info *state = &cpu_states[i];
		uint32_t min_residency_ticks;

		min_residency_ticks =
			k_us_to_ticks_ceil32(state->min_residency_us + state->exit_latency_us);

		if (ticks < min_residency_ticks) {
			/* If current state has higher residency then use the previous state; */
			break;
		}

		/* check if state is available. */
		if (!pm_policy_state_is_available(state->state, state->substate_id)) {
			continue;
		}

		out_state = state;
	}

	return out_state;
}
