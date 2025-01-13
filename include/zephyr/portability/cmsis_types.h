/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CMSIS_TYPES_H_
#define ZEPHYR_INCLUDE_CMSIS_TYPES_H_

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/portability/cmsis_os2.h>

/** @brief Size for names of RTOS objects. */
#define CMSIS_OBJ_NAME_MAX_LEN 16

/**
 * @brief Control block for a CMSIS-RTOSv2 thread.
 *
 * Application can use manual user-defined allocation for RTOS objects by supplying a pointer to
 * thread control block. Control block is initiazed within `osThreadNew()`.
 */
struct cmsis_rtos_thread_cb {
	sys_dnode_t node;
	struct k_thread z_thread;
	struct k_poll_signal poll_signal;
	struct k_poll_event poll_event;
	uint32_t signal_results;
	char name[CMSIS_OBJ_NAME_MAX_LEN];
	uint32_t attr_bits;
	struct k_sem join_guard;
	char has_joined;
};

/**
 * @brief Control block for a CMSIS-RTOSv2 timer.
 *
 * Application can use manual user-defined allocation for RTOS objects by supplying a pointer to
 * timer control block. Control block is initiazed within `osTimerNew()`.
 */
struct cmsis_rtos_timer_cb {
	struct k_timer z_timer;
	osTimerType_t type;
	uint32_t status;
	bool is_cb_dynamic_allocation;
	char name[CMSIS_OBJ_NAME_MAX_LEN];
	void (*callback_function)(void *argument);
	void *arg;
};

/**
 * @brief Control block for a CMSIS-RTOSv2 mutex.
 *
 * Application can use manual user-defined allocation for RTOS objects by supplying a pointer to
 * mutex control block. Control block is initiazed within `osMutexNew()`.
 */
struct cmsis_rtos_mutex_cb {
	struct k_mutex z_mutex;
	bool is_cb_dynamic_allocation;
	char name[CMSIS_OBJ_NAME_MAX_LEN];
	uint32_t state;
};

/**
 * @brief Control block for a CMSIS-RTOSv2 semaphore.
 *
 * Application can use manual user-defined allocation for RTOS objects by supplying a pointer to
 * semaphore control block. Control block is initiazed within `osSemaphoreNew()`.
 */
struct cmsis_rtos_semaphore_cb {
	struct k_sem z_semaphore;
	bool is_cb_dynamic_allocation;
	char name[CMSIS_OBJ_NAME_MAX_LEN];
};

/**
 * @brief Control block for a CMSIS-RTOSv2 memory pool.
 *
 * Application can use manual user-defined allocation for RTOS objects by supplying a pointer to
 * memory pool control block. Control block is initiazed within `osMemoryPoolNew()`.
 */
struct cmsis_rtos_mempool_cb {
	struct k_mem_slab z_mslab;
	void *pool;
	char is_dynamic_allocation;
	bool is_cb_dynamic_allocation;
	char name[CMSIS_OBJ_NAME_MAX_LEN];
};

/**
 * @brief Control block for a CMSIS-RTOSv2 message queue.
 *
 * Application can use manual user-defined allocation for RTOS objects by supplying a pointer to
 * message queue control block. Control block is initiazed within `osMessageQueueNew()`.
 */
struct cmsis_rtos_msgq_cb {
	struct k_msgq z_msgq;
	void *pool;
	char is_dynamic_allocation;
	bool is_cb_dynamic_allocation;
	char name[CMSIS_OBJ_NAME_MAX_LEN];
};

/**
 * @brief Control block for a CMSIS-RTOSv2 event flag.
 *
 * Application can use manual user-defined allocation for RTOS objects by supplying a pointer to
 * event flag control block. Control block is initiazed within `osEventFlagsNew()`.
 */
struct cmsis_rtos_event_cb {
	struct k_poll_signal poll_signal;
	struct k_poll_event poll_event;
	uint32_t signal_results;
	bool is_cb_dynamic_allocation;
	char name[CMSIS_OBJ_NAME_MAX_LEN];
};

#endif
