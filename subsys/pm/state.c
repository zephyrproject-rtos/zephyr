/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/state.h>
#include <zephyr/pm/policy.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/arch_inlines.h>

BUILD_ASSERT(DT_NODE_EXISTS(DT_PATH(cpus)),
	     "cpus node not defined in Devicetree");

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
		"Found CPU power state with min_residency < exit_latency")

/**
 * @brief Check CPU power states consistency
 *
 * All states should have a minimum residency >= than the exit latency.
 *
 * @param node_id A CPU node identifier.
 */
#define CHECK_POWER_STATES_CONSISTENCY(node_id)				       \
	LISTIFY(DT_PROP_LEN_OR(node_id, cpu_power_states, 0),		       \
		CHECK_POWER_STATE_CONSISTENCY, (;), node_id);		       \

/* Check that all power states are consistent */
DT_FOREACH_CHILD(DT_PATH(cpus), CHECK_POWER_STATES_CONSISTENCY)

#define DEFINE_CPU_STATES(n) \
	static const struct pm_state_info pmstates_##n[] \
		= PM_STATE_INFO_LIST_FROM_DT_CPU(n);
#define CPU_STATE_REF(n) pmstates_##n

DT_FOREACH_CHILD(DT_PATH(cpus), DEFINE_CPU_STATES);

/** CPU power states information for each CPU */
static const struct pm_state_info *cpus_states[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(cpus), CPU_STATE_REF, (,))
};

/** Number of states for each CPU */
static const uint8_t states_per_cpu[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(cpus), DT_NUM_CPU_POWER_STATES, (,))
};

uint8_t pm_state_cpu_get_all(uint8_t cpu, const struct pm_state_info **states)
{
	if (cpu >= ARRAY_SIZE(cpus_states)) {
		return 0;
	}

	*states = cpus_states[cpu];

	return states_per_cpu[cpu];
}

void pm_state_custom_data_set(enum pm_state state, uint8_t substate_id,
			      const void *data)
{
#ifdef CONFIG_PM_STATE_CUSTOM_DATA
	for (int16_t i = 0; i < ARRAY_SIZE(cpus_states); i++) {
		struct pm_state_info *state_info = cpus_states[i];

		if ((state_info->state == state) &&
		    ((state_info->substate_id == substate_id)) ||
		    (substate_id == PM_ALL_SUBSTATES)) {
			state_info->custom_data = data;
		}
	}
#endif
}

void *pm_state_custom_data_get(enum pm_state state, uint8_t substate_id)
{
#ifdef CONFIG_PM_STATE_CUSTOM_DATA
	unsigned int num_cpu_states;
	const struct pm_state_info *cpu_states;

	num_cpu_states = pm_state_cpu_get_all(arch_proc_id(), &cpu_states);

	for (int i = num_cpu_states - 1; i >= 0; i--) {
		const struct pm_state_info *state_info = &cpu_states[i];

		if ((state_info->state == state) &&
		    (state_info->substate_id == substate_id)) {
			return state_info->custom_data;
		}
	}
#endif

	return NULL;
}
