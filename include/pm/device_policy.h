/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PM_DEVICE_POLICY_H_
#define ZEPHYR_INCLUDE_PM_DEVICE_POLICY_H_

#include <pm/state.h>
#include <pm/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function to get the device power state given a system power state.
 *
 * This function is called by the power subsystem when the system is
 * changing its power state to figure out which state a device should use.
 *
 * @param dev The device that will change the state.
 * @param state The power state that the system will use.
 *
 * @return The device power state the system should use.
 */
enum pm_device_state pm_device_policy_next_state(const struct device *dev,
					 const struct pm_state_info *state);


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PM_POLICY_H_ */
