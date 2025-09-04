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

#define DEFINE_DISABLED_PM_STATE(node) \
	IF_ENABLED(DT_NODE_HAS_STATUS(node, disabled), (PM_STATE_INFO_DT_INIT(node),))

/** Check if power states exists in the Devicetree. */
#define POWER_STATES_EXISTS()						       \
	UTIL_OR(DT_NODE_EXISTS(DT_PATH(cpus, power_states)),		       \
		DT_NODE_EXISTS(DT_PATH(power_states)))

/** Get node with power states. Macro assumes that power states exists. */
#define POWER_STATES_NODE()						       \
	COND_CODE_1(DT_NODE_EXISTS(DT_PATH(cpus, power_states)),	       \
		    (DT_PATH(cpus, power_states)), (DT_PATH(power_states)))

/* Array with all states which are disabled but can be forced. */
static const struct pm_state_info disabled_states[] = {
	IF_ENABLED(POWER_STATES_EXISTS(),
		   (DT_FOREACH_CHILD(POWER_STATES_NODE(), DEFINE_DISABLED_PM_STATE)))
};

uint8_t pm_state_cpu_get_all(uint8_t cpu, const struct pm_state_info **states)
{
	if (cpu >= ARRAY_SIZE(cpus_states)) {
		return 0;
	}

	*states = cpus_states[cpu];

	return states_per_cpu[cpu];
}

const struct pm_state_info *pm_state_get(uint8_t cpu, enum pm_state state, uint8_t substate_id)
{
	__ASSERT_NO_MSG(cpu < ARRAY_SIZE(cpus_states));
	const struct pm_state_info *states = cpus_states[cpu];
	uint8_t cnt = states_per_cpu[cpu];

	for (uint8_t i = 0; i < cnt; i++) {
		if ((states[i].state == state) && (states[i].substate_id == substate_id)) {
			return &states[i];
		}
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(disabled_states); i++) {
		if ((disabled_states[i].state == state) &&
		    (disabled_states[i].substate_id == substate_id)) {
			return &disabled_states[i];
		}
	}

	return NULL;
}

const char *pm_state_to_str(enum pm_state state)
{
	switch (state) {
	case PM_STATE_ACTIVE:
		return "active";
	case PM_STATE_RUNTIME_IDLE:
		return "runtime-idle";
	case PM_STATE_SUSPEND_TO_IDLE:
		return "suspend-to-idle";
	case PM_STATE_STANDBY:
		return "standby";
	case PM_STATE_SUSPEND_TO_RAM:
		return "suspend-to-ram";
	case PM_STATE_SUSPEND_TO_DISK:
		return "suspend-to-disk";
	case PM_STATE_SOFT_OFF:
		return "soft-off";
	default:
		return "UNKNOWN";
	}
}

int pm_state_from_str(const char *name, enum pm_state *out)
{
	if (strcmp(name, "active") == 0) {
		*out = PM_STATE_ACTIVE;
	} else if (strcmp(name, "runtime-idle") == 0) {
		*out = PM_STATE_RUNTIME_IDLE;
	} else if (strcmp(name, "suspend-to-idle") == 0) {
		*out = PM_STATE_SUSPEND_TO_IDLE;
	} else if (strcmp(name, "standby") == 0) {
		*out = PM_STATE_STANDBY;
	} else if (strcmp(name, "suspend-to-ram") == 0) {
		*out = PM_STATE_SUSPEND_TO_RAM;
	} else if (strcmp(name, "suspend-to-disk") == 0) {
		*out = PM_STATE_SUSPEND_TO_DISK;
	} else if (strcmp(name, "soft-off") == 0) {
		*out = PM_STATE_SOFT_OFF;
	} else {
		return -EINVAL;
	}

	return 0;
}
