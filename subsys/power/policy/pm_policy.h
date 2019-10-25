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

#if defined(CONFIG_DEVICE_PM_CENTRAL_METHOD)
/**
 * @brief Function to suspend the devices in PM device list
 */
void sys_pm_suspend_devices(void);

/**
 * @brief Function to put the devices in PM device list in
 *        low power state(clock gate device).
 */
void sys_pm_clock_gate_devices(void);

/**
 * @brief Function to resume the devices in PM device list
 */
void sys_pm_resume_devices(void);

/**
 * @brief Function to determine whether to put devices in low
 *        power state(clock gate device), given the system PM state.
 */
bool sys_pm_policy_clock_gate_devices(enum power_states pm_state);
#endif

/**
 * @brief Function to get the next PM state based on the ticks
 */
enum power_states sys_pm_policy_next_state(s32_t ticks);

#ifdef __cplusplus
}
#endif

#endif /* _PM_POLICY_H_ */
