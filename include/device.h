
/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <errno.h>

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
#ifdef __cplusplus
extern "C" {
#endif

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
 * \li PRIMARY: Used for devices that have no dependencies, such as those
 * that rely solely on hardware present in the processor/SOC. These devices
 * cannot use any kernel services during configuration, since they are not
 * yet available.
 * \n
 * \li SECONDARY: Used for devices that rely on the initialization of devices
 * initialized as part of the PRIMARY level. These devices cannot use any
 * kernel services during configuration, since they are not yet available.
 * \n
 * \li NANOKERNEL: Used for devices that require nanokernel services during
 * configuration.
 * \n
 * \li MICROKERNEL: Used for devices that require microkernel services during
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
 * @details This macro defines a device object that is automatically
 * configured by the kernel during system initialization. The driver_api
 * is also be set here, eliminating the need to do that during initialization.
 *
 * \see DEVICE_INIT() for description on other parameters.
 *
 * @param api Provides an initial pointer to the API function struct
 * used by the driver. Can be NULL.
 */

#ifndef CONFIG_DEVICE_POWER_MANAGEMENT
#define DEVICE_AND_API_INIT(dev_name, drv_name, init_fn, data, cfg_info, \
			    level, prio, api) \
	\
	static struct device_config __config_##dev_name __used \
	__attribute__((__section__(".devconfig.init"))) = { \
		.name = drv_name, .init = (init_fn), \
		.config_info = (cfg_info) \
	}; \
	\
	static struct device (__device_##dev_name) __used \
	__attribute__((__section__(".init_" #level STRINGIFY(prio)))) = { \
		 .config = &(__config_##dev_name), \
		 .driver_api = api, \
		 .driver_data = data \
	}

#define DEVICE_INIT(dev_name, drv_name, init_fn, data, cfg_info, level, prio) \
	DEVICE_AND_API_INIT(dev_name, drv_name, init_fn, data, cfg_info, \
			    level, prio, NULL)

#else
/**
 * @def DEVICE_AND_API_INIT_PM
 *
 * @brief Create device object and set it up for boot time initialization,
 * with the options to set driver_api and device_pm_ops.
 *
 * @details This macro defines a device object that is automatically
 * configured by the kernel during system initialization. This driver_api
 * and device_pm_ops are also be set here.
 *
 * @param device_pm_ops Address to the device_pm_ops structure of the driver.
 *
 * \see DEVICE_AND_API_INIT() for description on other parameters.
 */

#define DEVICE_AND_API_INIT_PM(dev_name, drv_name, init_fn, device_pm_ops, \
			       data, cfg_info, level, prio, api) \
	\
	static struct device_config __config_##dev_name __used \
	__attribute__((__section__(".devconfig.init"))) = { \
		.name = drv_name, .init = (init_fn), \
		.dev_pm_ops = (device_pm_ops), \
		.config_info = (cfg_info) \
	}; \
	\
	static struct device (__device_##dev_name) __used \
	__attribute__((__section__(".init_" #level STRINGIFY(prio)))) = { \
		 .config = &(__config_##dev_name), \
		 .driver_api = api, \
		 .driver_data = data \
	}

/**
 * @def DEVICE_INIT_PM
 *
 * @brief Create device object and set it up for boot time initialization,
 * with the options to device_pm_ops.
 *
 * @details This macro defines a device object that is automatically
 * configured by the kernel during system initialization. This device_pm_ops
 * is also be set here.
 *
 * @param device_pm_ops Address to the device_pm_ops structure of the driver.
 *
 * \see DEVICE_INIT() for description on other parameters.
 */

#define DEVICE_INIT_PM(dev_name, drv_name, init_fn, device_pm_ops, \
			       data, cfg_info, level, prio) \
	DEVICE_AND_API_INIT_PM(dev_name, drv_name, init_fn, device_pm_ops, \
			       data, cfg_info, level, prio, NULL)

	/*
	 * Create a default device_pm_ops for devices that do not call the
	 * DEVICE_INIT_PM macro so that caller of hook functions
	 * need not check dev_pm_ops != NULL.
	 */
extern struct device_pm_ops device_pm_ops_nop;
#define DEVICE_AND_API_INIT(dev_name, drv_name, init_fn, data, cfg_info, \
			    level, prio, api) \
	DEVICE_AND_API_INIT_PM(dev_name, drv_name, init_fn, \
			       &device_pm_ops_nop, data, cfg_info, \
			       level, prio, api)

#define DEVICE_INIT(dev_name, drv_name, init_fn, data, cfg_info, level, prio) \
	DEVICE_AND_API_INIT(dev_name, drv_name, init_fn, data, cfg_info, \
			    level, prio, NULL)

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

/**
 * @cond DEPRECATED_HIDDEN
 *
 * Hide these from showing in public documentation as these are
 * being deprecated.
 */
/*
 * DEPRECATED.
 *
 * DEV_* error codes are deprecated. Use error codes from errno.h instead.
 */
#define DEV_OK			0  /* No error */
#define DEV_FAIL		(-EIO) /* General operation failure */
#define DEV_INVALID_OP		(-ENOTSUP) /* Invalid operation */
#define DEV_INVALID_CONF	(-EINVAL) /* Invalid configuration */
#define DEV_USED		(-EBUSY) /* Device controller in use */
#define DEV_NO_ACCESS		(-EACCES) /* Controller not accessible */
#define DEV_NO_SUPPORT		(-ENODEV) /* Device type not supported */
#define DEV_NOT_CONFIG		(-EPERM) /* Device not configured */
/** @endcond */

struct device;

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
struct device_pm_ops {
	int (*suspend)(struct device *device, int pm_policy);
	int (*resume)(struct device *device, int pm_policy);
};
#endif

/**
 * @brief Static device information (In ROM) Per driver instance
 * @param name name of the device
 * @param init init function for the driver
 * @param config_info address of driver instance config information
 */
struct device_config {
	char	*name;
	int (*init)(struct device *device);
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	struct device_pm_ops *dev_pm_ops;
#endif
	void *config_info;
};

/**
 * @brief Runtime device structure (In memory) Per driver instance
 * @param device_config Build time config information
 * @param driver_api pointer to structure containing the API functions for
 * the device type. This pointer is filled in by the driver at init time.
 * @param driver_data river instance data. For driver use only
 */
struct device {
	struct device_config *config;
	void *driver_api;
	void *driver_data;
};

void _sys_device_do_config_level(int level);
struct device* device_get_binding(char *name);

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
/**
 * Device PM functions
 */

/**
 * @brief No-op function to initialize unimplemented pm hooks
 *
 * This function should be used to initialize device pm hooks
 * for which a device has no operation.
 *
 * @param unused_device
 * @param unused_policy
 *
 * @retval Always returns 0
 */
int device_pm_nop(struct device *unused_device, int unused_policy);

/**
 * @brief Call the suspend function of a device
 *
 * Called by the Power Manager application to let the device do
 * any policy based PM suspend operations.
 *
 * @param device Pointer to device structure of the driver instance.
 * @param pm_policy PM policy for which this call is made.
 *
 * @retval 0 If successful.
 * @retval -EBUSY If device is busy
 * @retval Other negative errno code if failure.
 */
static inline int device_suspend(struct device *device, int pm_policy)
{
	return device->config->dev_pm_ops->suspend(device, pm_policy);
}

/**
 * @brief Call the resume function of a device
 *
 * Called by the Power Manager application to let the device do
 * any policy based PM resume operations.
 *
 * @param device Pointer to device structure of the driver instance.
 * @param pm_policy PM policy for which this call is made.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int device_resume(struct device *device, int pm_policy)
{
	return device->config->dev_pm_ops->resume(device, pm_policy);
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

#endif

/**
 * Synchronous calls API
 */

#include <stdbool.h>
#include <nanokernel.h>
#ifdef CONFIG_MICROKERNEL
#include <microkernel.h>
#endif

#ifdef CONFIG_MICROKERNEL
enum device_sync_waiter {
	DEVICE_SYNC_WAITER_NONE,
	DEVICE_SYNC_WAITER_FIBER,
	DEVICE_SYNC_WAITER_TASK,
};
#endif

/**
 * Specific type for synchronizing calls among the 2 possible contexts
 */
typedef struct {
	/** Nanokernel semaphore used for fiber context */
	struct nano_sem f_sem;
#ifdef CONFIG_MICROKERNEL
	/** Microkernel semaphore used for task context */
	struct _k_sem_struct _t_sem;
	ksem_t t_sem;
	enum device_sync_waiter waiter;
	bool device_ready;
#endif
} device_sync_call_t;


/**
 * @brief Initialize the context-dependent synchronization data
 *
 * @param sync A pointer to a valid device_sync_call_t
 */
static inline void device_sync_call_init(device_sync_call_t *sync)
{
	nano_sem_init(&sync->f_sem);
#ifdef CONFIG_MICROKERNEL
	sync->_t_sem.waiters = NULL;
	sync->_t_sem.level = sync->_t_sem.count = 0;
	sync->t_sem = (ksem_t)&sync->_t_sem;
	sync->waiter = DEVICE_SYNC_WAITER_NONE;
	sync->device_ready = false;
#endif
}

#ifdef CONFIG_MICROKERNEL

/*
 * The idle task cannot block and is used during boot, and thus polls a
 * nanokernel semaphore instead of waiting on a microkernel semaphore.
 */
static inline bool _is_blocking_task(void)
{
	bool is_task = sys_execution_context_type_get() == NANO_CTX_TASK;
	bool is_idle_task = task_priority_get() == (CONFIG_NUM_TASK_PRIORITIES - 1);

	return is_task && !is_idle_task;
}

/**
 * @brief Wait for the isr to complete the synchronous call
 * Note: In a microkernel built this function will take care of the caller
 * context and thus use the right attribute to handle the synchronization.
 *
 * @param sync A pointer to a valid device_sync_call_t
 */
static inline void device_sync_call_wait(device_sync_call_t *sync)
{
	/* protect the state of device_ready and waiter fields */
	int key = irq_lock();

	if (sync->device_ready) {
		sync->device_ready = false;
		/*
		 * If device_ready was set, the waiter field had to be NONE, so we
		 * don't have to reset it.
		 */
		irq_unlock(key);
		return;
	}

	if (_is_blocking_task()) {
		sync->waiter = DEVICE_SYNC_WAITER_TASK;
		irq_unlock(key);
		task_sem_take(sync->t_sem, TICKS_UNLIMITED);
	} else {
		sync->waiter = DEVICE_SYNC_WAITER_FIBER;
		irq_unlock(key);
		nano_sem_take(&sync->f_sem, TICKS_UNLIMITED);
	}

	sync->waiter = DEVICE_SYNC_WAITER_NONE;

	/* if we get here, device_ready was not set: we don't have to reset it */
}

/**
 * @brief Signal the waiter about synchronization completion
 * Note: In a microkernel built this function will take care of the waiter
 * context and thus use the right attribute to signale the completion.
 *
 * @param sync A pointer to a valid device_sync_call_t
 */
static inline void device_sync_call_complete(device_sync_call_t *sync)
{
	static void (*func[3])(ksem_t sema) = {
		isr_sem_give,
		fiber_sem_give,
		task_sem_give
	};

	/* protect the state of device_ready and waiter fields */
	int key = irq_lock();

	if (sync->waiter == DEVICE_SYNC_WAITER_NONE) {
		sync->device_ready = true;
		irq_unlock(key);
		return;
	}

	/*
	 * It's safe to unlock interrupts here since we know there was a waiter,
	 * and only one thread is allowed to wait on the object, so the state of
	 * waiter will not change and the device_ready flag will not get set.
	 */
	irq_unlock(key);

	if (sync->waiter == DEVICE_SYNC_WAITER_TASK) {
		func[sys_execution_context_type_get()](sync->t_sem);
	} else /* fiber */ {
		nano_sem_give(&sync->f_sem);
	}
}

#else

/**
 * @brief Wait for the isr to complete the synchronous call
 * Note: It will simply wait on the internal semaphore.
 *
 * @param sync A pointer to a valid device_sync_call_t
 */
static inline void device_sync_call_wait(device_sync_call_t *sync)
{
	nano_sem_take(&sync->f_sem, TICKS_UNLIMITED);
}

/**
 * @brief Signal the waiter about synchronization completion
 * Note: It will simply release the internal semaphore
 *
 * @param sync A pointer to a valid device_sync_call_t
 */
static inline void device_sync_call_complete(device_sync_call_t *sync)
{
	nano_sem_give(&sync->f_sem);
}

#endif /* CONFIG_MICROKERNEL || CONFIG_NANOKERNEL */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif /* _DEVICE_H_ */
