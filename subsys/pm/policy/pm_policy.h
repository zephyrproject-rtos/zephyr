/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_POWER_PM_POLICY_H_
#define ZEPHYR_POWER_PM_POLICY_H_

#include <pm/pm.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function to create device PM list
 */
void pm_create_device_list(void);

/**
 * @brief Function to suspend the devices in PM device list
 */
int pm_suspend_devices(void);

/**
 * @brief Function to put the devices in PM device list in low power state
 */
int pm_low_power_devices(void);

/**
 * @brief Function to force suspend the devices in PM device list
 */
int pm_force_suspend_devices(void);

/**
 * @brief Function to resume the devices in PM device list
 */
void pm_resume_devices(void);

/**
 * @brief Function to get the next PM state based on the ticks
 */
struct pm_state_info pm_policy_next_state(int32_t ticks);


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_POWER_PM_POLICY_H_ */
