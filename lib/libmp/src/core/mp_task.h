/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Task Management header file.
 */

#ifndef __MP_TASK_H__
#define __MP_TASK_H__

#include <stdbool.h>

#include <zephyr/kernel/thread.h>

/**
 *
 * @{
 */

/**
 * MpTaskFunction
 *
 * @param user_data: user data passed to function
 *
 * Function repeatedly called in thread created by @ref MpTask.
 */
typedef void (*MpTaskFunction)(void *user_data, void *, void *);

/**
 * @struct MpTask
 * @brief Structure that represents a task in the system
 */
typedef struct {
	/** Thread data */
	struct k_thread thread_data;
	/** Flag to indicate task status */
	bool running;
	/** Thread stack ID */
	uint8_t stack_id;
} MpTask;

/**
 * Create a new task
 *
 * @param task: pointer to task structure
 * @param func: entry function for the task
 * @param p1: first parameter to pass to the function
 * @param p2: second parameter to pass to the function
 * @param p3: third parameter to pass to the function
 * @param priority: priority of the task
 * @return k_tid_t which is the pointer to the k_thread structure
 */
k_tid_t mp_task_create(MpTask *task, k_thread_entry_t func, void *p1, void *p2, void *p3,
		       int priority);

/**
 * Destroy a task
 *
 * @param task: pointer to task structure
 */
void mp_task_destroy(MpTask *task);

/** @} */

#endif /* __MP_TASK_H__ */
