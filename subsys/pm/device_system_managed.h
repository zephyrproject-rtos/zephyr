/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_PM_DEVICE_SYSTEM_MANAGED_H_
#define ZEPHYR_SUBSYS_PM_DEVICE_SYSTEM_MANAGED_H_

#ifdef CONFIG_PM_DEVICE_SYSTEM_MANAGED

bool pm_suspend_devices(void);
void pm_resume_devices(void);

#else

bool pm_suspend_devices(void) { return true; }
void pm_resume_devices(void) {}

#endif /* CONFIG_PM_DEVICE_SYSTEM_MANAGED */

#endif /* ZEPHYR_SUBSYS_PM_DEVICE_SYSTEM_MANAGED_H_ */
