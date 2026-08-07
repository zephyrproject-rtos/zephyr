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

/**
 * @brief Apply an externally driven power-domain action to a runtime PM device.
 *
 * The caller must run in thread context, serialize calls for @p dev, and must
 * not call this function from @p dev's PM action callback or runtime suspend
 * work handler. TURN_OFF invalidates runtime PM usage and reconciles any pending
 * asynchronous suspend before publishing OFF. TURN_ON publishes SUSPENDED so a
 * later runtime get can resume the device.
 *
 * @param dev Device receiving the power-domain action.
 * @param action PM_DEVICE_ACTION_TURN_ON or PM_DEVICE_ACTION_TURN_OFF.
 *
 * @retval 0 Success.
 * @retval -EWOULDBLOCK Called from interrupt context.
 * @retval -EINVAL Unsupported action.
 * @retval -ENOTSUP Device has no PM metadata or the action is invalid for its state.
 * @retval -errno Device callback or parent-domain release error.
 */
int z_pm_device_runtime_power_domain_action_run(const struct device *dev,
						enum pm_device_action action);

#ifdef CONFIG_TEST_PM_DEVICE_RUNTIME_HOOKS
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
#endif /* CONFIG_TEST_PM_DEVICE_RUNTIME_HOOKS */

#ifdef __cplusplus
}
#endif

/** @endcond */

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PM_DEVICE_RUNTIME_INTERNAL_H_ */
