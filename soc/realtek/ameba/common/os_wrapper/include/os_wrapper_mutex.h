/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OS_WRAPPER_MUTEX_H__
#define __OS_WRAPPER_MUTEX_H__

/**
 * @brief  mutex handle type
 */
typedef void *rtos_mutex_t;

#define MUTEX_WAIT_TIMEOUT 0xFFFFFFFFU /* will be replaced by common max timeout later */

/**
 * @brief  Static memory allocation implementation of rtos_mutex_create
 * @param  pp_handle: The handle itself is a pointer, and the pp_handle means a pointer to the
 * handle.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_mutex_create_static(rtos_mutex_t *pp_handle);

/**
 * @brief  Static memory allocation implementation of rtos_mutex_delete
 * @param  p_handle: Address of the mutex
 * @retval The status is SUCCESS or FAIL
 */
int rtos_mutex_delete_static(rtos_mutex_t p_handle);

/**
 * @brief  Static memory allocation implementation of rtos_mutex_recursive_create
 * @param  pp_handle: The handle itself is a pointer, and the pp_handle means a pointer to the
 * handle.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_mutex_recursive_create_static(rtos_mutex_t *pp_handle);

/**
 * @brief  Static memory allocation implementation of rtos_mutex_recursive_delete
 * @param  p_handle: Address of the mutex
 * @retval The status is SUCCESS or FAIL
 */
int rtos_mutex_recursive_delete_static(rtos_mutex_t p_handle);

/**
 * @brief  Compared to semaphores, Mutex has a priority inheritance mechanism.
 *         Dynamic allocate memory.
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
 * @retval The status is SUCCESS or FAIL
 */
int rtos_mutex_create(rtos_mutex_t *pp_handle);

/**
 * @brief  Delete a mutex.
 * @note   Do not delete mutex if held by a task
 * @param  p_handle: Address of the mutex
 * @retval The status is SUCCESS or FAIL
 */
int rtos_mutex_delete(rtos_mutex_t p_handle);

/**
 * @brief  Take a mutex.
 *         The API internally determines whether it is in the interrupt state and calls the
 * corresponding RTOS interface.
 * @param  p_handle: Address of the mutex
 * @param  wait_ms: The time in milliseconds to wait, 0xFFFFFFFF means Block infinitely until the
 * semaphore taken.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_mutex_take(rtos_mutex_t p_handle, uint32_t wait_ms);

/**
 * @brief  Give a semaphore.
 *         The API internally determines whether it is in the interrupt state and calls the
 * corresponding RTOS interface.
 * @param  p_handle: Address of the mutex
 * @retval The status is SUCCESS or FAIL
 */
int rtos_mutex_give(rtos_mutex_t p_handle);

/**
 * @brief  Creates a new recursive mutex, compared to semaphores, Mutex has a priority inheritance
 * mechanism. Dynamic allocate memory.
 * @note   Usage example:
 * Create:
 *          rtos_mutex_t mutex_handle;
 *          rtos_mutex_recursive_create(&mutex_handle);
 * Give:
 *          rtos_mutex_recursive_give(mutex_handle);
 * Take:
 *          rtos_mutex_recursive_take(mutex_handle, 100);
 * Delete:
 *          rtos_mutex_recursive_delete(mutex_handle);
 * @param  pp_handle: The handle itself is a pointer, and the pp_handle means a pointer to the
 * handle.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_mutex_recursive_create(rtos_mutex_t *pp_handle);

/**
 * @brief  Delete a mutex.
 * @param  p_handle: Address of the mutex
 * @retval The status is SUCCESS or FAIL
 */
int rtos_mutex_recursive_delete(rtos_mutex_t p_handle);

/**
 * @brief  Take a mutex.
 * @note   recursive mutexes cannot be used in interrupt service routines.
 * @param  p_handle: Address of the mutex
 * @param  wait_ms: The time in milliseconds to wait, 0xFFFFFFFF means Block infinitely until the
 * semaphore taken.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_mutex_recursive_take(rtos_mutex_t p_handle, uint32_t wait_ms);

/**
 * @brief  Give a semaphore.
 * @note   Recursive mutexes cannot be used in interrupt service routines.
 * @param  p_handle: Address of the mutex
 * @retval The status is SUCCESS or FAIL
 */
int rtos_mutex_recursive_give(rtos_mutex_t p_handle);

#endif
