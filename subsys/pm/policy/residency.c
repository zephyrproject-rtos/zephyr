/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <devicetree.h>
#include <pm/pm.h>
#include <pm/policy.h>
#include <sys_clock.h>
#include <sys/check.h>
#include <sys/time_units.h>
#include <sys/util.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(pm, CONFIG_PM_LOG_LEVEL);

/**
 * Check CPU power state consistency.
 *
 * @param i Power state index.
 * @param node_id CPU node identifier.
 */
#define CHECK_POWER_STATE_CONSISTENCY(i, node_id)			       \
	BUILD_ASSERT(							       \
		DT_PROP_BY_PHANDLE_IDX_OR(node_id, cpu_power_states, i,	       \
					  min_residency_us, 0U) >=	       \
		DT_PROP_BY_PHANDLE_IDX_OR(node_id, cpu_power_states, i,	       \
					  exit_latency_us, 0U),		       \
		"Found CPU power state with min_residency < exit_latency");

/**
 * @brief Check CPU power states consistency
 *
 * All states should have a minimum residency >= than the exit latency.
 *
 * @param node_id A CPU node identifier.
 */
#define CHECK_POWER_STATES_CONSISTENCY(node_id)				       \
	UTIL_LISTIFY(DT_NUM_CPU_POWER_STATES(node_id),			       \
		     CHECK_POWER_STATE_CONSISTENCY, node_id)		       \

/* Check that all power states are consistent */
COND_CODE_1(DT_NODE_EXISTS(DT_PATH(cpus)),
	    (DT_FOREACH_CHILD(DT_PATH(cpus), CHECK_POWER_STATES_CONSISTENCY)),
	    ())

#define NUM_CPU_STATES(n) DT_NUM_CPU_POWER_STATES(n),
#define CPU_STATES(n) (struct pm_state_info[])PM_STATE_INFO_LIST_FROM_DT_CPU(n),

/** CPU power states information for each CPU */
static const struct pm_state_info *cpus_states[] = {
	COND_CODE_1(DT_NODE_EXISTS(DT_PATH(cpus)),
		    (DT_FOREACH_CHILD(DT_PATH(cpus), CPU_STATES)),
		    ())
};

/** Number of states for each CPU */
static const uint8_t states_per_cpu[] = {
	COND_CODE_1(DT_NODE_EXISTS(DT_PATH(cpus)),
		    (DT_FOREACH_CHILD(DT_PATH(cpus), NUM_CPU_STATES)),
		    ())
};

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
