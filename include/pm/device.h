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
	PM_DEVICE_STATE_SUSPENDED,
	/**
	 * Device is turned off (power removed).
	 *
	 * @note
	 *     Device context is lost.
	 */
	PM_DEVICE_STATE_OFF
};

/** @brief Device PM flags. */
enum pm_device_flag {
	/** Indicate if the device is busy or not. */
	PM_DEVICE_FLAG_BUSY,
	/**
	 * Indicates whether or not the device is capable of waking the system
	 * up.
	 */
	PM_DEVICE_FLAGS_WS_CAPABLE,
	/** Indicates if the device is being used as wakeup source. */
	PM_DEVICE_FLAGS_WS_ENABLED,
	/** Indicates that the device is changing its state */
	PM_DEVICE_FLAG_TRANSITIONING,
	/** Number of flags (internal use only). */
	PM_DEVICE_FLAG_COUNT
};

/** @brief Device PM actions. */
enum pm_device_action {
	/** Suspend. */
	PM_DEVICE_ACTION_SUSPEND,
	/** Resume. */
	PM_DEVICE_ACTION_RESUME,
	/** Turn off. */
	PM_DEVICE_ACTION_TURN_OFF,
	/** Force suspend. */
	PM_DEVICE_ACTION_FORCE_SUSPEND,
	/** Low power. */
	PM_DEVICE_ACTION_LOW_POWER,
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
	/* Device PM status flags. */
	atomic_t flags;
	/** Device usage count */
	uint32_t usage;
	/** Device power state */
	enum pm_device_state state;
	/** Work object for asynchronous calls */
	struct k_work_delayable work;
	/** Event conditional var to listen to the sync request events */
	struct k_condvar condvar;
};

/**
 * @brief Utility macro to initialize #pm_device.
 *
 * @note DT_PROP_OR is used to retrieve the wakeup_source property because
 * it may not be defined on all devices.
 *
 * @param obj Name of the #pm_device structure being initialized.
 * @param node_id Devicetree node for the initialized device (can be invalid).
 */
#define Z_PM_DEVICE_INIT(obj, node_id)					\
	{								\
		.usage = 0U,						\
		.lock = Z_MUTEX_INITIALIZER(obj.lock),			\
		.condvar = Z_CONDVAR_INITIALIZER(obj.condvar),		\
		.state = PM_DEVICE_STATE_ACTIVE,			\
		.flags = ATOMIC_INIT(COND_CODE_1(			\
				DT_NODE_EXISTS(node_id),		\
				(DT_PROP_OR(node_id, wakeup_source, 0)),\
				(0)) << PM_DEVICE_FLAGS_WS_CAPABLE),	\
	}

/**
 * @brief Device power management control function callback.
 *
 * @param dev Device instance.
 * @param action Requested action.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If the requested action is not supported.
 * @retval Errno Other negative errno on failure.
 */
typedef int (*pm_device_control_callback_t)(const struct device *dev,
					    enum pm_device_action action);

#ifdef CONFIG_PM_DEVICE
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
 * @retval -EALREADY If device is already at the requested state.
 * @retval -EBUSY If device is changing its state.

 * @retval Errno Other negative errno on failure.
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

/**
 * @brief Indicate that the device is in the middle of a transaction
 *
 * Called by a device driver to indicate that it is in the middle of a
 * transaction.
 *
 * @param dev Pointer to device structure of the driver instance.
 */
void pm_device_busy_set(const struct device *dev);

/**
 * @brief Indicate that the device has completed its transaction
 *
 * Called by a device driver to indicate the end of a transaction.
 *
 * @param dev Pointer to device structure of the driver instance.
 */
void pm_device_busy_clear(const struct device *dev);

/**
 * @brief Check if any device is in the middle of a transaction
 *
 * Called by an application to see if any device is in the middle
 * of a critical transaction that cannot be interrupted.
 *
 * @retval false if no device is busy
 * @retval true if any device is busy
 */
bool pm_device_is_any_busy(void);

/**
 * @brief Check if a specific device is in the middle of a transaction
 *
 * Called by an application to see if a particular device is in the
 * middle of a critical transaction that cannot be interrupted.
 *
 * @param dev Pointer to device structure of the specific device driver
 * the caller is interested in.
 * @retval false if the device is not busy
 * @retval true if the device is busy
 */
bool pm_device_is_busy(const struct device *dev);
#else
static inline void pm_device_busy_set(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static inline void pm_device_busy_clear(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static inline bool pm_device_is_any_busy(void)
{
	return false;
}

static inline bool pm_device_is_busy(const struct device *dev)
{
	ARG_UNUSED(dev);

	return false;
}
#endif

__deprecated static inline void device_busy_set(const struct device *dev)
{
	pm_device_busy_set(dev);
}

__deprecated static inline void device_busy_clear(const struct device *dev)
{
	pm_device_busy_clear(dev);
}

__deprecated static inline int device_any_busy_check(void)
{
	return pm_device_is_any_busy() ? -EBUSY : 0;
}

__deprecated static inline int device_busy_check(const struct device *dev)
{
	return pm_device_is_busy(dev) ? -EBUSY : 0;
}

/** Alias for legacy use of device_pm_control_nop */
#define device_pm_control_nop __DEPRECATED_MACRO NULL

#ifdef CONFIG_PM_DEVICE
/**
 * @brief Enable a power management wakeup source
 *
 * Enable a wakeup source. This will keep the current device active when the
 * system is suspended, allowing it to be used to wake up the system.
 *
 * @param dev device object to enable.
 * @param enable @c true to enable or @c false to disable
 *
 * @retval true if the wakeup source was successfully enabled.
 * @retval false if the wakeup source was not successfully enabled.
 */
bool pm_device_wakeup_enable(struct device *dev, bool enable);

/**
 * @brief Check if a power management wakeup source is enabled
 *
 * Checks if a wake up source is enabled.
 *
 * @param dev device object to check.
 *
 * @retval true if the wakeup source is enabled.
 * @retval false if the wakeup source is not enabled.
 */
bool pm_device_wakeup_is_enabled(const struct device *dev);

/**
 * @brief Check if a device is wake up capable
 *
 * @param dev device object to check.
 *
 * @retval true if the device is wake up capable.
 * @retval false if the device is not wake up capable.
 */
bool pm_device_wakeup_is_capable(const struct device *dev);
#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif
