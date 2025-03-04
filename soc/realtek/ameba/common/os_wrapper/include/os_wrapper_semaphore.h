/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OS_WRAPPER_SEMA_H__
#define __OS_WRAPPER_SEMA_H__

#define RTOS_SEMA_MIN_COUNT 0x0UL
#define RTOS_SEMA_MAX_COUNT 0xFFFFFFFFUL

/**
 * @brief  semaphore handle type
 */
typedef void *rtos_sema_t;

/**
 * @brief  Static memory allocation implementation of rtos_sema_create
 * @param  pp_handle: The handle itself is a pointer, and the pp_handle means a pointer to the
 * handle.
 * @param  init_count: The count value assigned to the semaphore when it is created.
 * @param  max_count:  The maximum count value that can be reached.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_sema_create_static(rtos_sema_t *pp_handle, uint32_t init_count, uint32_t max_count);

/**
 * @brief  Static memory allocation implementation of rtos_sema_create_binary
 * @param  pp_handle: The handle itself is a pointer, and the pp_handle means a pointer to the
 * handle.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_sema_create_binary_static(rtos_sema_t *pp_handle);

/**
 * @brief  Static memory allocation implementation of rtos_sema_delete
 * @param  p_handle: Address of the semaphore
 * @retval The status is SUCCESS or FAIL
 */
int rtos_sema_delete_static(rtos_sema_t p_handle);

/**
 * @brief  Creates a new counting semaphore instance
 *         Dynamic allocate memory.
 * @note   Usage example:
 * Create:
 *          rtos_sema_t sema_handle;
 *          rtos_sema_create(&sema_handle, 0, 10);
 * Give:
 *          rtos_sema_give(sema_handle);
 * Take:
 *          rtos_sema_take(sema_handle, 100);
 * Delete:
 *          rtos_sema_delete(sema_handle);
 * @param  pp_handle: The handle itself is a pointer, and the pp_handle means a pointer to the
 * handle.
 * @param  init_count: The count value assigned to the semaphore when it is created.
 * @param  max_count:  The maximum count value that can be reached.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_sema_create(rtos_sema_t *pp_handle, uint32_t init_count, uint32_t max_count);

/**
 * @brief  Creates a new binary semaphore instance
 *         Dynamic allocate memory.
 * @note   The semaphore must first be 'given' before it can be 'taken'.
 * Usage example:
 * Create:
 *          rtos_sema_t sema_handle;
 *          rtos_sema_create_binary(&sema_handle);
 * Give:
 *          rtos_sema_give(sema_handle);
 * Take:
 *          rtos_sema_take(sema_handle, 100);
 * Delete:
 *          rtos_sema_delete(sema_handle);
 * @param  pp_handle: The handle itself is a pointer, and the pp_handle means a pointer to the
 * handle.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_sema_create_binary(rtos_sema_t *pp_handle);

/**
 * @brief  Delete a semaphore.
 * @param  p_handle: A handle to the semaphore to be deleted.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_sema_delete(rtos_sema_t p_handle);

/**
 * @brief  Take a semaphore.
 *         The API internally determines whether it is in the interrupt state and calls the
 * corresponding RTOS interface.
 *
 * @note   If timeout_ms is set to the maximum value,
 *         then if the semaphore cannot be obtained consistently, the log will be printed every 10
 * seconds.
 * @param  p_handle: Address of the semaphore.
 * @param  timeout_ms: Waiting period to add the message, 0xFFFFFFFF means Block infinitely.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_sema_take(rtos_sema_t p_handle, uint32_t timeout_ms);

/**
 * @brief  Give a semaphore.
 *         The API internally determines whether it is in the interrupt state and calls the
 * corresponding RTOS interface.
 * @param  p_handle: Address of the semaphore.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_sema_give(rtos_sema_t p_handle);

/**
 * @brief  Get a semaphore's count. If the semaphore is a binary semaphore then returns 1 if the
 * semaphore is available, and 0 if the semaphore is not available.
 * @param  p_handle: Address of the semaphore.
 * @retval Current semaphore count.
 */
uint32_t rtos_sema_get_count(rtos_sema_t p_handle);

#endif
