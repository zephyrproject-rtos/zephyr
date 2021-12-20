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

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

#if DT_NODE_EXISTS(DT_PATH(cpus))

#define CPU_STATES(n) (struct pm_state_info[])PM_STATE_INFO_DT_ITEMS_LIST(n),

static const struct pm_state_info *pm_min_residency[] = {
	DT_FOREACH_CHILD(DT_PATH(cpus), CPU_STATES)
};

#define CPU_STATES_SIZE(n) PM_STATE_DT_ITEMS_LEN(n),

static int pm_min_residency_sizes[] = {
	DT_FOREACH_CHILD(DT_PATH(cpus), CPU_STATES_SIZE)
};
#else
static const struct pm_state_info *pm_min_residency[CONFIG_MP_NUM_CPUS] = {};
static int pm_min_residency_sizes[CONFIG_MP_NUM_CPUS] = {};
#endif	/* DT_NODE_EXISTS(DT_PATH(cpus)) */

struct pm_state_info pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	int i;

	CHECKIF(cpu >= ARRAY_SIZE(pm_min_residency_sizes)) {
		goto error;
	}

	i = pm_min_residency_sizes[cpu];
	const struct pm_state_info *states = (const struct pm_state_info *)
		pm_min_residency[cpu];

	for (i = i - 1; i >= 0; i--) {
		uint32_t min_residency, exit_latency;

		if (!pm_constraint_get(states[i].state)) {
			continue;
		}

		min_residency = k_us_to_ticks_ceil32(
			    states[i].min_residency_us);
		exit_latency = k_us_to_ticks_ceil32(
			    states[i].exit_latency_us);
		__ASSERT(min_residency > exit_latency,
				"min_residency_us < exit_latency_us");

		if ((ticks == K_TICKS_FOREVER) ||
		    (ticks >= (min_residency + exit_latency))) {
			LOG_DBG("Selected power state %d "
				"(ticks: %d, min_residency: %u) to cpu %d",
				states[i].state, ticks,
				states[i].min_residency_us, cpu);
			return states[i];
		}
	}

error:
	LOG_DBG("No suitable power state found for cpu: %d!", cpu);
	return (struct pm_state_info){PM_STATE_ACTIVE, 0, 0};
}
