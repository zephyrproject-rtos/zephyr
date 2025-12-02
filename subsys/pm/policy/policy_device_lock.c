/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/policy.h>
#include <zephyr/pm/state.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/pm/device.h>

struct pm_state_device_constraint {
	const char *const dev;
	size_t pm_constraints_size;
	struct pm_state_constraint *constraints;
};

/**
 * @brief Synthesize the name of the object that holds a device pm constraint.
 *
 * @param dev_id Device identifier.
 */
#define PM_CONSTRAINTS_NAME(node_id) _CONCAT(__devicepmconstraints_, node_id)


/**
 * @brief Helper macro to define a device pm constraints.
 */
#define PM_STATE_CONSTRAINT_DEFINE(i, node_id)                                  \
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_PHANDLE_BY_IDX(node_id,          \
		zephyr_disabling_power_states, i)),                             \
		(PM_STATE_CONSTRAINT_INIT(DT_PHANDLE_BY_IDX(node_id,            \
		zephyr_disabling_power_states, i)),), ())

/**
 * @brief Helper macro to generate a list of device pm constraints.
 */
#define PM_STATE_CONSTRAINTS_DEFINE(node_id)                                           \
	{                                                                              \
		LISTIFY(DT_PROP_LEN_OR(node_id, zephyr_disabling_power_states, 0),     \
			PM_STATE_CONSTRAINT_DEFINE, (), node_id)                       \
	}

/**
 * @brief Helper macro to define an array of device pm constraints.
 */
#define CONSTRAINTS_DEFINE(node_id)                         \
	Z_DECL_ALIGN(struct pm_state_constraint)            \
		PM_CONSTRAINTS_NAME(node_id)[] =            \
		PM_STATE_CONSTRAINTS_DEFINE(node_id);

#define DEVICE_CONSTRAINTS_DEFINE(node_id)                                           \
	COND_CODE_0(DT_NODE_HAS_PROP(node_id, zephyr_disabling_power_states), (),    \
		(CONSTRAINTS_DEFINE(node_id)))

DT_FOREACH_STATUS_OKAY_NODE(DEVICE_CONSTRAINTS_DEFINE)

/**
 * @brief Helper macro to initialize a pm state device constraint
 */
#define PM_STATE_DEVICE_CONSTRAINT_INIT(node_id)                                              \
	{                                                                                     \
		.dev = DEVICE_DT_NAME(node_id),                                                \
		.pm_constraints_size = DT_PROP_LEN(node_id, zephyr_disabling_power_states),   \
		.constraints = PM_CONSTRAINTS_NAME(node_id),                                  \
	},

/**
 * @brief Helper macro to initialize a pm state device constraint
 */
#define PM_STATE_DEVICE_CONSTRAINT_DEFINE(node_id)                                      \
	COND_CODE_0(DT_NODE_HAS_PROP(node_id, zephyr_disabling_power_states), (),       \
		(PM_STATE_DEVICE_CONSTRAINT_INIT(node_id)))

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)
static struct pm_state_device_constraint _devices_constraints[] = {
	DT_FOREACH_STATUS_OKAY_NODE(PM_STATE_DEVICE_CONSTRAINT_DEFINE)
};

/* returns device's constraints in _devices_constraints, NULL if not found */
static struct pm_state_device_constraint *
pm_policy_priv_device_find_device_constraints(const struct device *dev)
{
	if (dev == NULL) {
		return NULL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(_devices_constraints); i++) {
		const struct device *device = device_get_binding(_devices_constraints[i].dev);

		if (device == dev) {
			return &_devices_constraints[i];
		}
	}

	return NULL;
}
#endif

void pm_policy_device_power_lock_get(const struct device *dev)
{
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)
	struct pm_state_device_constraint *constraints =
				pm_policy_priv_device_find_device_constraints(dev);
	if (constraints == NULL) {
		return;
	}

	for (size_t j = 0; j < constraints->pm_constraints_size; j++) {
		pm_policy_state_lock_get(constraints->constraints[j].state,
					 constraints->constraints[j].substate_id);
	}
#endif
}

void pm_policy_device_power_lock_put(const struct device *dev)
{
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)
	struct pm_state_device_constraint *constraints =
				pm_policy_priv_device_find_device_constraints(dev);
	if (constraints == NULL) {
		return;
	}

	for (size_t j = 0; j < constraints->pm_constraints_size; j++) {
		pm_policy_state_lock_put(constraints->constraints[j].state,
					 constraints->constraints[j].substate_id);
	}
#endif
}

bool pm_policy_device_is_disabling_state(const struct device *dev,
					 enum pm_state state, uint8_t substate_id)
{
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)
	struct pm_state_device_constraint *constraints =
				pm_policy_priv_device_find_device_constraints(dev);
	if (constraints == NULL) {
		return false;
	}

	for (size_t j = 0; j < constraints->pm_constraints_size; j++) {
		if (constraints->constraints[j].state == state &&
		    constraints->constraints[j].substate_id == substate_id) {
			return true;
		}
	}
#endif
	return false;
}
