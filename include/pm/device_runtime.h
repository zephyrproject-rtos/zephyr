/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PM_DEVICE_RUNTIME_H_
#define ZEPHYR_INCLUDE_PM_DEVICE_RUNTIME_H_

#include <device.h>
#include <kernel.h>
#include <sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Runtime Power Management API
 *
 * @defgroup runtime_power_management_api Runtime Power Management API
 * @ingroup power_management_api
 * @{
 */

#ifdef CONFIG_PM_DEVICE_RUNTIME

/**
 * @brief Enable device runtime PM
 *
 * Called by a device driver to enable device runtime power management.
 * The device might be asynchronously suspended if runtime PM is enabled
 * when the device is not use.
 *
 * @param dev Pointer to device structure of the specific device driver
 * the caller is interested in.
 */
void pm_device_enable(const struct device *dev);

/**
 * @brief Disable device runtime PM
 *
 * Called by a device driver to disable device runtime power management.
 * The device might be asynchronously resumed if runtime PM is disabled
 *
 * @param dev Pointer to device structure of the specific device driver
 * the caller is interested in.
 */
void pm_device_disable(const struct device *dev);

/**
 * @brief Call device resume asynchronously based on usage count
 *
 * Called by a device driver to mark the device as being used.
 * This API will asynchronously bring the device to resume state
 * if it not already in active state.
 *
 * @param dev Pointer to device structure of the specific device driver
 * the caller is interested in.
 * @retval 0 If successfully queued the Async request. If queued,
 * the caller need to wait on the poll event linked to device
 * pm signal mechanism to know the completion of resume operation.
 * @retval Errno Negative errno code if failure.
 */
int pm_device_get(const struct device *dev);

/**
 * @brief Call device resume synchronously based on usage count
 *
 * Called by a device driver to mark the device as being used. It
 * will bring up or resume the device if it is in suspended state
 * based on the device usage count. This call is blocked until the
 * device PM state is changed to resume.
 *
 * @param dev Pointer to device structure of the specific device driver
 * the caller is interested in.
 * @retval 0 If successful.
 * @retval Errno Negative errno code if failure.
 */
int pm_device_get_sync(const struct device *dev);

/**
 * @brief Call device suspend asynchronously based on usage count
 *
 * Called by a device driver to mark the device as being released.
 * This API asynchronously put the device to suspend state if
 * it not already in suspended state.
 *
 * @param dev Pointer to device structure of the specific device driver
 * the caller is interested in.
 * @retval 0 If successfully queued the Async request. If queued,
 * the caller need to wait on the poll event linked to device pm
 * signal mechanism to know the completion of suspend operation.
 * @retval Errno Negative errno code if failure.
 */
int pm_device_put(const struct device *dev);

/**
 * @brief Call device suspend synchronously based on usage count
 *
 * Called by a device driver to mark the device as being released. It
 * will put the device to suspended state if is is in active state
 * based on the device usage count. This call is blocked until the
 * device PM state is changed to resume.
 *
 * @param dev Pointer to device structure of the specific device driver
 * the caller is interested in.
 * @retval 0 If successful.
 * @retval Errno Negative errno code if failure.
 */
int pm_device_put_sync(const struct device *dev);
#else
static inline void pm_device_enable(const struct device *dev) { }
static inline void pm_device_disable(const struct device *dev) { }
static inline int pm_device_get(const struct device *dev) { return -ENOSYS; }
static inline int pm_device_get_sync(const struct device *dev) { return -ENOSYS; }
static inline int pm_device_put(const struct device *dev) { return -ENOSYS; }
static inline int pm_device_put_sync(const struct device *dev) { return -ENOSYS; }
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_PM_DEVICE_RUNTIME_H_ */
