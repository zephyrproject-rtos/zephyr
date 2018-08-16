/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PM_POLICY_H_
#define _PM_POLICY_H_

#include <power.h>
#include <soc_power.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function to create device PM list
 */
extern  void sys_pm_create_device_list(void);

/**
 * @brief Function to suspend the devices in PM device list
 */
extern int sys_pm_suspend_devices(void);

/**
 * @brief Function to resume the devices in PM device list
 */
extern void sys_pm_resume_devices(void);

/**
 * @brief Function to get the next PM state based on the ticks
 */
extern int sys_pm_policy_next_state(s32_t ticks, enum power_states *state);

/**
 * @brief Application defined function for Lower Power entry
 *
 * Application defined function for doing any target specific operations
 * for low power entry.
 */
extern void sys_pm_notify_lps_entry(enum power_states state);

/**
 * @brief Application defined function for Lower Power exit
 *
 * Application defined function for doing any target specific operations
 * for low power exit.
 */
extern void sys_pm_notify_lps_exit(enum power_states state);

#ifdef __cplusplus
}
#endif

#endif /* _PM_POLICY_H_ */
