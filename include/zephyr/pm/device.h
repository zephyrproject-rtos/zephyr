/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PM_DEVICE_H_
#define ZEPHYR_INCLUDE_PM_DEVICE_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

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
	/** Indicate if the device failed to power up. */
	PM_DEVICE_FLAG_TURN_ON_FAILED,
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
	/** Indicateds if the device is used as a power domain */
	PM_DEVICE_FLAG_PD,
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
	/**
	 * Turn off.
	 * @note
	 *     Action triggered only by a power domain.
	 */
	PM_DEVICE_ACTION_TURN_OFF,
	/**
	 * Turn on.
	 * @note
	 *     Action triggered only by a power domain.
	 */
	PM_DEVICE_ACTION_TURN_ON,
};

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
 * @brief Device PM action failed callback
 *
 * @param dev Device that failed the action.
 * @param err Return code of action failure.
 *
 * @return True to continue iteration, false to halt iteration.
 */
typedef bool (*pm_device_action_failed_cb_t)(const struct device *dev,
					 int err);

/**
 * @brief Device PM info
 */
struct pm_device {
#if defined(CONFIG_PM_DEVICE_RUNTIME) || defined(__DOXYGEN__)
	/** Pointer to the device */
	const struct device *dev;
	/** Lock to synchronize the get/put operations */
	struct k_sem lock;
	/** Event var to listen to the sync request events */
	struct k_event event;
	/** Device usage count */
	uint32_t usage;
	/** Work object for asynchronous calls */
	struct k_work_delayable work;
#endif /* CONFIG_PM_DEVICE_RUNTIME */
#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN
	/** Power Domain it belongs */
	const struct device *domain;
#endif /* CONFIG_PM_DEVICE_POWER_DOMAIN */
	/* Device PM status flags. */
	atomic_t flags;
	/** Device power state */
	enum pm_device_state state;
	/** Device PM action callback */
	pm_device_action_cb_t action_cb;
};

/** @cond INTERNAL_HIDDEN */

#ifdef CONFIG_PM_DEVICE_RUNTIME
#define Z_PM_DEVICE_RUNTIME_INIT(obj)			\
	.lock = Z_SEM_INITIALIZER(obj.lock, 1, 1),	\
	.event = Z_EVENT_INITIALIZER(obj.event),
#else
#define Z_PM_DEVICE_RUNTIME_INIT(obj)
#endif /* CONFIG_PM_DEVICE_RUNTIME */

#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN
#define	Z_PM_DEVICE_POWER_DOMAIN_INIT(_node_id)			\
	.domain = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(_node_id,	\
				   power_domain)),
#else
#define Z_PM_DEVICE_POWER_DOMAIN_INIT(obj)
#endif /* CONFIG_PM_DEVICE_POWER_DOMAIN */

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
				((DT_PROP_OR(node_id, wakeup_source, 0) \
				  << PM_DEVICE_FLAG_WS_CAPABLE) |	\
				 (DT_NODE_HAS_COMPAT(node_id, power_domain) << \
				  PM_DEVICE_FLAG_PD)), (0))),		\
		Z_PM_DEVICE_POWER_DOMAIN_INIT(node_id)			\
	}

/**
 * Get the name of device PM resources.
 *
 * @param dev_id Device id.
 */
#define Z_PM_DEVICE_NAME(dev_id) _CONCAT(__pm_device_, dev_id)

/**
 * @brief Define device PM slot.
 *
 * This macro defines a pointer to a device in the z_pm_device_slots region.
 * When invoked for each device with PM, it will effectively result in a device
 * pointer array with the same size of the actual devices with PM enabled. This
 * is used internally by the PM subsystem to keep track of suspended devices
 * during system power transitions.
 *
 * @param dev_id Device id.
 */
#define Z_PM_DEVICE_DEFINE_SLOT(dev_id)					\
	static const Z_DECL_ALIGN(struct device *)			\
	_CONCAT(__pm_slot_, dev_id) __used				\
	__attribute__((__section__(".z_pm_device_slots")))

#ifdef CONFIG_PM_DEVICE
/**
 * Define device PM resources for the given node identifier.
 *
 * @param node_id Node identifier (DT_INVALID_NODE if not a DT device).
 * @param dev_id Device id.
 * @param pm_action_cb PM control callback.
 */
#define Z_PM_DEVICE_DEFINE(node_id, dev_id, pm_action_cb)		\
	Z_PM_DEVICE_DEFINE_SLOT(dev_id);				\
	static struct pm_device Z_PM_DEVICE_NAME(dev_id) =		\
	Z_PM_DEVICE_INIT(Z_PM_DEVICE_NAME(dev_id), node_id,		\
			 pm_action_cb)

/**
 * Get a reference to the device PM resources.
 *
 * @param dev_id Device id.
 */
#define Z_PM_DEVICE_GET(dev_id) (&Z_PM_DEVICE_NAME(dev_id))

#else
#define Z_PM_DEVICE_DEFINE(node_id, dev_id, pm_action_cb)
#define Z_PM_DEVICE_GET(dev_id) NULL
#endif /* CONFIG_PM_DEVICE */

/** @endcond */

/**
 * Define device PM resources for the given device name.
 *
 * @note This macro is a no-op if @kconfig{CONFIG_PM_DEVICE} is not enabled.
 *
 * @param dev_id Device id.
 * @param pm_action_cb PM control callback.
 *
 * @see #PM_DEVICE_DT_DEFINE, #PM_DEVICE_DT_INST_DEFINE
 */
#define PM_DEVICE_DEFINE(dev_id, pm_action_cb) \
	Z_PM_DEVICE_DEFINE(DT_INVALID_NODE, dev_id, pm_action_cb)

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
	Z_PM_DEVICE_DEFINE(node_id, Z_DEVICE_DT_DEV_ID(node_id),	\
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
			   Z_DEVICE_DT_DEV_ID(DT_DRV_INST(idx)),	\
			   pm_action_cb)

/**
 * @brief Obtain a reference to the device PM resources for the given device.
 *
 * @param dev_id Device id.
 *
 * @return Reference to the device PM resources (NULL if device
 * @kconfig{CONFIG_PM_DEVICE} is disabled).
 */
#define PM_DEVICE_GET(dev_id) \
	Z_PM_DEVICE_GET(dev_id)

/**
 * @brief Obtain a reference to the device PM resources for the given node.
 *
 * @param node_id Node identifier.
 *
 * @return Reference to the device PM resources (NULL if device
 * @kconfig{CONFIG_PM_DEVICE} is disabled).
 */
#define PM_DEVICE_DT_GET(node_id) \
	PM_DEVICE_GET(Z_DEVICE_DT_DEV_ID(node_id))

/**
 * @brief Obtain a reference to the device PM resources for the given instance.
 *
 * @param idx Instance index.
 *
 * @return Reference to the device PM resources (NULL if device
 * @kconfig{CONFIG_PM_DEVICE} is disabled).
 */
#define PM_DEVICE_DT_INST_GET(idx) \
	PM_DEVICE_DT_GET(DT_DRV_INST(idx))

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

#if defined(CONFIG_PM_DEVICE) || defined(__DOXYGEN__)
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
	struct pm_device *pm = dev->pm;

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
	struct pm_device *pm = dev->pm;

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
 * @retval true If device is currently powered
 * @retval false If device is not currently powered
 */
bool pm_device_is_powered(const struct device *dev);
#else
static inline int pm_device_state_get(const struct device *dev,
				      enum pm_device_state *state)
{
	ARG_UNUSED(dev);

	*state = PM_DEVICE_STATE_ACTIVE;

	return 0;
}

static inline void pm_device_init_suspended(const struct device *dev)
{
	ARG_UNUSED(dev);
}
static inline void pm_device_init_off(const struct device *dev)
{
	ARG_UNUSED(dev);
}
static inline void pm_device_busy_set(const struct device *dev)
{
	ARG_UNUSED(dev);
}
static inline void pm_device_busy_clear(const struct device *dev)
{
	ARG_UNUSED(dev);
}
static inline bool pm_device_is_any_busy(void) { return false; }
static inline bool pm_device_is_busy(const struct device *dev)
{
	ARG_UNUSED(dev);
	return false;
}
static inline bool pm_device_wakeup_enable(const struct device *dev,
					   bool enable)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(enable);
	return false;
}
static inline bool pm_device_wakeup_is_enabled(const struct device *dev)
{
	ARG_UNUSED(dev);
	return false;
}
static inline bool pm_device_wakeup_is_capable(const struct device *dev)
{
	ARG_UNUSED(dev);
	return false;
}
static inline void pm_device_state_lock(const struct device *dev)
{
	ARG_UNUSED(dev);
}
static inline void pm_device_state_unlock(const struct device *dev)
{
	ARG_UNUSED(dev);
}
static inline bool pm_device_state_is_locked(const struct device *dev)
{
	ARG_UNUSED(dev);
	return false;
}
static inline bool pm_device_on_power_domain(const struct device *dev)
{
	ARG_UNUSED(dev);
	return false;
}

static inline int pm_device_power_domain_add(const struct device *dev,
					     const struct device *domain)
{
	return -ENOSYS;
}

static inline int pm_device_power_domain_remove(const struct device *dev,
						const struct device *domain)
{
	return -ENOSYS;
}

static inline bool pm_device_is_powered(const struct device *dev)
{
	ARG_UNUSED(dev);
	return true;
}
#endif /* CONFIG_PM_DEVICE */

/** @} */

#ifdef __cplusplus
}
#endif

#endif
