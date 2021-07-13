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
	 * Device is in low power state.
	 *
	 * @note
	 *     Device context is preserved.
	 */
	PM_DEVICE_STATE_LOW_POWER,
	/**
	 * Device is suspended.
	 *
	 * @note
	 *     Device context may be lost.
	 */
	PM_DEVICE_STATE_SUSPEND,
	/**
	 * Device is suspended (forced).
	 *
	 * @note
	 *     Device context may be lost.
	 */
	PM_DEVICE_STATE_FORCE_SUSPEND,
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

/** Device PM set state control command. */
#define PM_DEVICE_STATE_SET 0
/** Device PM get state control command. */
#define PM_DEVICE_STATE_GET 1

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
 * Bit position in device_pm::atomic_flags that records whether or not
 * the device is wakeup capable.
 */
#define PM_DEVICE_ATOMIC_FLAGS_WS_CAPABLE_BIT 1U

/**
 * Bit position in device_pm::atomic_flags that records whether or not
 * the device is being used as a wakeup source.
 */
#define PM_DEVICE_ATOMIC_FLAGS_WS_ENABLED_BIT 2U

/**
 * @brief Get name of device PM state
 *
 * @param state State id which name should be returned
 */
const char *pm_device_state_str(enum pm_device_state state);

/**
 * @brief Call the set power state function of a device
 *
 * Called by the application or power management service to let the device do
 * required operations when moving to the required power state
 * Note that devices may support just some of the device power states
 * @param dev Pointer to device structure of the driver instance.
 * @param device_power_state Device power state to be set
 *
 * @retval 0 If successful in queuing the request or changing the state.
 * @retval Errno Negative errno code if failure.
 */
int pm_device_state_set(const struct device *dev,
			enum pm_device_state device_power_state);

/**
 * @brief Call the get power state function of a device
 *
 * This function lets the caller know the current device
 * power state at any time. This state will be one of the defined
 * power states allowed for the devices in that system
 *
 * @param dev pointer to device structure of the driver instance.
 * @param device_power_state Device power state to be filled by the device
 *
 * @retval 0 If successful.
 * @retval Errno Negative errno code if failure.
 */
int pm_device_state_get(const struct device *dev,
			enum pm_device_state *device_power_state);

/** Alias for legacy use of device_pm_control_nop */
#define device_pm_control_nop __DEPRECATED_MACRO NULL

/**
 * @brief Enable a power management wakeup source
 *
 * Enable a wakeup source. This will keep the current device active when the
 * system is suspended, allowing it to be used to wake up the system.
 *
 * @param dev device object to enable.
 * @param enable @c true to enable or @c false to disable
 *
 * @return @c true if the wakeup source was successfully enabled,
 *         @c false otherwise.
 */
bool pm_device_wakeup_enable(struct device *dev, bool enable);

/**
 * @brief Check if a power management wakeup source is enabled
 *
 * Checks if a wake up source is enabled.
 *
 * @param dev device object to check.
 *
 * @return @c true if the wakeup source is enabled,
 *         @c false otherwise.
 */
bool pm_device_wakeup_is_enabled(const struct device *dev);

/**
 * @brief Check if a device is wake up capable
 *
 * @param dev device object check.
 *
 * @return @c true if the device is wake up capable,
 *         @c false otherwise.
 */
bool pm_device_wakeup_capable(const struct device *dev);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
