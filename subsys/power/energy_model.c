/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/power/energy_model.h>
#include <zephyr/sys/util.h>

#define POWER_ENERGY_IDLE_ACTIVE_POWER	64U
#define POWER_ENERGY_IDLE_RUNTIME_POWER	32U
#define POWER_ENERGY_IDLE_SUSPEND_POWER	8U
#define POWER_ENERGY_IDLE_STANDBY_POWER	4U
#define POWER_ENERGY_IDLE_DEEP_POWER	1U

#define POWER_ENERGY_TRANSITION_POWER	POWER_ENERGY_IDLE_ACTIVE_POWER

/**
 * Convert the target frequency to a relative scale compared to the minimum frequency.
 *
 * For example, the minimum frequency is 50MHz,
 *
 * target = 50 MHz  -> scale = 1
 * target = 100 MHz -> scale = 2
 * target = 150 MHz -> scale = 3
 *
 * This means that the higher the frequency, the more "expensive" each unit of time is;
 * the longer it runs, the higher the total score.
 */
static uint32_t active_power_scale(const struct pstate *state, uint32_t min_frequency_hz)
{
	if ((state->frequency_hz == 0U) || (min_frequency_hz == 0U)) {
		return 0U;
	}

	uint64_t scale;

	scale = DIV_ROUND_UP((uint64_t)state->frequency_hz, (uint64_t)min_frequency_hz);
	scale = MAX(scale, 1ULL);

	return (uint32_t)MIN(scale, (uint64_t)UINT32_MAX);
}

/**
 * Assign a fixed multiplier based on the predicted idle state.
 *
 * ACTIVE          -> 64
 * RUNTIME_IDLE    -> 32
 * SUSPEND_TO_IDLE -> 8
 * STANDBY         -> 4
 *
 * This means that the deeper the idle state, the less "expensive" each unit of idle time is;
 * the longer it idles, the lower the total score.
 */
static uint32_t idle_power_scale(const struct pm_state_info *idle_state)
{
	if (idle_state == NULL) {
		return POWER_ENERGY_IDLE_ACTIVE_POWER;
	}

	switch (idle_state->state) {
	case PM_STATE_RUNTIME_IDLE:
		return POWER_ENERGY_IDLE_RUNTIME_POWER;
	case PM_STATE_SUSPEND_TO_IDLE:
		return POWER_ENERGY_IDLE_SUSPEND_POWER;
	case PM_STATE_STANDBY:
		return POWER_ENERGY_IDLE_STANDBY_POWER;
	case PM_STATE_SUSPEND_TO_RAM:
	case PM_STATE_SUSPEND_TO_DISK:
	case PM_STATE_SOFT_OFF:
		return POWER_ENERGY_IDLE_DEEP_POWER;
	case PM_STATE_ACTIVE:
	case PM_STATE_COUNT:
	default:
		return POWER_ENERGY_IDLE_ACTIVE_POWER;
	}
}

int power_energy_eval_pstate(const struct power_energy_eval_input *input,
			     struct power_energy_eval_result *result)
{
	ARG_UNUSED(input->from_pstate);
	ARG_UNUSED(input->estimated_cycles);

	if ((input == NULL) || (result == NULL) || (input->to_pstate == NULL) ||
	    (input->min_frequency_hz == 0U)) {
		return -EINVAL;
	}

	uint32_t active_scale;

	active_scale = active_power_scale(input->to_pstate, input->min_frequency_hz);
	if (active_scale == 0U) {
		return -EINVAL;
	}

	/**
	 * 1. active_energy = execution time * relative scale of target frequency;
	 *
	 * 2. transition_energy = transition time * active power scale;
	 *    Simplified as transition power is often close to active power,
	 *    This penalizes P-states with large switching latencies, avoiding
	 *    frequent frequency switching for very small workloads.
	 *
	 * 3. idle_energy = predicted idle time * idle state power scale;
	 *
	 */
	result->active_energy = input->exec_time_us * active_scale;
	result->transition_energy = input->transition_time_us * POWER_ENERGY_TRANSITION_POWER;
	result->idle_energy = input->idle_time_us * idle_power_scale(input->idle_state);
	result->total_energy = result->active_energy + result->transition_energy +
			       result->idle_energy;

	return 0;
}
