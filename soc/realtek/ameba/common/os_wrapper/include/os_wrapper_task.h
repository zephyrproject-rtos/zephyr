/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OS_WRAPPER_TASK_H__
#define __OS_WRAPPER_TASK_H__

/* CONFIG_NUM_COOP_PRIORITIES shall be 0 */
#define RTOS_TASK_MAX_PRIORITIES       (CONFIG_NUM_PREEMPT_PRIORITIES)
#define RTOS_MINIMAL_STACK_SIZE        (1024)
#define RTOS_MINIMAL_SECURE_STACK_SIZE (1024)

#define RTOS_SCHED_SUSPENDED   0x0UL
#define RTOS_SCHED_NOT_STARTED 0x1UL
#define RTOS_SCHED_RUNNING     0x2UL

/**
 * @brief  task handle and function type
 */
typedef void *rtos_task_t;
typedef void (*rtos_task_function_t)(void *);

/**
 * @brief  Start os kernel scheduler
 * @retval return SUCCESS Only
 */
int rtos_sched_start(void);

/**
 * @brief  Stop os kernel scheduler
 * @retval return SUCCESS Only
 */
int rtos_sched_stop(void);

/**
 * @brief  Suspend os kernel scheduler
 * @retval return SUCCESS Only
 */
int rtos_sched_suspend(void);

/**
 * @brief  Resume os kernel scheduler
 * @retval return SUCCESS Only
 */
int rtos_sched_resume(void);

/**
 * @brief  Get scheduler status.
 * @retval RTOS_SCHED_SUSPENDED / RTOS_SCHED_NOT_STARTED / RTOS_SCHED_RUNNING
 */
int rtos_sched_get_state(void);

/**
 * @brief  Create os level task routine.
 * @note   Usage example:
 * Create:
 *          rtos_task_t handle;
 *          rtos_task_create(&handle, "test_task", task, NULL, 1024, 2);
 * Suspend:
 *          rtos_task_suspend(handle);
 * Resume:
 *          rtos_task_resume(handle);
 * Delete:
 *          rtos_task_delete(handle);
 * @param  pp_handle: The handle itself is a pointer, and the pp_handle means a pointer to the
 * handle.
 * @param  p_name: A descriptive name for the task.
 * @param  p_routine: Pointer to the task entry function. Tasks must be implemented to never return
 * (i.e. continuous loop).
 * @param  p_param: Pointer that will be used as the parameter
 * @param  stack_size_in_byte: The size of the task stack specified
 * @param  priority: The priority at which the task should run (higher value, higher priority)
 * @retval SUCCESS(0) / FAIL(-1)
 */
int rtos_task_create(rtos_task_t *pp_handle, const char *p_name, rtos_task_function_t p_routine,
		     void *p_param, uint16_t stack_size_in_byte, uint16_t priority);

/**
 * @brief  Delete os level task routine.
 * @param  p_handle: Task handle. If a null pointer is passed, the task itself is deleted.
 * @retval return SUCCESS Only
 */
int rtos_task_delete(rtos_task_t p_handle);

/**
 * @brief  Suspend os level task routine.
 * @param  p_handle: Task handle.
 * @retval return SUCCESS Only
 */
int rtos_task_suspend(rtos_task_t p_handle);

/**
 * @brief  Resume os level task routine.
 * @param  p_handle: Task handle.
 * @retval return SUCCESS Only
 */
int rtos_task_resume(rtos_task_t p_handle);

/**
 * @brief  Yield current os level task routine.
 * @retval return SUCCESS Only
 */
int rtos_task_yield(void);

/**
 * @brief  Get current os level task routine handle.
 * @retval The task handle pointer
 */
rtos_task_t rtos_task_handle_get(void);

/**
 * @brief  Get os level task routine priority.
 * @param  p_handle: Task handle.
 * @retval Task priority value
 */
uint32_t rtos_task_priority_get(rtos_task_t p_handle);

/**
 * @brief  Set os level task routine priority.
 * @param  p_handle: Task handle.
 * @param  priority: The priority at which the task should run (higher value, higher priority)
 * @retval return SUCCESS Only
 */
int rtos_task_priority_set(rtos_task_t p_handle, uint16_t priority);

/**
 * @brief  Print current task status
 */
void rtos_task_out_current_status(void);

/**
 * @brief  Allocate a secure context for the task.
 * @note   trustzone is required. Otherwise it's just an empty implementation.
 */
void rtos_create_secure_context(uint32_t size);

#endif
