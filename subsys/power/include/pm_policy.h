/*
 * Copyright (c) 2020 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PM_POLICY_H_
#define _PM_POLICY_H_

#include <string.h>
#include <sys/util.h>
#include <power_state.h>
#include <toolchain/common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Power management policy
 * @defgroup Pm_policy Power management policy
 * @{
 * @}
 */

struct pm_policy;

/**
 * @brief Power management policy API.
 */
struct pm_policy_api {
	/* initialize the power management policy */
	void (*init)(void);

	/* get next system power state based on current system
	 * status and the power management policy where this
	 * api resides.
	 */
	pm_state_t (*next_state)(struct pm_policy *policy, int ticks);

	/* clear the force state set by set_force_state */
	int (*clear_force_state)(struct pm_policy *policy);

	/* set the state forcing used by the policy */
	int (*set_force_state)(struct pm_policy *policy, pm_state_t state);

	/* set constraints for the supported power states so
	 * that others can impact the pm_policy when make decision.
	 */
	int (*set_constraint)(struct pm_policy *policy, pm_state_t states);

	/* release the constraints set by set_constraint for
	 * the supported power states.
	 * should be paired with set_constraint.
	 */
	int (*release_constraint)(struct pm_policy *policy, pm_state_t states);
};

/**
 * @brief Power management policy structure.
 */
struct pm_policy {
	const char *name;
	pm_state_t supported_states;
	struct pm_policy_api *policy_api;
};

/**
 * @brief Create pm_policy instance.
 *
 * @param _supported_states Supported states.
 * @param _name Instance name.
 * @param _api Power management policy API.
 */
#define PM_POLICY_DEFINE(_supported_states, _name, _api) \
	static const Z_STRUCT_SECTION_ITERABLE(pm_policy, _name) = \
	{ \
		.name = STRINGIFY(_name), \
		.supported_states = (_supported_states & PM_STATE_BIT_MASK), \
		.policy_api = &_api \
	}

/**
 * @brief Initialize power management policy.
 *
 * @param policy Pointer to pm_policy instance.
 */
static inline void pm_policy_init(struct pm_policy *policy)
{
	if (policy && policy->policy_api) {
		policy->policy_api->init();
	}
}

/**
 * @brief Generate system power state based on the given policy.
 *
 * @param policy Pointer to pm_policy instance.
 * @param ticks The number of ticks system going to idle.
 *
 * @return Power state of system decided by the policy or
 * 	   PM_STATE_RUNTIME_IDLE if not completed structure.
 */
static inline pm_state_t pm_policy_next_state(struct pm_policy *policy,
					      int ticks)
{
	if (policy && policy->policy_api) {
		return policy->policy_api->next_state(policy, ticks);
	}

	return PM_STATE_RUNTIME_IDLE;
}

/**
 * @brief Clear the force state for the given policy.
 *
 * @param policy Pointer to pm_policy instance.
 *
 * @return 0 if successful, -1 if failure.
 */
static inline int pm_policy_clear_force_state(struct pm_policy *policy)
{
	if (policy && policy->policy_api) {
		return policy->policy_api->clear_force_state(policy);
	}

	return -1;
}

/**
 * @brief Set the state forcing used by the given policy.
 *
 * @param policy Pointer to pm_policy instance.
 * @param state  Power state forcing used by the given policy.
 *
 * @return 0 if successful, -1 if failure.
 */
static inline int pm_policy_set_force_state(struct pm_policy *policy,
					    pm_state_t state)
{
	if (policy && policy->policy_api) {
		return policy->policy_api->set_force_state(policy,
					state & PM_STATE_BIT_MASK);
	}

	return -1;
}

/**
 * @brief Get supported power states based on the given policy.
 *
 * @param policy Pointer to pm_policy instance.
 *
 * @return Supported power states of the given policy or if not completed
 * 	   structure return PM_STATE_RUNTIME_IDLE.
 */
static inline pm_state_t pm_policy_get_supported_state(struct pm_policy *policy)
{
	if (policy) {
		return policy->supported_states & PM_STATE_BIT_MASK;
	}

	return PM_STATE_RUNTIME_IDLE;
}

/**
 * @brief Set constraint for the power states of the given policy.
 *
 * @param policy Pointer to pm_policy instance.
 * @param states A bit set identifying PM states that the power management
 *               policy is not permitted to select.
 *
 * @return 0 if successful, -1 if failure.
 */
static inline int pm_policy_set_constraint(struct pm_policy *policy,
					   pm_state_t states)
{
	if (policy && policy->policy_api) {
		return policy->policy_api->set_constraint(policy,
					states & PM_STATE_BIT_MASK);
	}

	return -1;
}

/**
 * @brief Release constraint for the power states of the given policy.
 *
 * @param policy Pointer to pm_policy instance.
 * @param states A bit set identifying PM states for which a previous
 *               constraint should be cleared, but only allowing the
 *               state to be selected by the policy when all previous
 *               constraints are cleared.
 *
 * @return 0 if success, -1 if failure.
 */
static inline int pm_policy_release_constraint(struct pm_policy *policy,
					       pm_state_t states)
{
	if (policy && policy->policy_api) {
		return policy->policy_api->release_constraint(policy,
						states & PM_STATE_BIT_MASK);
	}

	return -1;
}

/**
 * @brief Get power management policy based on the name
 *        of pm_policy in power management policy section.
 *
 * @param name Name of wanted pm_policy.
 *
 * @return Pointer of the wanted pm_policy or NULL.
 */
static inline struct pm_policy *pm_policy_get(char *name)
{
	Z_STRUCT_SECTION_FOREACH(pm_policy, policy) {
		if (strcmp(policy->name, name) == 0) {
			return policy;
		}
	}

	return NULL;
}

#ifdef __cplusplus
}
#endif

#endif
