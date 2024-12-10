/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OS_WRAPPER_TASK_H__
#define __OS_WRAPPER_TASK_H__

#include <zephyr/kernel.h>

typedef void (*rtos_task_function_t)(void *);
typedef struct k_thread *rtos_task_t;

/**
 * @brief  Suspend os kernel scheduler
 * @retval For RTOS, return SUCCESS
 */
int rtos_sched_suspend(void);

/**
 * @brief  Resume os kernel scheduler
 * @retval For RTOS, return SUCCESS
 */
int rtos_sched_resume(void);

#endif
