/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_PM_PRIV_H_
#define ZEPHYR_SUBSYS_PM_PRIV_H_

#include <pm/pm.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_PM_PRIV_H_ */
