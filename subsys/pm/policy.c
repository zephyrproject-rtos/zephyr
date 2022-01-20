/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pm/pm.h>
#include <pm/policy.h>
#include <sys_clock.h>
#include <sys/time_units.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(pm, CONFIG_PM_LOG_LEVEL);

const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	uint8_t num_cpu_states;
	const struct pm_state_info *cpu_states;

	num_cpu_states = pm_state_cpu_get_all(cpu, &cpu_states);

	for (int16_t i = (int16_t)num_cpu_states - 1; i >= 0; i--) {
		const struct pm_state_info *state = &cpu_states[i];
		uint32_t min_residency, exit_latency;

		if (!pm_constraint_get(state->state)) {
			continue;
		}

		min_residency = k_us_to_ticks_ceil32(state->min_residency_us);
		exit_latency = k_us_to_ticks_ceil32(state->exit_latency_us);

		if ((ticks == K_TICKS_FOREVER) ||
		    (ticks >= (min_residency + exit_latency))) {
			return state;
		}
	}

	return NULL;
}
