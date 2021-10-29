/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PM_POLICY_H_
#define ZEPHYR_INCLUDE_PM_POLICY_H_

#include <pm/state.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Function to get the next PM state
 *
 * This function is called by the power subsystem when the system is
 * idle and returns the most appropriate state based on the number of
 * ticks to the next event.
 *
 * @param ticks The number of ticks to the next scheduled event.
 *
 * @return The power state the system should use.
 */
struct pm_state_info pm_policy_next_state(int32_t ticks);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PM_POLICY_H_ */
