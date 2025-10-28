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
 * mp_task_function
 *
 * @param user_data: user data passed to function
 *
 * Function repeatedly called in thread created by @ref struct mp_task.
 */
typedef void (*mp_task_function)(void *user_data, void *, void *);

/**
 * @struct mp_task
 * @brief Structure that represents a task in the system
 */
struct mp_task {
	/** Thread data */
	struct k_thread thread_data;
	/** Flag to indicate task status */
	bool running;
	/** Thread stack ID */
	int8_t stack_id;
};

/**
 * Create a new task
 *
 * @param task: pointer to task structure
 * @param func: entry function for the task
 * @param user_data: Additional data from user
 * @param priority: priority of the task
 * @return k_tid_t which is the pointer to the k_thread structure
 */
k_tid_t mp_task_create(struct mp_task *task, k_thread_entry_t func, void *user_data, int priority);

/**
 * Destroy a task
 *
 * @param task: pointer to task structure
 */
void mp_task_destroy(struct mp_task *task);

/** @} */

#endif /* __MP_TASK_H__ */
