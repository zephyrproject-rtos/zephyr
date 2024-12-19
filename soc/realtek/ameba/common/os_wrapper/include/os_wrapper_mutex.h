/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OS_WRAPPER_MUTEX_H__
#define __OS_WRAPPER_MUTEX_H__

#include <zephyr/kernel.h>

#define MUTEX_WAIT_TIMEOUT 0xFFFFFFFFU

/**
 * @brief  mutex handle type
 */
typedef struct k_mutex *rtos_mutex_t;

/**
 * @brief  For Zephyr, map to k_mutex_init.
 * @note   Usage example:
 * Create:
 *          rtos_mutex_t mutex_handle;
 *          rtos_mutex_create(&mutex_handle);
 * Give:
 *          rtos_mutex_give(mutex_handle);
 * Take:
 *          rtos_mutex_take(mutex_handle, 100);
 * Delete:
 *          rtos_mutex_delete(mutex_handle);
 * @param  pp_handle: The handle itself is a pointer, and the pp_handle means a pointer to the
 * handle.
 * @retval
 */
int rtos_mutex_create(rtos_mutex_t *pp_handle);

/**
 * @brief  For FreeRTOS, map to vSemaphoreDelete
 * @note   Do not delete mutex if held by a task
 * @param  p_handle:
 * @retval
 */
int rtos_mutex_delete(rtos_mutex_t p_handle);

/**
 * @brief  For FreeRTOS, map to xSemaphoreTake / xSemaphoreTakeFromISR
 *         The API internally determines whether it is in the interrupt state and calls the
 * corresponding RTOS interface.
 * @param  p_handle:
 * @param  wait_ms:
 * @retval
 */
int rtos_mutex_take(rtos_mutex_t p_handle, uint32_t wait_ms);

/**
 * @brief  For FreeRTOS, map to xSemaphoreGive / xSemaphoreGiveFromISR
 *         The API internally determines whether it is in the interrupt state and calls the
 * corresponding RTOS interface.
 * @param  p_handle:
 * @param  wait_ms:
 * @retval
 */
int rtos_mutex_give(rtos_mutex_t p_handle);

int rtos_mutex_create_static(rtos_mutex_t *pp_handle);
int rtos_mutex_delete_static(rtos_mutex_t p_handle);

#endif
