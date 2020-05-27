/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PM_POLICY_H_
#define _PM_POLICY_H_

#include <power/power.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function to create device PM list
 */
void sys_pm_create_device_list(void);

/**
 * @brief Function to suspend the devices in PM device list
 */
int sys_pm_suspend_devices(void);

/**
 * @brief Function to put the devices in PM device list in low power state
 */
int sys_pm_low_power_devices(void);

/**
 * @brief Function to force suspend the devices in PM device list
 */
int sys_pm_force_suspend_devices(void);

/**
 * @brief Function to resume the devices in PM device list
 */
void sys_pm_resume_devices(void);

/**
 * @brief Function to get the next PM state based on the ticks
 */
enum power_states sys_pm_policy_next_state(int32_t ticks);

/**
 * @brief Function to determine whether to put devices in low
 *        power state, given the system PM state.
 */
bool sys_pm_policy_low_power_devices(enum power_states pm_state);

#ifdef __cplusplus
}
#endif

#endif /* _PM_POLICY_H_ */
