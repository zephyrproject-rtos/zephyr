/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PM_DEVICE_H_
#define ZEPHYR_INCLUDE_PM_DEVICE_H_

#include <device.h>
#include <kernel.h>
#include <sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Device Power Management API
 * @defgroup subsys_pm_device Device
 * @ingroup subsys_pm
 * @{
 */

/** @cond INTERNAL_HIDDEN */

struct device;

/** @brief Device PM flags. */
enum pm_device_flag {
	/** Indicate if the device is busy or not. */
	PM_DEVICE_FLAG_BUSY,
	/**
	 * Indicates whether or not the device is capable of waking the system
	 * up.
	 */
	PM_DEVICE_FLAG_WS_CAPABLE,
	/** Indicates if the device is being used as wakeup source. */
	PM_DEVICE_FLAG_WS_ENABLED,
	/** Indicates if device runtime is enabled  */
	PM_DEVICE_FLAG_RUNTIME_ENABLED,
	/** Indicates if the device pm is locked.  */
	PM_DEVICE_FLAG_STATE_LOCKED,
};

/** @endcond */

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
	PM_DEVICE_STATE_SUSPENDED,
	/** Device is being suspended. */
	PM_DEVICE_STATE_SUSPENDING,
	/**
	 * Device is turned off (power removed).
	 *
	 * @note
	 *     Device context is lost.
	 */
	PM_DEVICE_STATE_OFF
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
};

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Device PM action callback.
 *
 * @param dev Device instance.
 * @param action Requested action.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If the requested action is not supported.
 * @retval Errno Other negative errno on failure.
 */
typedef int (*pm_device_action_cb_t)(const struct device *dev,
				     enum pm_device_action action);

/**
 * @brief Device PM info
 */
struct pm_device {
#if defined(CONFIG_PM_DEVICE_RUNTIME) || defined(__DOXYGEN__)
	/** Pointer to the device */
	const struct device *dev;
	/** Lock to synchronize the get/put operations */
	struct k_mutex lock;
	/** Device usage count */
	uint32_t usage;
	/** Work object for asynchronous calls */
	struct k_work_delayable work;
	/** Event conditional var to listen to the sync request events */
	struct k_condvar condvar;
#endif /* CONFIG_PM_DEVICE_RUNTIME */
	/* Device PM status flags. */
	atomic_t flags;
	/** Device power state */
	enum pm_device_state state;
	/** Device PM action callback */
	pm_device_action_cb_t action_cb;
};

#ifdef CONFIG_PM_DEVICE_RUNTIME
#define Z_PM_DEVICE_RUNTIME_INIT(obj)			\
	.lock = Z_MUTEX_INITIALIZER(obj.lock),		\
	.condvar = Z_CONDVAR_INITIALIZER(obj.condvar),
#else
#define Z_PM_DEVICE_RUNTIME_INIT(obj)
#endif /* CONFIG_PM_DEVICE_RUNTIME */

/**
 * @brief Utility macro to initialize #pm_device.
 *
 * @note #DT_PROP_OR is used to retrieve the wakeup_source property because
 * it may not be defined on all devices.
 *
 * @param obj Name of the #pm_device structure being initialized.
 * @param node_id Devicetree node for the initialized device (can be invalid).
 * @param pm_action_cb Device PM control callback function.
 */
#define Z_PM_DEVICE_INIT(obj, node_id, pm_action_cb)			\
	{								\
		Z_PM_DEVICE_RUNTIME_INIT(obj)				\
		.action_cb = pm_action_cb,				\
		.state = PM_DEVICE_STATE_ACTIVE,			\
		.flags = ATOMIC_INIT(COND_CODE_1(			\
				DT_NODE_EXISTS(node_id),		\
				(DT_PROP_OR(node_id, wakeup_source, 0)),\
				(0)) << PM_DEVICE_FLAG_WS_CAPABLE),	\
	}

/**
 * Get the name of device PM resources.
 *
 * @param dev_name Device name.
 */
#define Z_PM_DEVICE_NAME(dev_name) _CONCAT(__pm_device__, dev_name)

/**
 * @brief Define device PM slot.
 *
 * This macro defines a pointer to a device in the z_pm_device_slots region.
 * When invoked for each device with PM, it will effectively result in a device
 * pointer array with the same size of the actual devices with PM enabled. This
 * is used internally by the PM subsystem to keep track of suspended devices
 * during system power transitions.
 *
 * @param dev_name Device name.
 */
#define Z_PM_DEVICE_DEFINE_SLOT(dev_name)				\
	static const Z_DECL_ALIGN(struct device *)			\
	_CONCAT(Z_PM_DEVICE_NAME(dev_name), slot) __used		\
	__attribute__((__section__(".z_pm_device_slots")))

#ifdef CONFIG_PM_DEVICE
/**
 * Define device PM resources for the given node identifier.
 *
 * @param node_id Node identifier (DT_INVALID_NODE if not a DT device).
 * @param dev_name Device name.
 * @param pm_action_cb PM control callback.
 */
#define Z_PM_DEVICE_DEFINE(node_id, dev_name, pm_action_cb)		\
	Z_PM_DEVICE_DEFINE_SLOT(dev_name);				\
	static struct pm_device Z_PM_DEVICE_NAME(dev_name) =		\
	Z_PM_DEVICE_INIT(Z_PM_DEVICE_NAME(dev_name), node_id,		\
			 pm_action_cb)

/**
 * Get a reference to the device PM resources.
 *
 * @param dev_name Device name.
 */
#define Z_PM_DEVICE_REF(dev_name) &Z_PM_DEVICE_NAME(dev_name)

#else
#define Z_PM_DEVICE_DEFINE(node_id, dev_name, pm_action_cb)
#define Z_PM_DEVICE_REF(dev_name) NULL
#endif /* CONFIG_PM_DEVICE */

/** @endcond */

/**
 * Define device PM resources for the given device name.
 *
 * @note This macro is a no-op if @kconfig{CONFIG_PM_DEVICE} is not enabled.
 *
 * @param dev_name Device name.
 * @param pm_action_cb PM control callback.
 *
 * @see #PM_DEVICE_DT_DEFINE, #PM_DEVICE_DT_INST_DEFINE
 */
#define PM_DEVICE_DEFINE(dev_name, pm_action_cb) \
	Z_PM_DEVICE_DEFINE(DT_INVALID_NODE, dev_name, pm_action_cb)

/**
 * Define device PM resources for the given node identifier.
 *
 * @note This macro is a no-op if @kconfig{CONFIG_PM_DEVICE} is not enabled.
 *
 * @param node_id Node identifier.
 * @param pm_action_cb PM control callback.
 *
 * @see #PM_DEVICE_DT_INST_DEFINE, #PM_DEVICE_DEFINE
 */
#define PM_DEVICE_DT_DEFINE(node_id, pm_action_cb)			\
	Z_PM_DEVICE_DEFINE(node_id, Z_DEVICE_DT_DEV_NAME(node_id),	\
			   pm_action_cb)

/**
 * Define device PM resources for the given instance.
 *
 * @note This macro is a no-op if @kconfig{CONFIG_PM_DEVICE} is not enabled.
 *
 * @param idx Instance index.
 * @param pm_action_cb PM control callback.
 *
 * @see #PM_DEVICE_DT_DEFINE, #PM_DEVICE_DEFINE
 */
#define PM_DEVICE_DT_INST_DEFINE(idx, pm_action_cb)			\
	Z_PM_DEVICE_DEFINE(DT_DRV_INST(idx),				\
			   Z_DEVICE_DT_DEV_NAME(DT_DRV_INST(idx)),	\
			   pm_action_cb)

/**
 * @brief Obtain a reference to the device PM resources for the given device.
 *
 * @param dev_name Device name.
 *
 * @return Reference to the device PM resources (NULL if device
 * @kconfig{CONFIG_PM_DEVICE} is disabled).
 */
#define PM_DEVICE_REF(dev_name) \
	Z_PM_DEVICE_REF(dev_name)

/**
 * @brief Obtain a reference to the device PM resources for the given node.
 *
 * @param node_id Node identifier.
 *
 * @return Reference to the device PM resources (NULL if device
 * @kconfig{CONFIG_PM_DEVICE} is disabled).
 */
#define PM_DEVICE_DT_REF(node_id) \
	PM_DEVICE_REF(Z_DEVICE_DT_DEV_NAME(node_id))

/**
 * @brief Obtain a reference to the device PM resources for the given instance.
 *
 * @param idx Instance index.
 *
 * @return Reference to the device PM resources (NULL if device
 * @kconfig{CONFIG_PM_DEVICE} is disabled).
 */
#define PM_DEVICE_DT_INST_REF(idx) \
	PM_DEVICE_DT_REF(DT_DRV_INST(idx))

/**
 * @brief Get name of device PM state
 *
 * @param state State id which name should be returned
 */
const char *pm_device_state_str(enum pm_device_state state);

/**
 * @brief Set the power state of a device.
 *
 * @deprecated Use pm_device_action_run() instead.
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
 * @retval -ENOSYS If device does not support PM.
 * @retval -EPERM If device has power state locked.
 * @retval Errno Other negative errno on failure.
 */
__deprecated int pm_device_state_set(const struct device *dev,
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
 * @retval Errno Other negative errno on failure.
 */
int pm_device_action_run(const struct device *dev,
		enum pm_device_action action);

#if defined(CONFIG_PM_DEVICE) || defined(__DOXYGEN__)
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
bool pm_device_wakeup_enable(struct device *dev, bool enable);

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
 * @brief Lock current device state.
 *
 * This function locks the current device power state. Once
 * locked the device power state will not be changed by
 * system power management or device runtime power
 * management until unlocked.
 *
 * @note The given device should not have device runtime enabled.
 *
 * @see pm_device_state_unlock
 *
 * @param dev Device instance.
 */
void pm_device_state_lock(const struct device *dev);

/**
 * @brief Unlock the current device state.
 *
 * Unlocks a previously locked device pm.
 *
 * @see pm_device_state_lock
 *
 * @param dev Device instance.
 */
void pm_device_state_unlock(const struct device *dev);

/**
 * @brief Check if the device pm is locked.
 *
 * @param dev Device instance.
 *
 * @retval true If device is locked.
 * @retval false If device is not locked.
 */
bool pm_device_state_is_locked(const struct device *dev);

#else
static inline void pm_device_busy_set(const struct device *dev) {}
static inline void pm_device_busy_clear(const struct device *dev) {}
static inline bool pm_device_is_any_busy(void) { return false; }
static inline bool pm_device_is_busy(const struct device *dev) { return false; }
static inline bool pm_device_wakeup_enable(struct device *dev, bool enable)
{
	return false;
}
static inline bool pm_device_wakeup_is_enabled(const struct device *dev)
{
	return false;
}
static inline bool pm_device_wakeup_is_capable(const struct device *dev)
{
	return false;
}
static inline void pm_device_state_lock(const struct device *dev) {}
static inline void pm_device_state_unlock(const struct device *dev) {}
static inline bool pm_device_state_is_locked(const struct device *dev)
{
	return false;
}
#endif /* CONFIG_PM_DEVICE */

/**
 * Mark a device as busy.
 *
 * @deprecated Use pm_device_busy_set() instead
 *
 * @param dev Device instance.
 */
__deprecated static inline void device_busy_set(const struct device *dev)
{
	pm_device_busy_set(dev);
}

/**
 * @brief Clear busy status of a device.
 *
 * @deprecated Use pm_device_busy_clear() instead
 *
 * @param dev Device instance.
 */
__deprecated static inline void device_busy_clear(const struct device *dev)
{
	pm_device_busy_clear(dev);
}

/**
 * @brief Check if any device is busy.
 *
 * @deprecated Use pm_device_is_any_busy() instead
 *
 * @retval 0 No devices are busy.
 * @retval -EBUSY One or more devices are busy.
 */
__deprecated static inline int device_any_busy_check(void)
{
	return pm_device_is_any_busy() ? -EBUSY : 0;
}

/**
 * @brief Check if a device is busy.
 *
 * @deprecated Use pm_device_is_busy() instead
 *
 * @param dev Device instance.
 *
 * @retval 0 Device is not busy.
 * @retval -EBUSY Device is busy.
 */
__deprecated static inline int device_busy_check(const struct device *dev)
{
	return pm_device_is_busy(dev) ? -EBUSY : 0;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif
