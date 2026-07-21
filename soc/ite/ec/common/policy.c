/*
 * Copyright (c) 2022 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>
#include <soc.h>

__weak const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	const struct pm_state_info *cpu_states;
	uint8_t num_cpu_states;

	num_cpu_states = pm_state_cpu_get_all(cpu, &cpu_states);

	for (int16_t i = (int16_t)num_cpu_states - 1; i >= 0; i--) {
		const struct pm_state_info *state = &cpu_states[i];
		uint32_t min_residency;

		/* check if there is a lock on state + substate */
		if (pm_policy_state_lock_is_active(
			state->state, state->substate_id)) {
			continue;
		}

		min_residency = k_us_to_ticks_ceil32(state->min_residency_us);
		/*
		 * The tick interval for the system to enter sleep mode needs
		 * to be longer than or equal to the minimum residency.
		 */
		if (ticks >= min_residency) {
			return state;
		}
	}

	return NULL;
}
