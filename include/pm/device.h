/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PM_DEVICE_H_
#define ZEPHYR_INCLUDE_PM_DEVICE_H_

#include <kernel.h>
#include <sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Device Power Management API
 *
 * @defgroup device_power_management_api Device Power Management API
 * @ingroup power_management_api
 * @{
 */

struct device;

/** @brief Device power states. */
enum pm_device_state {
	/** Device is in active or regular state. */
	PM_DEVICE_STATE_ACTIVE,
	/**
	 * Device is suspended.
	 *
	 * @note
	 *     Device context may be lost.
	 */
	PM_DEVICE_STATE_SUSPEND,
	/**
	 * Device is turned off (power removed).
	 *
	 * @note
	 *     Device context is lost.
	 */
	PM_DEVICE_STATE_OFF,
	/** Device is being resumed. */
	PM_DEVICE_STATE_RESUMING,
	/** Device is being suspended. */
	PM_DEVICE_STATE_SUSPENDING,
};

/**
 * @brief Device PM info
 */
struct pm_device {
	/** Pointer to the device */
	const struct device *dev;
	/** Lock to synchronize the get/put operations */
	struct k_mutex lock;
	/* Following are packed fields protected by #lock. */
	/** Device pm enable flag */
	bool enable : 1;
	/* Following are packed fields accessed with atomic bit operations. */
	atomic_t atomic_flags;
	/** Device usage count */
	uint32_t usage;
	/** Device power state */
	enum pm_device_state state;
	/** Work object for asynchronous calls */
	struct k_work_delayable work;
	/** Event conditional var to listen to the sync request events */
	struct k_condvar condvar;
};

/** Bit position in device_pm::atomic_flags that records whether the
 * device is busy.
 */
#define PM_DEVICE_ATOMIC_FLAGS_BUSY_BIT 0

/**
 * @brief Get name of device PM state
 *
 * @param state State id which name should be returned
 */
const char *pm_device_state_str(enum pm_device_state state);

/**
 * @brief Set the power state of a device.
 *
 * This function calls the device PM control callback so that the device does
 * the necessary operations to put the device into the given state.
 *
 * @note Some devices may not support all device power states.
 *
 * @param dev Device instance.
 * @param state Device power state to be set.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If requested state is not supported.
 * @retval -EALREADY If device is already at (or transitioning to) the requested
 *         state.
 */
int pm_device_state_set(const struct device *dev,
			enum pm_device_state state);

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

/** Alias for legacy use of device_pm_control_nop */
#define device_pm_control_nop __DEPRECATED_MACRO NULL

/** @} */

#ifdef __cplusplus
}
#endif

#endif
