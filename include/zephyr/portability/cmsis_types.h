/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for CMSIS-RTOSv2 object control block structures.
 * @ingroup os_services
 */

#ifndef ZEPHYR_INCLUDE_PORTABILITY_CMSIS_TYPES_H_
#define ZEPHYR_INCLUDE_PORTABILITY_CMSIS_TYPES_H_

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/portability/cmsis_os2.h>

/**
 * @brief Control block for a CMSIS-RTOSv2 thread.
 *
 * Application can use manual user-defined allocation for RTOS objects by supplying a pointer to
 * thread control block. Control block is initialized within `osThreadNew()`.
 */
struct cmsis_rtos_thread_cb {
	/** @cond INTERNAL_HIDDEN */
	sys_dnode_t node;
	struct k_thread z_thread;
	struct k_poll_signal poll_signal;
	struct k_poll_event poll_event;
	uint32_t signal_results;
	uint32_t attr_bits;
	/** @endcond */
};

/**
 * @brief Control block for a CMSIS-RTOSv2 timer.
 *
 * Application can use manual user-defined allocation for RTOS objects by supplying a pointer to
 * timer control block. Control block is initialized within `osTimerNew()`.
 */
struct cmsis_rtos_timer_cb {
	/** @cond INTERNAL_HIDDEN */
	struct k_timer z_timer;
	osTimerType_t type;
	uint32_t status;
	bool is_cb_dynamic_allocation;
	const char *name;
	void (*callback_function)(void *argument);
	void *arg;
	/** @endcond */
};

/**
 * @brief Control block for a CMSIS-RTOSv2 mutex.
 *
 * Application can use manual user-defined allocation for RTOS objects by supplying a pointer to
 * mutex control block. Control block is initialized within `osMutexNew()`.
 */
struct cmsis_rtos_mutex_cb {
	/** @cond INTERNAL_HIDDEN */
	struct k_mutex z_mutex;
	bool is_cb_dynamic_allocation;
	const char *name;
	uint32_t state;
	/** @endcond */
};

/**
 * @brief Control block for a CMSIS-RTOSv2 semaphore.
 *
 * Application can use manual user-defined allocation for RTOS objects by supplying a pointer to
 * semaphore control block. Control block is initialized within `osSemaphoreNew()`.
 */
struct cmsis_rtos_semaphore_cb {
	/** @cond INTERNAL_HIDDEN */
	struct k_sem z_semaphore;
	bool is_cb_dynamic_allocation;
	const char *name;
	/** @endcond */
};

/**
 * @brief Control block for a CMSIS-RTOSv2 memory pool.
 *
 * Application can use manual user-defined allocation for RTOS objects by supplying a pointer to
 * memory pool control block. Control block is initialized within `osMemoryPoolNew()`.
 */
struct cmsis_rtos_mempool_cb {
	/** @cond INTERNAL_HIDDEN */
	struct k_mem_slab z_mslab;
	void *pool;
	char is_dynamic_allocation;
	bool is_cb_dynamic_allocation;
	const char *name;
	/** @endcond */
};

/**
 * @brief Control block for a CMSIS-RTOSv2 message queue.
 *
 * Application can use manual user-defined allocation for RTOS objects by supplying a pointer to
 * message queue control block. Control block is initialized within `osMessageQueueNew()`.
 */
struct cmsis_rtos_msgq_cb {
	/** @cond INTERNAL_HIDDEN */
	struct k_msgq z_msgq;
	void *pool;
	char is_dynamic_allocation;
	bool is_cb_dynamic_allocation;
	const char *name;
	/** @endcond */
};

/**
 * @brief Control block for a CMSIS-RTOSv2 event flag.
 *
 * Application can use manual user-defined allocation for RTOS objects by supplying a pointer to
 * event flag control block. Control block is initialized within `osEventFlagsNew()`.
 */
struct cmsis_rtos_event_cb {
	/** @cond INTERNAL_HIDDEN */
	struct k_event z_event;
	bool is_cb_dynamic_allocation;
	const char *name;
	/** @endcond */
};

#endif /* ZEPHYR_INCLUDE_PORTABILITY_CMSIS_TYPES_H_ */
