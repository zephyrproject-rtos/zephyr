/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal runtime power management helpers.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PM_DEVICE_RUNTIME_INTERNAL_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PM_DEVICE_RUNTIME_INTERNAL_H_

#include <zephyr/pm/device.h>

/** @cond INTERNAL_HIDDEN */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_PM_DEVICE_RUNTIME_TESTING
enum z_pm_device_runtime_test_hook {
	Z_PM_DEVICE_RUNTIME_HOOK_BEFORE_GET,
	Z_PM_DEVICE_RUNTIME_HOOK_AFTER_GET,
	Z_PM_DEVICE_RUNTIME_HOOK_BEFORE_PUT,
	Z_PM_DEVICE_RUNTIME_HOOK_AFTER_PUT,
	Z_PM_DEVICE_RUNTIME_HOOK_BEFORE_PD_ACTION,
	Z_PM_DEVICE_RUNTIME_HOOK_AFTER_PD_ACTION,
};

void z_pm_device_runtime_test_hook(const struct device *dev,
				   enum z_pm_device_runtime_test_hook hook);
#endif /* CONFIG_PM_DEVICE_RUNTIME_TESTING */

#ifdef __cplusplus
}
#endif

/** @endcond */

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PM_DEVICE_RUNTIME_INTERNAL_H_ */