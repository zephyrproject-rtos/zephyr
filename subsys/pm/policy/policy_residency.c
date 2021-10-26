/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <devicetree.h>
#include <zephyr.h>
#include <kernel.h>
#include <init.h>
#include <pm/pm.h>
#include <pm/policy.h>
#include <sys/check.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(pm, CONFIG_PM_LOG_LEVEL);

#if DT_NODE_EXISTS(DT_PATH(cpus))

#define CPU_STATES(n) (struct pm_state_info[])PM_STATE_INFO_DT_ITEMS_LIST(n),

static const struct pm_state_info *pm_min_residency[] = {
	DT_FOREACH_CHILD(DT_PATH(cpus), CPU_STATES)
};

#define CPU_STATES_SIZE(n) PM_STATE_DT_ITEMS_LEN(n),

static int pm_min_residency_sizes[] = {
	DT_FOREACH_CHILD(DT_PATH(cpus), CPU_STATES_SIZE)
};

#define THRESHOLD_ITEMS_LISTIFY_FUNC(child, node_id) \
	{ 0 },

#define THRESHOLD_ITEMS_LIST(node_id) {         \
	UTIL_LISTIFY(PM_STATE_DT_ITEMS_LEN(node_id),   \
		     THRESHOLD_ITEMS_LISTIFY_FUNC,\
		     node_id)                          \
	}

#define RESIDENCY_THRESHOLD_INIT(n) (uint32_t[])THRESHOLD_ITEMS_LIST(n),

static uint32_t *pm_min_residency_thresholds[] = {
	DT_FOREACH_CHILD(DT_PATH(cpus), RESIDENCY_THRESHOLD_INIT)
};
#else
static const struct pm_state_info *pm_min_residency[CONFIG_MP_NUM_CPUS] = {};
static int pm_min_residency_sizes[CONFIG_MP_NUM_CPUS] = {};
static uint32_t *pm_min_residency_thresholds[CONFIG_MP_NUM_CPUS] = {};
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
		if (!pm_constraint_get(states[i].state)) {
			continue;
		}
		if ((ticks == K_TICKS_FOREVER) ||
		    (ticks >= pm_min_residency_thresholds[cpu][i])) {
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

/*
 * Precompute threshold values used later
 * to make decision about state transition.
 */
static int precompute_residency(const struct device *dev)
{
	ARG_UNUSED(dev);

	int cpu;

	for (cpu = 0; cpu < ARRAY_SIZE(pm_min_residency_sizes); cpu++) {
		const struct pm_state_info *states = (const struct pm_state_info *)
			pm_min_residency[cpu];
		int i = pm_min_residency_sizes[cpu];

		for (i = i - 1; i >= 0; i--) {
			uint32_t min_residency, exit_latency;

			min_residency = k_us_to_ticks_ceil32(states[i].min_residency_us);
			exit_latency = k_us_to_ticks_ceil32(states[i].exit_latency_us);
			__ASSERT(min_residency > exit_latency,
				"min_residency_us < exit_latency_us");
			pm_min_residency_thresholds[cpu][i] =
					min_residency + exit_latency;
		}
	}

	return 0;
}

SYS_INIT(precompute_residency, PRE_KERNEL_1, 0);

