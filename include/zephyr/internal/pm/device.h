/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INTERNAL_PM_DEVICE_H_
#define ZEPHYR_INCLUDE_INTERNAL_PM_DEVICE_H_

#include <zephyr/pm/device.h>

/**
 * @brief Get name of device PM state
 *
 * @param state State id which name should be returned
 */
const char *pm_device_state_str(enum pm_device_state state);

/**
 * @brief Run a pm action on a device.
 *
 * This function calls the device PM control callback so that the device does
 * the necessary operations to execute the given action.
 *
 * @param dev Device instance.
 * @param action Device pm action.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If requested state is not supported.
 * @retval -EALREADY If device is already at the requested state.
 * @retval -EBUSY If device is changing its state.
 * @retval -ENOSYS If device does not support PM.
 * @retval -EPERM If device has power state locked.
 * @retval Errno Other negative errno on failure.
 */
int pm_device_action_run(const struct device *dev,
			 enum pm_device_action action);

/**
 * @brief Run a pm action on all children of a device.
 *
 * This function calls all child devices PM control callback so that the device
 * does the necessary operations to execute the given action.
 *
 * @param dev Device instance.
 * @param action Device pm action.
 * @param failure_cb Function to call if a child fails the action, can be NULL.
 */
void pm_device_children_action_run(const struct device *dev,
				   enum pm_device_action action,
				   pm_device_action_failed_cb_t failure_cb);

/**
 * @brief Obtain the power state of a device.
 *
 * @param dev Device instance.
 * @param state Pointer where device power state will be stored.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If device does not implement power management.
 */
int pm_device_state_get(const struct device *dev,
			enum pm_device_state *state);

/**
 * @brief Initialize a device state to #PM_DEVICE_STATE_SUSPENDED.
 *
 * By default device state is initialized to #PM_DEVICE_STATE_ACTIVE. However
 * in order to save power some drivers may choose to only initialize the device
 * to the suspended state, or actively put the device into the suspended state.
 * This function can therefore be used to notify the PM subsystem that the
 * device is in #PM_DEVICE_STATE_SUSPENDED instead of the default.
 *
 * @param dev Device instance.
 */
static inline void pm_device_init_suspended(const struct device *dev)
{
	struct pm_device_base *pm = dev->pm_base;

	pm->state = PM_DEVICE_STATE_SUSPENDED;
}

/**
 * @brief Initialize a device state to #PM_DEVICE_STATE_OFF.
 *
 * By default device state is initialized to #PM_DEVICE_STATE_ACTIVE. In
 * general, this makes sense because the device initialization function will
 * resume and configure a device, leaving it operational. However, when power
 * domains are enabled, the device may be connected to a switchable power
 * source, in which case it won't be powered at boot. This function can
 * therefore be used to notify the PM subsystem that the device is in
 * #PM_DEVICE_STATE_OFF instead of the default.
 *
 * @param dev Device instance.
 */
static inline void pm_device_init_off(const struct device *dev)
{
	struct pm_device_base *pm = dev->pm_base;

	pm->state = PM_DEVICE_STATE_OFF;
}

/**
 * @brief Mark a device as busy.
 *
 * Devices marked as busy will not be suspended when the system goes into
 * low-power states. This can be useful if, for example, the device is in the
 * middle of a transaction.
 *
 * @param dev Device instance.
 *
 * @see pm_device_busy_clear()
 */
void pm_device_busy_set(const struct device *dev);

/**
 * @brief Clear a device busy status.
 *
 * @param dev Device instance.
 *
 * @see pm_device_busy_set()
 */
void pm_device_busy_clear(const struct device *dev);

/**
 * @brief Check if any device is busy.
 *
 * @retval false If no device is busy
 * @retval true If one or more devices are busy
 */
bool pm_device_is_any_busy(void);

/**
 * @brief Check if a device is busy.
 *
 * @param dev Device instance.
 *
 * @retval false If the device is not busy
 * @retval true If the device is busy
 */
bool pm_device_is_busy(const struct device *dev);

/**
 * @brief Enable or disable a device as a wake up source.
 *
 * A device marked as a wake up source will not be suspended when the system
 * goes into low-power modes, thus allowing to use it as a wake up source for
 * the system.
 *
 * @param dev Device instance.
 * @param enable @c true to enable or @c false to disable
 *
 * @retval true If the wakeup source was successfully enabled.
 * @retval false If the wakeup source was not successfully enabled.
 */
bool pm_device_wakeup_enable(const struct device *dev, bool enable);

/**
 * @brief Check if a device is enabled as a wake up source.
 *
 * @param dev Device instance.
 *
 * @retval true if the wakeup source is enabled.
 * @retval false if the wakeup source is not enabled.
 */
bool pm_device_wakeup_is_enabled(const struct device *dev);

/**
 * @brief Check if a device is wake up capable
 *
 * @param dev Device instance.
 *
 * @retval true If the device is wake up capable.
 * @retval false If the device is not wake up capable.
 */
bool pm_device_wakeup_is_capable(const struct device *dev);

/**
 * @brief Check if the device is on a switchable power domain.
 *
 * @param dev Device instance.
 *
 * @retval true If device is on a switchable power domain.
 * @retval false If device is not on a switchable power domain.
 */
bool pm_device_on_power_domain(const struct device *dev);

/**
 * @brief Add a device to a power domain.
 *
 * This function adds a device to a given power domain.
 *
 * @param dev Device to be added to the power domain.
 * @param domain Power domain.
 *
 * @retval 0 If successful.
 * @retval -EALREADY If device is already part of the power domain.
 * @retval -ENOSYS If the application was built without power domain support.
 * @retval -ENOSPC If there is no space available in the power domain to add the device.
 */
int pm_device_power_domain_add(const struct device *dev,
			       const struct device *domain);

/**
 * @brief Remove a device from a power domain.
 *
 * This function removes a device from a given power domain.
 *
 * @param dev Device to be removed from the power domain.
 * @param domain Power domain.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If the application was built without power domain support.
 * @retval -ENOENT If device is not in the given domain.
 */
int pm_device_power_domain_remove(const struct device *dev,
				  const struct device *domain);

/**
 * @brief Check if the device is currently powered.
 *
 * @param dev Device instance.
 *
 * @retval true If device is currently powered, or is assumed to be powered
 * (i.e. it does not support PM or is not under a PM domain)
 * @retval false If device is not currently powered
 */
bool pm_device_is_powered(const struct device *dev);

/** @} */

#endif /* ZEPHYR_INCLUDE_INTERNAL_PM_DEVICE_H_ */
