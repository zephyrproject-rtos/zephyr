
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

/** @def DECLARE_DEVICE_INIT_CONFIG
 *
 *  @brief Define an config object
 *
 *  @details This macro declares an config object to be placed in the
 *  image by the linker in the ROM region.
 *
 *  @param cfg_name Name of the config object to be created. This name
 *  must be used in the *_init() macro(s) defined in init.h so the
 *  linker can associate the config object with the correct init
 *  object.
 *
 *  @param drv_name The name this instance of the driver exposes to
 *  the system.
 *  @param init_fn Address to the init function of the driver.
 *  @param config The address to the structure containing the
 *  configuration information for this instance of the driver.
 *
 *  @sa __define_initconfig()
 */
#define DECLARE_DEVICE_INIT_CONFIG(cfg_name, drv_name, init_fn, config) \
	static struct device_config config_##cfg_name	__used		\
	__attribute__((__section__(".devconfig.init"))) = { \
		.name = drv_name, .init = (init_fn), \
		.config_info = (config)				   \
	}

/* Common Error Codes devices can provide */
#define DEV_OK			0  /* No error */
#define DEV_FAIL		1 /* General operation failure */
#define DEV_INVALID_OP		2 /* Invalid operation */
#define DEV_INVALID_CONF	3 /* Invalid configuration */
#define DEV_USED		4 /* Device controller in use */
#define DEV_NO_ACCESS		5 /* Controller not accessible */
#define DEV_NO_SUPPORT		6 /* Device type not supported */
#define DEV_NOT_CONFIG		7 /* Device not configured */

struct device;

/**
 * @brief Static device information (In ROM) Per driver instance
 * @param name name of the device
 * @param init init function for the driver
 * @param config_info address of driver instance config information
 */
struct device_config {
	char	*name;
	int (*init)(struct device *device);
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

/**
 * Synchronous calls API
 */

#include <stdbool.h>
#include <nanokernel.h>
#ifdef CONFIG_MICROKERNEL
#include <microkernel.h>
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
	bool caller_is_task;
#endif
} device_sync_call_t;


/**
 * @brief Initialize the context-dependent synchronization data
 *
 * @param sync A pointer to a valid devic_sync_call_t
 */
static inline void synchronous_call_init(device_sync_call_t *sync)
{
	nano_sem_init(&sync->f_sem);
#ifdef CONFIG_MICROKERNEL
	sync->_t_sem.waiters = NULL;
	sync->_t_sem.level = sync->_t_sem.count = 0;
	sync->t_sem = (ksem_t)&sync->_t_sem;
	sync->caller_is_task = false;
#endif
}

#ifdef CONFIG_MICROKERNEL

/**
 * @brief Wait for the isr to complete the synchronous call
 * Note: In a microkernel built this function will take care of the caller
 * context and thus use the right attribute to handle the synchronization.
 *
 * @param sync A pointer to a valid devic_sync_call_t
 */
static inline void synchronous_call_wait(device_sync_call_t *sync)
{
	if ((sys_execution_context_type_get() == NANO_CTX_TASK) &&
	    (task_priority_get() < CONFIG_NUM_TASK_PRIORITIES - 1)) {
		sync->caller_is_task = true;
		task_sem_take_wait(sync->t_sem);
	} else {
		sync->caller_is_task = false;
		nano_sem_take_wait(&sync->f_sem);
	}
}

/**
 * @brief Signal the caller about synchronization completion
 * Note: In a microkernel built this function will take care of the caller
 * context and thus use the right attribute to signale the completion.
 *
 * @param sync A pointer to a valid devic_sync_call_t
 */
static inline void synchronous_call_complete(device_sync_call_t *sync)
{
	if (sync->caller_is_task) {
		task_sem_give(sync->t_sem);
	} else {
		nano_isr_sem_give(&sync->f_sem);
	}
}

#else

/**
 * @brief Wait for the isr to complete the synchronous call
 * Note: It will simply wait on the internal semaphore.
 *
 * @param sync A pointer to a valid devic_sync_call_t
 */
static inline void synchronous_call_wait(device_sync_call_t *sync)
{
	nano_sem_take_wait(&sync->f_sem);
}

/**
 * @brief Signal the caller about synchronization completion
 * Note: It will simply release the internal semaphore
 *
 * @param sync A pointer to a valid devic_sync_call_t
 */
static inline void synchronous_call_complete(device_sync_call_t *sync)
{
	nano_isr_sem_give(&sync->f_sem);
}

#endif /* CONFIG_MICROKERNEL || CONFIG_NANOKERNEL */

#endif /* _DEVICE_H_ */
