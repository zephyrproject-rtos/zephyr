/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/power/energy_model.h>
#include <zephyr/sys/util.h>

#define POWER_ENERGY_IDLE_ACTIVE_POWER 64U
#define POWER_ENERGY_IDLE_RUNTIME_POWER 32U
#define POWER_ENERGY_IDLE_SUSPEND_POWER 8U
#define POWER_ENERGY_IDLE_STANDBY_POWER 4U
#define POWER_ENERGY_IDLE_DEEP_POWER 1U

#define POWER_ENERGY_TRANSITION_POWER POWER_ENERGY_IDLE_ACTIVE_POWER

static uint32_t active_power_scale(const struct pstate *state, uint32_t min_frequency_hz)
{
	uint64_t scale;

	if ((state->frequency_hz == 0U) || (min_frequency_hz == 0U)) {
		return 0U;
	}

	scale = DIV_ROUND_UP((uint64_t)state->frequency_hz, (uint64_t)min_frequency_hz);
	scale = MAX(scale, 1ULL);

	return (uint32_t)MIN(scale, (uint64_t)UINT32_MAX);
}

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
	uint32_t active_scale;

	if ((input == NULL) || (result == NULL) || (input->to_pstate == NULL) ||
	    (input->min_frequency_hz == 0U)) {
		return -EINVAL;
	}

	ARG_UNUSED(input->from_pstate);
	ARG_UNUSED(input->estimated_cycles);

	active_scale = active_power_scale(input->to_pstate, input->min_frequency_hz);
	if (active_scale == 0U) {
		return -EINVAL;
	}

	result->active_energy = input->exec_time_us * active_scale;
	result->transition_energy = input->transition_time_us * POWER_ENERGY_TRANSITION_POWER;
	result->idle_energy = input->idle_time_us * idle_power_scale(input->idle_state);
	result->total_energy = result->active_energy + result->transition_energy +
		result->idle_energy;

	return 0;
}
