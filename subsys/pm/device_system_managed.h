/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_PM_DEVICE_SYSTEM_MANAGED_H_
#define ZEPHYR_SUBSYS_PM_DEVICE_SYSTEM_MANAGED_H_

#ifdef CONFIG_PM_DEVICE_SYSTEM_MANAGED

bool pm_suspend_devices(bool turn_on_off_flag);
void pm_resume_devices(bool turn_on_off_flag);

#else

bool pm_suspend_devices(bool turn_on_off_flag) { return true; }
void pm_resume_devices(bool turn_on_off_flag) {}

#endif /* CONFIG_PM_DEVICE_SYSTEM_MANAGED */

#endif /* ZEPHYR_SUBSYS_PM_DEVICE_SYSTEM_MANAGED_H_ */
