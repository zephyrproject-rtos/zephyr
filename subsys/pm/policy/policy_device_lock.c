/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2022 Nordic Semiconductor ASA
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
 * @brief initialize a device pm constraint with information from devicetree.
 *
 * @param node_id Node identifier.
 */
#define PM_STATE_CONSTRAINT_INIT(node_id)                                     \
	{                                                                     \
		.state = PM_STATE_DT_INIT(node_id),                           \
		.substate_id = DT_PROP_OR(node_id, substate_id, 0),           \
	}

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

static struct pm_state_device_constraint _devices_constraints[] = {
	DT_FOREACH_STATUS_OKAY_NODE(PM_STATE_DEVICE_CONSTRAINT_DEFINE)
};

void pm_policy_device_power_lock_get(const struct device *dev)
{
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)
	for (size_t i = 0; i < ARRAY_SIZE(_devices_constraints); i++) {
		const struct device *device = device_get_binding(_devices_constraints[i].dev);

		if ((device != NULL) && (device == dev)) {
			for (size_t j = 0; j < _devices_constraints[i].pm_constraints_size; j++) {
				pm_policy_state_lock_get(
						_devices_constraints[i].constraints[j].state,
						_devices_constraints[i].constraints[j].substate_id);
			}
			break;
		}
	}
#endif
}

void pm_policy_device_power_lock_put(const struct device *dev)
{
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)
	for (size_t i = 0; i < ARRAY_SIZE(_devices_constraints); i++) {
		const struct device *device = device_get_binding(_devices_constraints[i].dev);

		if ((device != NULL) && (device == dev)) {
			for (size_t j = 0; j < _devices_constraints[i].pm_constraints_size; j++) {
				pm_policy_state_lock_put(
						_devices_constraints[i].constraints[j].state,
						_devices_constraints[i].constraints[j].substate_id);
			}
			break;
		}
	}
#endif
}
