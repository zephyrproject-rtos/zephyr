
/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DEVICE_H_
#define _DEVICE_H_

/* for __deprecated */
#include <toolchain.h>

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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* XXX the easiest way to trigger a warning on a preprocessor macro
 * is to use _Pragma("GCC warning \"...\"), however it's impossible to filter
 * those out of -Werror, needed for sanitycheck. So we do this nastiness
 * instead. These functions get compiled but don't take up extra space in
 * the binary..
 */
static __deprecated const int _INIT_LEVEL_PRIMARY = 1;
static __deprecated const int _INIT_LEVEL_SECONDARY = 1;
static __deprecated const int _INIT_LEVEL_NANOKERNEL = 1;
static __deprecated const int _INIT_LEVEL_MICROKERNEL = 1;
static const int _INIT_LEVEL_PRE_KERNEL_1 = 1;
static const int _INIT_LEVEL_PRE_KERNEL_2 = 1;
static const int _INIT_LEVEL_POST_KERNEL = 1;
static const int _INIT_LEVEL_APPLICATION = 1;

#define _DEPRECATION_CHECK(dev_name, level) \
	static inline void _CONCAT(_deprecation_check_, dev_name)() \
	{ \
		int foo = _CONCAT(_INIT_LEVEL_, level); \
		(void)foo; \
	}

/**
 * @def DEVICE_INIT
 *
 * @brief Create device object and set it up for boot time initialization.
 *
 * @details This macro defines a device object that is automatically
 * configured by the kernel during system initialization.
 *
 * @param dev_name Device name.
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
 * initialized as part of the PRIMARY level. These devices cannot use any
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
#define DEVICE_AND_API_INIT(dev_name, drv_name, init_fn, data, cfg_info, \
			    level, prio, api) \
	\
	static struct device_config _CONCAT(__config_, dev_name) __used \
	__attribute__((__section__(".devconfig.init"))) = { \
		.name = drv_name, .init = (init_fn), \
		.config_info = (cfg_info) \
	}; \
	_DEPRECATION_CHECK(dev_name, level) \
	static struct device _CONCAT(__device_, dev_name) __used \
	__attribute__((__section__(".init_" #level STRINGIFY(prio)))) = { \
		 .config = &_CONCAT(__config_, dev_name), \
		 .driver_api = api, \
		 .driver_data = data \
	}

#define DEVICE_AND_API_INIT_PM(dev_name, drv_name, init_fn, device_pm_ops, \
			       data, cfg_info, level, prio, api) \
	DEVICE_AND_API_INIT(dev_name, drv_name, init_fn, data, cfg_info, \
			    level, prio, api)

#define DEVICE_DEFINE(dev_name, drv_name, init_fn, pm_control_fn, \
		      data, cfg_info, level, prio, api) \
	DEVICE_AND_API_INIT(dev_name, drv_name, init_fn, data, cfg_info, \
			    level, prio, api)
#else
/**
 * @def DEVICE_INIT_PM
 *
 * @warning This macro is deprecated and will be removed in
 *        a future version, superseded by DEVICE_DEFINE.
 *
 * @brief Create device object and set it up for boot time initialization,
 * with the option to device_pm_ops.
 *
 * @copydetails DEVICE_INIT
 * @param device_pm_ops Address to the device_pm_ops structure of the driver.
 */

/**
 * @def DEVICE_AND_API_INIT_PM
 *
 * @warning This macro is deprecated and will be removed in
 *        a future version, superseded by DEVICE_DEFINE.
 *
 * @brief Create device object and set it up for boot time initialization,
 * with the options to set driver_api and device_pm_ops.
 *
 * @copydetails DEVICE_INIT_PM
 * @param api Provides an initial pointer to the API function struct
 * used by the driver. Can be NULL.
 * @details The driver api is also set here, eliminating the need to do that
 * during initialization.
 */
#define DEVICE_AND_API_INIT_PM(dev_name, drv_name, init_fn, device_pm_ops, \
			       data, cfg_info, level, prio, api) \
	\
	static struct device_config _CONCAT(__config_, dev_name) __used \
	__attribute__((__section__(".devconfig.init"))) = { \
		.name = drv_name, .init = (init_fn), \
		.dev_pm_ops = (device_pm_ops), \
		.config_info = (cfg_info) \
	}; \
	_DEPRECATION_CHECK(dev_name, level) \
	static struct device _CONCAT(__device_, dev_name) __used \
	__attribute__((__section__(".init_" #level STRINGIFY(prio)))) = { \
		 .config = &_CONCAT(__config_, dev_name), \
		 .driver_api = api, \
		 .driver_data = data \
	}

/**
* @def DEVICE_DEFINE
*
* @brief Create device object and set it up for boot time initialization,
* with the option to device_pm_control.
*
* @copydetails DEVICE_AND_API_INIT
* @param pm_control_fn Pointer to device_pm_control function.
* Can be empty function (device_pm_control_nop) if not implemented.
*/
extern struct device_pm_ops device_pm_ops_nop;
#define DEVICE_DEFINE(dev_name, drv_name, init_fn, pm_control_fn, \
		      data, cfg_info, level, prio, api) \
	\
	static struct device_config _CONCAT(__config_, dev_name) __used \
	__attribute__((__section__(".devconfig.init"))) = { \
		.name = drv_name, .init = (init_fn), \
		.device_pm_control = (pm_control_fn), \
		.dev_pm_ops = (&device_pm_ops_nop), \
		.config_info = (cfg_info) \
	}; \
	_DEPRECATION_CHECK(dev_name, level) \
	static struct device _CONCAT(__device_, dev_name) __used \
	__attribute__((__section__(".init_" #level STRINGIFY(prio)))) = { \
		 .config = &_CONCAT(__config_, dev_name), \
		 .driver_api = api, \
		 .driver_data = data \
	}
/*
 * Use the default device_pm_control for devices that do not call the
 * DEVICE_DEFINE macro so that caller of hook functions
 * need not check device_pm_control != NULL.
 */
#define DEVICE_AND_API_INIT(dev_name, drv_name, init_fn, data, cfg_info, \
			    level, prio, api) \
	DEVICE_DEFINE(dev_name, drv_name, init_fn, \
		      device_pm_control_nop, data, cfg_info, level, \
		      prio, api)
#endif

/* deprecated */
#define DEVICE_INIT_PM(dev_name, drv_name, init_fn, device_pm_ops, \
			       data, cfg_info, level, prio) \
	DEVICE_AND_API_INIT_PM(dev_name, drv_name, init_fn, device_pm_ops, \
			       data, cfg_info, level, prio, NULL)

#define DEVICE_INIT(dev_name, drv_name, init_fn, data, cfg_info, level, prio) \
	DEVICE_AND_API_INIT(dev_name, drv_name, init_fn, data, cfg_info, \
			    level, prio, NULL)

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
 * @return The exanded name of the device object created by DEVICE_INIT()
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
  * @brief Declare a device object
  *
  * This macro can be used at the top-level to declare a device, such
  * that DEVICE_GET() may be used before the full declaration in
  * DEVICE_INIT(), or reference the device in another C file.
  *
  * This is often useful when configuring interrupts statically in a
  * device's init or per-instance config function, as the init function
  * itself is required by DEVICE_INIT() and use of DEVICE_GET()
  * inside it creates a circular dependeny.
  *
  * @param name Device name
  */
#define DEVICE_DECLARE(name) extern struct device DEVICE_NAME_GET(name)

struct device;

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
/**
 * @brief Device Power Management APIs
 * @defgroup device_power_management_api Device Power Management APIs
 * @ingroup power_management_api
 * @{
 */
/**
 * @brief Structure holding handlers for device PM operations
 *
 * @warning This struct is deprecated and will be removed in
 *        a future version.
 *
 * @param suspend Pointer to the handler for suspend operations
 * @param resume Pointer to the handler for resume operations
 */
struct device_pm_ops {
	int (*suspend)(struct device *device, int pm_policy);
	int (*resume)(struct device *device, int pm_policy);
};

/**
 * @brief Helper macro to define the device_pm_ops structure
 *
 * @param _name name of the device
 * @param _suspend name of the suspend function
 * @param _resume name of the resume function
 */
#define DEFINE_DEVICE_PM_OPS(_name, _suspend, _resume)	\
	static struct device_pm_ops _CONCAT(_name, _dev_pm_ops) = { \
		.suspend = _suspend,			\
		.resume = _resume,			\
	}

/**
 * @brief Macro to get a pointer to the device_ops_structure
 *
 * Will return the name of the structure that was created using
 * DEFINE_PM_OPS macro if CONFIG_DEVICE_POWER_MANAGEMENT is defined.
 * Otherwise, will return NULL.
 *
 * @param _name name of the device
 */
#define DEVICE_PM_OPS_GET(_name) &_CONCAT(_name, _dev_pm_ops)

/**
 * @}
 */

/** @def DEVICE_PM_ACTIVE_STATE
 *
 * @brief device is in ACTIVE power state
 *
 * @details Normal operation of the device. All device context is retained.
 */
#define DEVICE_PM_ACTIVE_STATE		1

/** @def DEVICE_PM_LOW_POWER_STATE
 *
 * @brief device is in LOW power state
 *
 * @details Device context is preserved by the HW and need not be
 * restored by the driver.
 */
#define DEVICE_PM_LOW_POWER_STATE	2

/** @def DEVICE_PM_SUSPEND_STATE
 *
 * @brief device is in SUSPEND power state
 *
 * @details Most device context is lost by the hardware.
 * Device drivers must save and restore or reinitialize any context
 * lost by the hardware
 */
#define DEVICE_PM_SUSPEND_STATE		3

/** @def DEVICE_PM_OFF_STATE
 *
 * @brief device is in OFF power state
 *
 * @details - Power has been fully removed from the device.
 * The device context is lost when this state is entered, so the OS
 * software will reinitialize the device when powering it back on
 */
#define DEVICE_PM_OFF_STATE		4

/* Constants defining support device power commands */
#define DEVICE_PM_SET_POWER_STATE	1
#define DEVICE_PM_GET_POWER_STATE	2
#else
#define DEFINE_DEVICE_PM_OPS(_name, _suspend, _resume)
#define DEVICE_PM_OPS_GET(_name) NULL
#endif

/**
 * @brief Static device information (In ROM) Per driver instance
 *
 * @note  This struct contains deprecated struct (device_pm_ops)
 *  that will be removed in a future version.
 *
 * @param name name of the device
 * @param init init function for the driver
 * @param config_info address of driver instance config information
 */
struct device_config {
	char	*name;
	int (*init)(struct device *device);
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	struct device_pm_ops *dev_pm_ops; /* deprecated */
	int (*device_pm_control)(struct device *device, uint32_t command,
			      void *context);
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

void _sys_device_do_config_level(int level);

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
struct device *device_get_binding(const char *name);

/**
 * @brief Device Power Management APIs
 * @defgroup device_power_management_api Device Power Management APIs
 * @ingroup power_management_api
 * @{
 */

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
 * @brief No-op function to initialize unimplemented pm hooks
 *
 * This function should be used to initialize device pm hooks
 * for which a device has no operation.
 *
 * @param unused_device Unused
 * @param unused_policy Unused
 *
 * @retval 0 Always returns 0
 */
int device_pm_nop(struct device *unused_device, int unused_policy);

/**
 * @fn static inline int device_suspend(struct device *device, int pm_policy)
 *
 * @brief Call the suspend function of a device
 *
 *
 * @warning This function is deprecated and will be removed in
 *        a future version, use device_set_power_state instead.
 *
 * Called by the Power Manager application to let the device do
 * any policy based PM suspend operations.
 *
 * @param device Pointer to device structure of the driver instance.
 * @param pm_policy PM policy for which this call is made.
 *
 * @retval 0 If successful.
 * @retval Errno Negative errno code if failure.
 */
static inline int __deprecated device_suspend(struct device *device,
						int pm_policy)
{
	return device->config->dev_pm_ops->suspend(device, pm_policy);
}

/**
 * @fn static inline int device_resume(struct device *device, int pm_policy)
 *
 * @brief Call the resume function of a device
 *
 * @warning This function is deprecated and will be removed in
 *        a future version, use device_set_power_state instead.
 *
 * Called by the Power Manager application to let the device do
 * any policy based PM resume operations.
 *
 * @param device Pointer to device structure of the driver instance.
 * @param pm_policy PM policy for which this call is made.
 *
 * @retval 0 If successful.
 * @retval Errno Negative errno code if failure.
 */
static inline int __deprecated device_resume(struct device *device,
						int pm_policy)
{
	return device->config->dev_pm_ops->resume(device, pm_policy);
}

/**
 * @brief No-op function to initialize unimplemented hook
 *
 * This function should be used to initialize device hook
 * for which a device has no PM operations.
 *
 * @param unused_device Unused
 * @param unused_ctrl_command Unused
 * @param unused_context Unused
 *
 * @retval 0 Always returns 0
 */
int device_pm_control_nop(struct device *unused_device,
		       uint32_t unused_ctrl_command, void *unused_context);
/**
 * @brief Call the set power state function of a device
 *
 * Called by the application or power management service to let the device do
 * required operations when moving to the required power state
 * Note that devices may support just some of the device power states
 * @param device Pointer to device structure of the driver instance.
 * @param device_power_state Device power state to be set
 *
 * @retval 0 If successful.
 * @retval Errno Negative errno code if failure.
 */
static inline int device_set_power_state(struct device *device,
					 uint32_t device_power_state)
{
	return device->config->device_pm_control(device,
			DEVICE_PM_SET_POWER_STATE, &device_power_state);
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
					 uint32_t *device_power_state)
{
	return device->config->device_pm_control(device,
				DEVICE_PM_GET_POWER_STATE, device_power_state);
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

#endif

/**
 * @}
 */

/**
 * Synchronous calls API
 */

#include <kernel.h>
#include <stdbool.h>

/**
 * Specific type for synchronizing calls among the 2 possible contexts
 */
typedef struct {
	/** Nanokernel semaphore used for fiber context */
	struct k_sem f_sem;
} device_sync_call_t;


/**
 * @brief Initialize the context-dependent synchronization data
 *
 * @param sync A pointer to a valid device_sync_call_t
 */
static inline void __deprecated device_sync_call_init(device_sync_call_t *sync)
{
	k_sem_init(&sync->f_sem, 0, UINT_MAX);
}

/**
 * @brief Wait for the isr to complete the synchronous call
 * Note: It will simply wait on the internal semaphore.
 *
 * @param sync A pointer to a valid device_sync_call_t
 */
static inline void __deprecated device_sync_call_wait(device_sync_call_t *sync)
{
	k_sem_take(&sync->f_sem, K_FOREVER);
}

/**
 * @brief Signal the waiter about synchronization completion
 * Note: It will simply release the internal semaphore
 *
 * @param sync A pointer to a valid device_sync_call_t
 */
static inline void __deprecated
		   device_sync_call_complete(device_sync_call_t *sync)
{
	k_sem_give(&sync->f_sem);
}

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif /* _DEVICE_H_ */
