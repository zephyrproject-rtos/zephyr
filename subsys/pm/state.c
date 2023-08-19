/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/state.h>
#include <zephyr/toolchain.h>

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
	static const struct pm_state_info pmstates_cpu##n[] \
		= PM_STATE_INFO_LIST_FROM_DT_CPU(n);
#define CPU_STATE_REF(n) pmstates_cpu##n

DT_FOREACH_CHILD(DT_PATH(cpus), DEFINE_CPU_STATES);

#if CONFIG_MP_NUM_CPUS > 1 && DT_NODE_EXISTS(DT_PATH(cpus, power_states))
/** General power-states list on SMP platforms, as individual per-CPU states may vary */
static const struct pm_state_info pmstates_all[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(cpus, power_states),
					 PM_STATE_INFO_DT_INIT, (,)),
};
#endif

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

inline uint8_t pm_state_cpu_get_index(uint8_t cpu, enum pm_state state, uint8_t substate_id)
{
	for (uint8_t i = 0; i < states_per_cpu[cpu]; i++) {
		if (cpus_states[cpu][i].state == state &&
		    cpus_states[cpu][i].substate_id == substate_id) {
			return i;
		}
	}

	return -1;
}

#if CONFIG_MP_NUM_CPUS > 1
inline uint8_t pm_state_get_index(enum pm_state state, uint8_t substate_id)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(pmstates_all); i++) {
		if (pmstates_all[i].state == state &&
		    pmstates_all[i].substate_id == substate_id) {
			return i;
		}
	}

	return -1;
}
#else
inline uint8_t pm_state_get_index(enum pm_state state, uint8_t substate_id)
{
	return pm_state_cpu_get_index(0U, state, substate_id);
}
#endif
