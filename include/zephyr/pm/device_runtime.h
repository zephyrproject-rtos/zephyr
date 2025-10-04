/*
 * Copyright (c) 2015 Intel Corporation.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PM_DEVICE_RUNTIME_H_
#define ZEPHYR_INCLUDE_PM_DEVICE_RUNTIME_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Device Runtime Power Management API
 * @defgroup subsys_pm_device_runtime Device Runtime
 * @ingroup subsys_pm
 * @{
 */

#if defined(CONFIG_PM_DEVICE_RUNTIME) || defined(__DOXYGEN__)
/**
 * @brief Automatically enable device runtime based on devicetree properties
 *
 * @note Must not be called from application code. See the
 * zephyr,pm-device-runtime-auto property in pm.yaml and z_sys_init_run_level.
 *
 * @param dev Device instance.
 *
 * @retval 0 If the device runtime PM is enabled successfully or it has not
 * been requested for this device in devicetree.
 * @retval -errno Other negative errno, result of enabled device runtime PM.
 */
int pm_device_runtime_auto_enable(const struct device *dev);

/**
 * @brief Enable device runtime PM
 *
 * This function will enable runtime PM on the given device. If the device is
 * in #PM_DEVICE_STATE_ACTIVE state, the device will be suspended.
 *
 * @funcprops \pre_kernel_ok
 *
 * @param dev Device instance.
 *
 * @retval 0 If the device runtime PM is enabled successfully.
 * @retval -EBUSY If device is busy.
 * @retval -ENOTSUP If the device does not support PM.
 * @retval -errno Other negative errno, result of suspending the device.
 *
 * @see pm_device_init_suspended()
 */
int pm_device_runtime_enable(const struct device *dev);

/**
 * @brief Disable device runtime PM
 *
 * If the device is currently suspended it will be resumed.
 *
 * @funcprops \pre_kernel_ok
 *
 * @param dev Device instance.
 *
 * @retval 0 If the device runtime PM is disabled successfully.
 * @retval -ENOTSUP If the device does not support PM.
 * @retval -errno Other negative errno, result of resuming the device.
 */
int pm_device_runtime_disable(const struct device *dev);

/**
 * @brief Resume a device based on usage count.
 *
 * This function will resume the device if the device is suspended (usage count
 * equal to 0). In case of a resume failure, usage count and device state will
 * be left unchanged. In all other cases, usage count will be incremented.
 *
 * If the device is still being suspended as a result of calling
 * pm_device_runtime_put_async(), this function will wait for the operation to
 * finish to then resume the device.
 *
 * @note It is safe to use this function in contexts where blocking is not
 * allowed, e.g. ISR, provided the device PM implementation does not block.
 *
 * @funcprops \pre_kernel_ok
 *
 * @param dev Device instance.
 *
 * @retval 0 If it succeeds. In case device runtime PM is not enabled or not
 * available this function will be a no-op and will also return 0.
 * @retval -EWOUDBLOCK If call would block but it is not allowed (e.g. in ISR).
 * @retval -errno Other negative errno, result of the PM action callback.
 */
int pm_device_runtime_get(const struct device *dev);

/**
 * @brief Suspend a device based on usage count.
 *
 * This function will suspend the device if the device is no longer required
 * (usage count equal to 0). In case of suspend failure, usage count and device
 * state will be left unchanged. In all other cases, usage count will be
 * decremented (down to 0).
 *
 * @funcprops \pre_kernel_ok
 *
 * @param dev Device instance.
 *
 * @retval 0 If it succeeds. In case device runtime PM is not enabled or not
 * available this function will be a no-op and will also return 0.
 * @retval -EALREADY If device is already suspended (can only happen if get/put
 * calls are unbalanced).
 * @retval -errno Other negative errno, result of the action callback.
 *
 * @see pm_device_runtime_put_async()
 */
int pm_device_runtime_put(const struct device *dev);

/**
 * @brief Suspend a device based on usage count (asynchronously).
 *
 * This function will schedule the device suspension if the device is no longer
 * required (usage count equal to 0). In all other cases, usage count will be
 * decremented (down to 0).
 *
 * @note Asynchronous operations are not supported when in pre-kernel mode. In
 * this case, the function will be blocking (equivalent to
 * pm_device_runtime_put()).
 *
 * @funcprops \pre_kernel_ok, \async, \isr_ok
 *
 * @param dev Device instance.
 * @param delay Minimum amount of time before triggering the action.
 *
 * @retval 0 If it succeeds. In case device runtime PM is not enabled or not
 * available this function will be a no-op and will also return 0.
 * @retval -EBUSY If the device is busy.
 * @retval -EALREADY If device is already suspended (can only happen if get/put
 * calls are unbalanced).
 *
 * @see pm_device_runtime_put()
 */
int pm_device_runtime_put_async(const struct device *dev, k_timeout_t delay);

/**
 * @brief Check if device runtime is enabled for a given device.
 *
 * @funcprops \pre_kernel_ok
 *
 * @param dev Device instance.
 *
 * @retval true If device has device runtime PM enabled.
 * @retval false If the device has device runtime PM disabled.
 *
 * @see pm_device_runtime_enable()
 */
bool pm_device_runtime_is_enabled(const struct device *dev);

/**
 * @brief Return the current device usage counter.
 *
 * @param dev Device instance.
 *
 * @returns the current usage counter.
 * @retval -ENOTSUP If the device is not using runtime PM.
 * @retval -ENOSYS If the runtime PM is not enabled at all.
 */
int pm_device_runtime_usage(const struct device *dev);

/** @cond INTERNAL_HIDDEN */

struct pm_device_runtime_reference {
	bool active;
};

/** @endcond */

/**
 * @brief Statically initialize a PM device runtime reference
 *
 * @note Must be called only once for each PM device runtime reference
 * prior to use with other APIs.
 */
#define PM_DEVICE_RUNTIME_REFERENCE_INIT() \
	{.active = false}

/**
 * @brief Statically define and initialize a PM device runtime reference.
 *
 * @param name Name of the PM device runtime reference.
 */
#define PM_DEVICE_RUNTIME_REFERENCE_DEFINE(name) \
	struct pm_device_runtime_reference name = PM_DEVICE_RUNTIME_REFERENCE_INIT();

/**
 * @brief Initialize a PM device runtime reference.
 *
 * @note Must be called only once for each PM device runtime reference
 * prior to use with other APIs if not statically defined or initialized.
 *
 * @param ref PM device runtime reference.
 */
void pm_device_runtime_reference_init(struct pm_device_runtime_reference *ref);

/**
 * @brief Check if a a PM device runtime reference is active.
 *
 * @param ref PM device runtime reference.
 *
 * @retval true if PM device runtime reference is active
 * @retval false if PM device runtime reference is not active
 */
static inline bool pm_device_runtime_reference_is_active(struct pm_device_runtime_reference *ref)
{
	return ref->active;
}

/**
 * @brief Resume a device based on a reference.
 *
 * @details Activates a reference to a device, ensuring the device is resumed
 * by increasing the PM device runtime usage count by 1. The reference is
 * deactivated using @ref pm_device_runtime_release or
 * @ref pm_device_runtime_release_async, which will decrease the PM device
 * runtime usage count by 1.
 *
 * @note If the reference is already active, this API is a no-op.
 *
 * @param dev Device instance to reference.
 * @param ref PM device runtime reference to activate.
 *
 * @retval 0 if successful.
 * @retval -errno code if failure.
 */
int pm_device_runtime_request(const struct device *dev,
			      struct pm_device_runtime_reference *ref);

/**
 * @brief Suspends a device based on a reference.
 *
 * @details Deactivates a reference to a device, decreasing the PM device
 * runtime usage count by 1, thus no longer ensuring the device is resumed.
 *
 * @note If the reference is already inactive, this API is a no-op.
 *
 * @param dev Device instance to dereference.
 * @param ref PM device runtime reference to deactivate.
 *
 * @retval 0 if successful.
 * @retval -errno code if failure.
 */
int pm_device_runtime_release(const struct device *dev,
			      struct pm_device_runtime_reference *ref);

/**
 * @brief Suspends a device based on a reference asynchronously.
 *
 * @details Deactivates a reference to a device, decreasing the PM device
 * runtime usage count by 1, thus no longer ensuring the device is resumed
 * after a specified delay, asynchronously.
 *
 * @note If the reference is already inactive, this API is a no-op.
 *
 * @param dev Device instance to dereference.
 * @param ref PM device runtime reference to deactivate.
 * @param delay Minimum amount of time before the reference will be deactivated.
 *
 * @retval 0 if successful.
 * @retval -errno code if failure.
 */
int pm_device_runtime_release_async(const struct device *dev,
				    struct pm_device_runtime_reference *ref,
				    k_timeout_t delay);

#else

static inline int pm_device_runtime_auto_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static inline int pm_device_runtime_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static inline int pm_device_runtime_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static inline int pm_device_runtime_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static inline int pm_device_runtime_put(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static inline int pm_device_runtime_put_async(const struct device *dev,
		k_timeout_t delay)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(delay);
	return 0;
}

static inline bool pm_device_runtime_is_enabled(const struct device *dev)
{
	ARG_UNUSED(dev);
	return false;
}

static inline int pm_device_runtime_usage(const struct device *dev)
{
	ARG_UNUSED(dev);
	return -ENOSYS;
}

struct pm_device_runtime_reference {
};

#define PM_DEVICE_RUNTIME_REFERENCE_INIT() {}

#define PM_DEVICE_RUNTIME_REFERENCE_DEFINE(name) \
	struct pm_device_runtime_reference name;

static inline void pm_device_runtime_reference_init(struct pm_device_runtime_reference *ref)
{
	ARG_UNUSED(ref);
}

static inline bool pm_device_runtime_reference_is_active(struct pm_device_runtime_reference *ref)
{
	ARG_UNUSED(ref);
	return true;
}

static inline int pm_device_runtime_request(const struct device *dev,
					    struct pm_device_runtime_reference *ref)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ref);
	return 0;
}

static inline int pm_device_runtime_release(const struct device *dev,
					    struct pm_device_runtime_reference *ref)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ref);
	return 0;
}

static inline int pm_device_runtime_release_async(const struct device *dev,
						  struct pm_device_runtime_reference *ref,
						  k_timeout_t delay)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ref);
	ARG_UNUSED(delay);
	return 0;
}

#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PM_DEVICE_RUNTIME_H_ */
