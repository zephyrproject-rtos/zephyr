/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OS_WRAPPER_TIMER_H__
#define __OS_WRAPPER_TIMER_H__

#define RTOS_TIMER_MAX_DELAY 0xFFFFFFFFUL

/**
 * @brief  timer handle type
 */
typedef void *rtos_timer_t;

/**
 * @brief  Static memory allocation implementation of rtos_timer_create
 * @param  pp_handle: The handle itself is a pointer, and the pp_handle means a pointer to the
 * handle.
 * @param  p_timer_name: A descriptive name for the timer.
 * @param  timer_id: An identifier that is assigned to the timer being created. Typically this would
 * be used in the timer callback function to identify which timer expired when the same callback
 * function is assigned to more than one timer.
 * @param  interval_ms: The timer period in milliseconds.
 * @param  reload: Used to set the timer as a periodic or one-shot timer.
 * @param  p_timer_callback: The function to call when the timer expires.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_timer_create_static(rtos_timer_t *pp_handle, const char *p_timer_name, uint32_t timer_id,
			     uint32_t interval_ms, uint8_t reload,
			     void (*p_timer_callback)(void *));

/**
 * @brief  Static memory allocation implementation of rtos_timer_delete
 * @param  p_handle: The handle of the timer being deleted.
 * @param  wait_ms: Specifies the time that the calling task should be held in the Blocked state
 *                  to wait for the delete command to be successfully sent to the timer command
 * queue.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_timer_delete_static(rtos_timer_t p_handle, uint32_t wait_ms);

/**
 * @brief  Create a new software timer instance. This allocates the storage required by the new
 * timer, initializes the new timers internal state, and returns a handle by which the new timer can
 * be referenced.
 * @note   Usage example:
 * Create:
 *          rtos_timer_t timer_handle;
 *          rtos_timer_create(&timer_handle, "timer_test", timer_id, delay_ms, is_reload,
 * timer_cb_function); Start: rtos_timer_start(timer_handle, wait_ms); Stop:
 *          rtos_timer_stop(timer_handle, wait_ms);
 * Delete:
 *          rtos_timer_delete(timer_handle, wait_ms);
 * @param  pp_handle: The handle itself is a pointer, and the pp_handle means a pointer to the
 * handle.
 * @param  p_timer_name: A descriptive name for the timer.
 * @param  timer_id: An identifier that is assigned to the timer being created. Typically this would
 * be used in the timer callback function to identify which timer expired when the same callback
 * function is assigned to more than one timer.
 * @param  interval_ms: The timer period in milliseconds.
 * @param  reload: Used to set the timer as a periodic or one-shot timer.
 * @param  p_timer_callback: The function to call when the timer expires.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_timer_create(rtos_timer_t *pp_handle, const char *p_timer_name, uint32_t timer_id,
		      uint32_t interval_ms, uint8_t reload, void (*p_timer_callback)(void *));

/**
 * @brief  Delete a timer that was previously created using a call to rtos_timer_create.
 * @param  p_handle: The handle of the timer being deleted.
 * @param  wait_ms: Specifies the time that the calling task should be held in the Blocked state
 *                  to wait for the delete command to be successfully sent to the timer command
 * queue.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_timer_delete(rtos_timer_t p_handle, uint32_t wait_ms);

/**
 * @brief  Start a timer that was previously created using a call to rtos_timer_create.
 * @param  p_handle: The handle of the created timer
 * @param  wait_ms: Specifies the time that the calling task should be held in the Blocked state
 *                  to wait for the start command to be successfully sent to the timer command
 * queue.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_timer_start(rtos_timer_t p_handle, uint32_t wait_ms);

/**
 * @brief  Stopping a timer ensures the timer is not in the active state.
 * @param  p_handle: The handle of the created timer
 * @param  wait_ms: Specifies the time that the calling task should be held in the Blocked state
 *                  to wait for the stop command to be successfully sent to the timer command queue.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_timer_stop(rtos_timer_t p_handle, uint32_t wait_ms);

/**
 * @brief  changes the period of a timer that was previously created using a call to
 * rtos_timer_create.
 * @param  p_handle: The handle of the created timer
 * @param  interval_ms: The new period for xTimer.
 * @param  wait_ms: Specifies the time that the calling task should be held in the Blocked state
 *                  to wait for the change command to be successfully sent to the timer command
 * queue.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_timer_change_period(rtos_timer_t p_handle, uint32_t interval_ms, uint32_t wait_ms);

/**
 * @brief  Queries a timer to see if it is active or dormant. A timer will be dormant if:
 *         1) It has been created but not started, or
 *         2) It is an expired one-shot timer that has not been restarted.
 * @note   non-interrupt safety
 * @param  p_handle: The handle of the created timer
 * @retval Timer is active or not.
 */
uint32_t rtos_timer_is_timer_active(rtos_timer_t p_handle);

/**
 * @brief  Get the ID assigned to the timer when created.
 * @note   non-interrupt safety
 * @param  p_handle: Pointer to the created timer handle.
 * @retval the ID assigned to the timer.
 */
uint32_t rtos_timer_get_id(rtos_timer_t p_handle);

#endif
