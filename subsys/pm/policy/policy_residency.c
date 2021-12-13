/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <devicetree.h>
#include <zephyr.h>
#include <kernel.h>
#include <pm/pm.h>
#include <pm/policy.h>
#include <sys/check.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(pm, CONFIG_PM_LOG_LEVEL);

#if DT_NODE_EXISTS(DT_PATH(cpus))

#define NUM_CPU_STATES(n) DT_NUM_CPU_POWER_STATES(n),
#define CPU_STATES(n) (struct pm_state_info[])PM_STATE_INFO_LIST_FROM_DT_CPU(n),

static const struct pm_state_info *cpus_states[] = {
	DT_FOREACH_CHILD(DT_PATH(cpus), CPU_STATES)
};

static const uint8_t states_per_cpu[] = {
	DT_FOREACH_CHILD(DT_PATH(cpus), NUM_CPU_STATES)
};
#else
static const struct pm_state_info *cpus_states[CONFIG_MP_NUM_CPUS] = {};
static int states_per_cpu[CONFIG_MP_NUM_CPUS] = {};
#endif	/* DT_NODE_EXISTS(DT_PATH(cpus)) */

struct pm_state_info pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	CHECKIF(cpu >= ARRAY_SIZE(states_per_cpu)) {
		goto error;
	}

	for (int16_t i = (int16_t)states_per_cpu[cpu] - 1; i >= 0; i--) {
		const struct pm_state_info *state = (&cpus_states[cpu])[i];
		uint32_t min_residency, exit_latency;

		if (!pm_constraint_get(state->state)) {
			continue;
		}

		min_residency = k_us_to_ticks_ceil32(state->min_residency_us);
		exit_latency = k_us_to_ticks_ceil32(state->exit_latency_us);
		__ASSERT(min_residency > exit_latency,
			 "min_residency_us < exit_latency_us");

		if ((ticks == K_TICKS_FOREVER) ||
		    (ticks >= (min_residency + exit_latency))) {
			LOG_DBG("Selected power state %d "
				"(ticks: %d, min_residency: %u) to cpu %d",
				state->state, ticks, state->min_residency_us,
				cpu);
			return *state;
		}
	}

error:
	LOG_DBG("No suitable power state found for cpu: %d!", cpu);
	return (struct pm_state_info){PM_STATE_ACTIVE, 0, 0};
}
