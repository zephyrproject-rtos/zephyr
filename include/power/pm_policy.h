/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POWER_PM_POLICY_H_
#define ZEPHYR_INCLUDE_POWER_PM_POLICY_H_

#include <power/power.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PM Policy definition
 *
 * Store information about a power management policy.
 */
struct pm_policy {
	int (*next_state_get)(struct pm_policy *policy,
			      int32_t ticks, struct pm_state_info *info);
};

/**
 * @brief Statically define and register a power management policy
 *
 * @param policy_name The name of this policy
 * @param policy_next_state_get Callback to get the next PM state
 */
#define PM_POLICY_DEFINE(policy_name, policy_next_state_get)		\
	static const struct pm_policy policy_name = {			\
		.next_state_get = policy_next_state_get,		\
	};								\
	const struct pm_policy *z_sys_pm_policy = &policy_name

/**
 * @brief Function to get the next PM state based on the ticks
 *
 * @return
 *   - 0 in case of success
 *   - -ENOTSUP if there is no policy or the policy does not support
 *              next-state capability
 *   - A negative value indicating a specific failure
 */
static inline int pm_policy_next_state(int32_t ticks,
				       struct pm_state_info *info)
{
	extern const struct pm_policy *z_sys_pm_policy;

	if ((z_sys_pm_policy) == NULL ||
	    (z_sys_pm_policy->next_state_get == NULL)) {
		return -ENOTSUP;
	}

	return z_sys_pm_policy->next_state_get(
		(struct pm_policy *)z_sys_pm_policy, ticks, info);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POWER_PM_POLICY_H_ */
