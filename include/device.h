
/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICE_H_
#define ZEPHYR_INCLUDE_DEVICE_H_

#include <kernel.h>

/**
 * @brief Device Driver APIs
 * @defgroup io_interfaces Device Driver APIs
 * @{
 * @}
 */
/**
 * @brief Device Model APIs
 * @defgroup device_model Device Model APIs
 * @{
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define Z_DEVICE_MAX_NAME_LEN	48

/**
 * @def DEVICE_INIT
 *
 * @brief Create device object and set it up for boot time initialization.
 *
 * @details This macro defines a device object that is automatically
 * configured by the kernel during system initialization. Note that
 * devices set up with this macro will not be accessible from user mode
 * since the API is not specified; whenever possible, use DEVICE_AND_API_INIT
 * instead.
 *
 * @param dev_name Device name. This must be less than Z_DEVICE_MAX_NAME_LEN
 * characters in order to be looked up from user mode with device_get_binding().
 *
 * @param drv_name The name this instance of the driver exposes to
 * the system.
 *
 * @param init_fn Address to the init function of the driver.
 *
 * @param data Pointer to the device's configuration data.
 *
 * @param cfg_info The address to the structure containing the
 * configuration information for this instance of the driver.
 *
 * @param level The initialization level at which configuration occurs.
 * Must be one of the following symbols, which are listed in the order
 * they are performed by the kernel:
 * \n
 * \li PRE_KERNEL_1: Used for devices that have no dependencies, such as those
 * that rely solely on hardware present in the processor/SOC. These devices
 * cannot use any kernel services during configuration, since they are not
 * yet available.
 * \n
 * \li PRE_KERNEL_2: Used for devices that rely on the initialization of devices
 * initialized as part of the PRE_KERNEL_1 level. These devices cannot use any
 * kernel services during configuration, since they are not yet available.
 * \n
 * \li POST_KERNEL: Used for devices that require kernel services during
 * configuration.
 * \n
 * \li APPLICATION: Used for application components (i.e. non-kernel components)
 * that need automatic configuration. These devices can use all services
 * provided by the kernel during configuration.
 *
 * @param prio The initialization priority of the device, relative to
 * other devices of the same initialization level. Specified as an integer
 * value in the range 0 to 99; lower values indicate earlier initialization.
 * Must be a decimal integer literal without leading zeroes or sign (e.g. 32),
 * or an equivalent symbolic name (e.g. \#define MY_INIT_PRIO 32); symbolic
 * expressions are *not* permitted
 * (e.g. CONFIG_KERNEL_INIT_PRIORITY_DEFAULT + 5).
 */
#define DEVICE_INIT(dev_name, drv_name, init_fn, data, cfg_info, level, prio) \
	DEVICE_AND_API_INIT(dev_name, drv_name, init_fn,\
	data, cfg_info, level, prio, NULL)


/**
 * @def DEVICE_AND_API_INIT
 *
 * @brief Create device object and set it up for boot time initialization,
 * with the option to set driver_api.
 *
 * @copydetails DEVICE_INIT
 * @param api Provides an initial pointer to the API function struct
 * used by the driver. Can be NULL.
 * @details The driver api is also set here, eliminating the need to do that
 * during initialization.
 */
#ifndef CONFIG_DEVICE_POWER_MANAGEMENT
#define DEVICE_AND_API_INIT(dev_name, drv_name, init_fn, data, cfg_info,  \
			    level, prio, api)				  \
	static struct device_config _CONCAT(__config_, dev_name) __used	  \
	__attribute__((__section__(".devconfig.init"))) = {		  \
		.name = drv_name, .init = (init_fn),			  \
		.config_info = (cfg_info)				  \
	};								  \
	static Z_DECL_ALIGN(struct device) _CONCAT(__device_, dev_name) __used \
	__attribute__((__section__(".init_" #level STRINGIFY(prio)))) = { \
		.config = &_CONCAT(__config_, dev_name),		  \
		.driver_api = api,					  \
		.driver_data = data					  \
	}
#else
/*
 * Use the default device_pm_control for devices that do not call the
 * DEVICE_DEFINE macro so that caller of hook functions
 * need not check device_pm_control != NULL.
 */
#define DEVICE_AND_API_INIT(dev_name, drv_name, init_fn, data, cfg_info, \
			    level, prio, api)				 \
	DEVICE_DEFINE(dev_name, drv_name, init_fn,			 \
		      device_pm_control_nop, data, cfg_info, level,	 \
		      prio, api)
#endif

/**
 * @def DEVICE_DEFINE
 *
 * @brief Create device object and set it up for boot time initialization,
 * with the option to device_pm_control. In case of Device Idle Power
 * Management is enabled, make sure the device is in suspended state after
 * initialization.
 *
 * @copydetails DEVICE_AND_API_INIT
 * @param pm_control_fn Pointer to device_pm_control function.
 * Can be empty function (device_pm_control_nop) if not implemented.
 */
#ifndef CONFIG_DEVICE_POWER_MANAGEMENT
#define DEVICE_DEFINE(dev_name, drv_name, init_fn, pm_control_fn,	 \
		      data, cfg_info, level, prio, api)			 \
	DEVICE_AND_API_INIT(dev_name, drv_name, init_fn, data, cfg_info, \
			    level, prio, api)
#else
#define DEVICE_DEFINE(dev_name, drv_name, init_fn, pm_control_fn,	  \
		      data, cfg_info, level, prio, api)			  \
	static struct device_pm _CONCAT(__pm_, dev_name) __used           \
							= {               \
		.usage = ATOMIC_INIT(0),                                  \
		.lock = Z_SEM_INITIALIZER(                               \
				_CONCAT(__pm_, dev_name).lock, 1, 1),     \
		.signal = K_POLL_SIGNAL_INITIALIZER(                      \
				_CONCAT(__pm_, dev_name).signal),         \
		.event = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,     \
				K_POLL_MODE_NOTIFY_ONLY,                  \
				&_CONCAT(__pm_, dev_name).signal),        \
	};								  \
	static struct device_config _CONCAT(__config_, dev_name) __used	  \
	__attribute__((__section__(".devconfig.init"))) = {		  \
		.name = drv_name, .init = (init_fn),			  \
		.device_pm_control = (pm_control_fn),			  \
		.pm  = &_CONCAT(__pm_, dev_name),                         \
		.config_info = (cfg_info)				  \
	};								  \
	static Z_DECL_ALIGN(struct device) _CONCAT(__device_, dev_name) __used \
	__attribute__((__section__(".init_" #level STRINGIFY(prio)))) = { \
		.config = &_CONCAT(__config_, dev_name),		  \
		.driver_api = api,					  \
		.driver_data = data,					  \
	}
#endif

/**
 * @def DEVICE_NAME_GET
 *
 * @brief Expands to the full name of a global device object
 *
 * @details Return the full name of a device object symbol created by
 * DEVICE_INIT(), using the dev_name provided to DEVICE_INIT().
 *
 * It is meant to be used for declaring extern symbols pointing on device
 * objects before using the DEVICE_GET macro to get the device object.
 *
 * @param name The same as dev_name provided to DEVICE_INIT()
 *
 * @return The expanded name of the device object created by DEVICE_INIT()
 */
#define DEVICE_NAME_GET(name) (_CONCAT(__device_, name))

/**
 * @def DEVICE_GET
 *
 * @brief Obtain a pointer to a device object by name
 *
 * @details Return the address of a device object created by
 * DEVICE_INIT(), using the dev_name provided to DEVICE_INIT().
 *
 * @param name The same as dev_name provided to DEVICE_INIT()
 *
 * @return A pointer to the device object created by DEVICE_INIT()
 */
#define DEVICE_GET(name) (&DEVICE_NAME_GET(name))

/** @def DEVICE_DECLARE
 *
 * @brief Declare a static device object
 *
 * This macro can be used at the top-level to declare a device, such
 * that DEVICE_GET() may be used before the full declaration in
 * DEVICE_INIT().
 *
 * This is often useful when configuring interrupts statically in a
 * device's init or per-instance config function, as the init function
 * itself is required by DEVICE_INIT() and use of DEVICE_GET()
 * inside it creates a circular dependency.
 *
 * @param name Device name
 */
#define DEVICE_DECLARE(name) static struct device DEVICE_NAME_GET(name)

struct device;

typedef void (*device_pm_cb)(struct device *dev,
			     int status, void *context, void *arg);

/**
 * @brief Device PM info
 *
 * @param dev pointer to device structure
 * @param lock lock to synchronize the get/put operations
 * @param enable device pm enable flag
 * @param usage device usage count
 * @param fsm_state device idle internal power state
 * @param event event object to listen to the sync request events
 * @param signal signal to notify the Async API callers
 */
struct device_pm {
	struct device *dev;
	struct k_sem lock;
	bool enable;
	atomic_t usage;
	atomic_t fsm_state;
	struct k_work work;
	struct k_poll_event event;
	struct k_poll_signal signal;
};

/**
 * @brief Static device information (In ROM) Per driver instance
 *
 * @param name name of the device
 * @param init init function for the driver
 * @param config_info address of driver instance config information
 */
struct device_config {
	const char *name;
	int (*init)(struct device *device);
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	int (*device_pm_control)(struct device *device, u32_t command,
				 void *context, device_pm_cb cb, void *arg);
	struct device_pm *pm;
#endif
	const void *config_info;
};

/**
 * @brief Runtime device structure (In memory) Per driver instance
 * @param device_config Build time config information
 * @param driver_api pointer to structure containing the API functions for
 * the device type. This pointer is filled in by the driver at init time.
 * @param driver_data driver instance data. For driver use only
 */
struct device {
	struct device_config *config;
	const void *driver_api;
	void *driver_data;
};

void z_sys_device_do_config_level(s32_t level);

/**
 * @brief Retrieve the device structure for a driver by name
 *
 * @details Device objects are created via the DEVICE_INIT() macro and
 * placed in memory by the linker. If a driver needs to bind to another driver
 * it can use this function to retrieve the device structure of the lower level
 * driver by the name the driver exposes to the system.
 *
 * @param name device name to search for.
 *
 * @return pointer to device structure; NULL if not found or cannot be used.
 */
__syscall struct device *device_get_binding(const char *name);

/**
 * @}
 */

/**
 * @brief Device Power Management APIs
 * @defgroup device_power_management_api Device Power Management APIs
 * @ingroup power_management_api
 * @{
 */

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT

/** @def DEVICE_PM_ACTIVE_STATE
 *
 * @brief device is in ACTIVE power state
 *
 * @details Normal operation of the device. All device context is retained.
 */
#define DEVICE_PM_ACTIVE_STATE          1

/** @def DEVICE_PM_LOW_POWER_STATE
 *
 * @brief device is in LOW power state
 *
 * @details Device context is preserved by the HW and need not be
 * restored by the driver.
 */
#define DEVICE_PM_LOW_POWER_STATE       2

/** @def DEVICE_PM_SUSPEND_STATE
 *
 * @brief device is in SUSPEND power state
 *
 * @details Most device context is lost by the hardware.
 * Device drivers must save and restore or reinitialize any context
 * lost by the hardware
 */
#define DEVICE_PM_SUSPEND_STATE         3

/** @def DEVICE_PM_FORCE_SUSPEND_STATE
 *
 * @brief device is in force SUSPEND power state
 *
 * @details Driver puts the device in suspended state after
 * completing the ongoing transactions and will not process any
 * queued work or will not take any new requests for processing.
 * Most device context is lost by the hardware. Device drivers must
 * save and restore or reinitialize any context lost by the hardware.
 */
#define DEVICE_PM_FORCE_SUSPEND_STATE	4

/** @def DEVICE_PM_OFF_STATE
 *
 * @brief device is in OFF power state
 *
 * @details - Power has been fully removed from the device.
 * The device context is lost when this state is entered, so the OS
 * software will reinitialize the device when powering it back on
 */
#define DEVICE_PM_OFF_STATE             5

/* Constants defining support device power commands */
#define DEVICE_PM_SET_POWER_STATE       1
#define DEVICE_PM_GET_POWER_STATE       2

#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

/**
 * @brief Indicate that the device is in the middle of a transaction
 *
 * Called by a device driver to indicate that it is in the middle of a
 * transaction.
 *
 * @param busy_dev Pointer to device structure of the driver instance.
 */
void device_busy_set(struct device *busy_dev);

/**
 * @brief Indicate that the device has completed its transaction
 *
 * Called by a device driver to indicate the end of a transaction.
 *
 * @param busy_dev Pointer to device structure of the driver instance.
 */
void device_busy_clear(struct device *busy_dev);

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
/*
 * Device PM functions
 */

/**
 * @brief No-op function to initialize unimplemented hook
 *
 * This function should be used to initialize device hook
 * for which a device has no PM operations.
 *
 * @param unused_device Unused
 * @param unused_ctrl_command Unused
 * @param unused_context Unused
 * @param cb Unused
 * @param unused_arg Unused
 *
 * @retval 0 Always returns 0
 */
int device_pm_control_nop(struct device *unused_device,
			  u32_t unused_ctrl_command,
			  void *unused_context,
			  device_pm_cb cb,
			  void *unused_arg);
/**
 * @brief Call the set power state function of a device
 *
 * Called by the application or power management service to let the device do
 * required operations when moving to the required power state
 * Note that devices may support just some of the device power states
 * @param device Pointer to device structure of the driver instance.
 * @param device_power_state Device power state to be set
 * @param cb Callback function to notify device power status
 * @param arg Caller passed argument to callback function
 *
 * @retval 0 If successful in queuing the request or changing the state.
 * @retval Errno Negative errno code if failure. Callback will not be called.
 */
static inline int device_set_power_state(struct device *device,
					 u32_t device_power_state,
					 device_pm_cb cb, void *arg)
{
	return device->config->device_pm_control(device,
						 DEVICE_PM_SET_POWER_STATE,
						 &device_power_state, cb, arg);
}

/**
 * @brief Call the get power state function of a device
 *
 * This function lets the caller know the current device
 * power state at any time. This state will be one of the defined
 * power states allowed for the devices in that system
 *
 * @param device pointer to device structure of the driver instance.
 * @param device_power_state Device power state to be filled by the device
 *
 * @retval 0 If successful.
 * @retval Errno Negative errno code if failure.
 */
static inline int device_get_power_state(struct device *device,
					 u32_t *device_power_state)
{
	return device->config->device_pm_control(device,
						 DEVICE_PM_GET_POWER_STATE,
						 device_power_state,
						 NULL, NULL);
}

/**
 * @brief Gets the device structure list array and device count
 *
 * Called by the Power Manager application to get the list of
 * device structures associated with the devices in the system.
 * The PM app would use this list to create its own sorted list
 * based on the order it wishes to suspend or resume the devices.
 *
 * @param device_list Pointer to receive the device list array
 * @param device_count Pointer to receive the device count
 */
void device_list_get(struct device **device_list, int *device_count);

/**
 * @brief Check if any device is in the middle of a transaction
 *
 * Called by an application to see if any device is in the middle
 * of a critical transaction that cannot be interrupted.
 *
 * @retval 0 if no device is busy
 * @retval -EBUSY if any device is busy
 */
int device_any_busy_check(void);

/**
 * @brief Check if a specific device is in the middle of a transaction
 *
 * Called by an application to see if a particular device is in the
 * middle of a critical transaction that cannot be interrupted.
 *
 * @param chk_dev Pointer to device structure of the specific device driver
 * the caller is interested in.
 * @retval 0 if the device is not busy
 * @retval -EBUSY if the device is busy
 */
int device_busy_check(struct device *chk_dev);

#ifdef CONFIG_DEVICE_IDLE_PM

/* Device PM FSM states */
enum device_pm_fsm_state {
	DEVICE_PM_FSM_STATE_ACTIVE = 1,
	DEVICE_PM_FSM_STATE_SUSPENDED,
	DEVICE_PM_FSM_STATE_SUSPENDING,
	DEVICE_PM_FSM_STATE_RESUMING,
};

/**
 * @brief Enable device idle PM
 *
 * Called by a device driver to enable device idle power management.
 * The device might be asynchronously suspended if Idle PM is enabled
 * when the device is not use.
 *
 * @param dev Pointer to device structure of the specific device driver
 * the caller is interested in.
 */
void device_pm_enable(struct device *dev);

/**
 * @brief Disable device idle PM
 *
 * Called by a device driver to disable device idle power management.
 * The device might be asynchronously resumed if Idle PM is disabled
 *
 * @param dev Pointer to device structure of the specific device driver
 * the caller is interested in.
 */
void device_pm_disable(struct device *dev);

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
int device_pm_get(struct device *dev);

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
int device_pm_get_sync(struct device *dev);

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
int device_pm_put(struct device *dev);

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
int device_pm_put_sync(struct device *dev);
#else
static inline void device_pm_enable(struct device *dev) { }
static inline void device_pm_disable(struct device *dev) { }
static inline int device_pm_get(struct device *dev) { return -ENOTSUP; }
static inline int device_pm_get_sync(struct device *dev) { return -ENOTSUP; }
static inline int device_pm_put(struct device *dev) { return -ENOTSUP; }
static inline int device_pm_put_sync(struct device *dev) { return -ENOTSUP; }
#endif

#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/device.h>

#endif /* ZEPHYR_INCLUDE_DEVICE_H_ */
