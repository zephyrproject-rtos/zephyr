/***********************************************************************************************//**
 * \file cyabs_rtos_zephyr.c
 *
 * \brief
 * Implementation for Zephyr RTOS abstraction
 *
 ***************************************************************************************************
 * \copyright
 * Copyright 2018-2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **************************************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <cy_utils.h>
#include <cyabs_rtos.h>

#if defined(__cplusplus)
extern "C" {
#endif


/*
 *                 Error Converter
 */

/* Last received error status */
static cy_rtos_error_t dbgErr;

cy_rtos_error_t cy_rtos_last_error(void)
{
	return dbgErr;
}

/* Converts internal error type to external error type */
static cy_rslt_t error_converter(cy_rtos_error_t internalError)
{
	cy_rslt_t value;

	switch (internalError) {
	case 0:
		value = CY_RSLT_SUCCESS;
		break;

	case -EAGAIN:
		value = CY_RTOS_TIMEOUT;
		break;

	case -EINVAL:
		value = CY_RTOS_BAD_PARAM;
		break;

	case -ENOMEM:
		value = CY_RTOS_NO_MEMORY;
		break;

	default:
		value = CY_RTOS_GENERAL_ERROR;
		break;
	}

	/* Update the last known error status */
	dbgErr = internalError;

	return value;
}

static void free_thead_obj(cy_thread_t *thread)
{
	/* Free allocated stack buffer */
	if ((*thread)->memptr != NULL) {
		k_free((*thread)->memptr);
	}

	/* Free object */
	k_free(*thread);
}


/*
 *                 Threads
 */

static void thread_entry_function_wrapper(void *p1, void *p2, void *p3)
{
	CY_UNUSED_PARAMETER(p3);
	((cy_thread_entry_fn_t)p2)(p1);
}

cy_rslt_t cy_rtos_create_thread(cy_thread_t *thread, cy_thread_entry_fn_t entry_function,
				const char *name, void *stack, uint32_t stack_size,
				cy_thread_priority_t priority, cy_thread_arg_t arg)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;
	cy_rtos_error_t status_internal = 0;

	if ((thread == NULL) || (stack_size < CY_RTOS_MIN_STACK_SIZE)) {
		status = CY_RTOS_BAD_PARAM;
	} else if ((stack != NULL) && (0 != (((uint32_t)stack) & CY_RTOS_ALIGNMENT_MASK))) {
		status = CY_RTOS_ALIGNMENT_ERROR;
	} else {
		void *stack_alloc = NULL;
		k_tid_t my_tid = NULL;

		/* Allocate cy_thread_t object */
		*thread = k_malloc(sizeof(k_thread_wrapper_t));

		/* Allocate stack if NULL was passed */
		if ((uint32_t *)stack == NULL) {
			stack_alloc = k_aligned_alloc(Z_KERNEL_STACK_OBJ_ALIGN,
						      K_KERNEL_STACK_LEN(stack_size));

			/* Store pointer to allocated stack,
			 * NULL if not allocated by cy_rtos_thread_create (passed by application).
			 */
			(*thread)->memptr = stack_alloc;
		} else {
			stack_alloc = stack;
			(*thread)->memptr = NULL;
		}

		if (stack_alloc == NULL) {
			status = CY_RTOS_NO_MEMORY;
		} else {
			/* Create thread */
			my_tid = k_thread_create(&(*thread)->z_thread, stack_alloc, stack_size,
						 thread_entry_function_wrapper,
						 (cy_thread_arg_t)arg, (void *)entry_function, NULL,
						 priority, 0, K_NO_WAIT);

			/* Set thread name */
			status_internal = k_thread_name_set(my_tid, name);
			/* NOTE: k_thread_name_set returns -ENOSYS if thread name
			 * configuration option not enabled.
			 */
		}

		if ((my_tid == NULL) || ((status_internal != 0) && (status_internal != -ENOSYS))) {
			status = CY_RTOS_GENERAL_ERROR;

			/* Free allocated thread object and stack buffer */
			free_thead_obj(thread);
		}
	}

	return status;
}

cy_rslt_t cy_rtos_exit_thread(void)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (k_is_in_isr()) {
		status = CY_RTOS_GENERAL_ERROR;
	} else {
		cy_thread_t thread = (cy_thread_t)k_current_get();

		/* Abort thread */
		k_thread_abort((k_tid_t)&(thread)->z_thread);

		CODE_UNREACHABLE;
	}

	return status;
}

cy_rslt_t cy_rtos_terminate_thread(cy_thread_t *thread)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (thread == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else if (k_is_in_isr()) {
		status = CY_RTOS_GENERAL_ERROR;
	} else {
		/* Abort thread */
		k_thread_abort((k_tid_t)&(*thread)->z_thread);

		/* Free allocated stack buffer */
		if ((*thread)->memptr != NULL) {
			k_free((*thread)->memptr);
		}
	}

	return status;
}

cy_rslt_t cy_rtos_is_thread_running(cy_thread_t *thread, bool *running)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if ((thread == NULL) || (running == NULL)) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		*running = (k_current_get() == &(*thread)->z_thread) ? true : false;
	}

	return status;
}
cy_rslt_t cy_rtos_get_thread_state(cy_thread_t *thread, cy_thread_state_t *state)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if ((thread == NULL) || (state == NULL)) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		if (k_is_in_isr()) {
			*state = CY_THREAD_STATE_UNKNOWN;
		} else if (k_current_get() == &(*thread)->z_thread) {
			*state = CY_THREAD_STATE_RUNNING;
		}
		if (((*thread)->z_thread.base.thread_state & _THREAD_DEAD) != 0) {
			*state = CY_THREAD_STATE_TERMINATED;
		} else {
			switch ((*thread)->z_thread.base.thread_state) {
			case _THREAD_DUMMY:
				*state = CY_THREAD_STATE_UNKNOWN;
				break;

			case _THREAD_SUSPENDED:
			case _THREAD_PENDING:
				*state = CY_THREAD_STATE_BLOCKED;
				break;

			case _THREAD_QUEUED:
				*state = CY_THREAD_STATE_READY;
				break;

			default:
				*state = CY_THREAD_STATE_UNKNOWN;
				break;
			}
		}
	}

	return status;
}

cy_rslt_t cy_rtos_join_thread(cy_thread_t *thread)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;
	cy_rtos_error_t status_internal;

	if (thread == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		if (*thread != NULL) {
			/* Sleep until a thread exits */
			status_internal = k_thread_join(&(*thread)->z_thread, K_FOREVER);
			status = error_converter(status_internal);

			if (status == CY_RSLT_SUCCESS) {
				/* Free allocated thread object and stack buffer */
				free_thead_obj(thread);
			}
		}
	}

	return status;
}

cy_rslt_t cy_rtos_get_thread_handle(cy_thread_t *thread)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (thread == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		*thread = (cy_thread_t)k_current_get();
	}

	return status;
}

cy_rslt_t cy_rtos_wait_thread_notification(cy_time_t timeout_ms)
{
	cy_rtos_error_t status_internal;
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (timeout_ms == CY_RTOS_NEVER_TIMEOUT) {
		status_internal = k_sleep(K_FOREVER);
	} else {
		status_internal = k_msleep((int32_t)timeout_ms);
		if (status_internal == 0) {
			status = CY_RTOS_TIMEOUT;
		}
	}

	if ((status_internal < 0) && (status_internal != K_TICKS_FOREVER)) {
		status = error_converter(status_internal);
	}

	return status;
}

cy_rslt_t cy_rtos_thread_set_notification(cy_thread_t *thread, bool in_isr)
{
	CY_UNUSED_PARAMETER(in_isr);

	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (thread == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		k_wakeup(&(*thread)->z_thread);
	}

	return status;
}


/*
 *                 Mutexes
 */

cy_rslt_t cy_rtos_init_mutex2(cy_mutex_t *mutex, bool recursive)
{
	cy_rtos_error_t status_internal;
	cy_rslt_t status;

	/* Non recursive mutex is not supported by Zephyr */
	CY_UNUSED_PARAMETER(recursive);

	if (mutex == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Initialize a mutex object */
		status_internal = k_mutex_init(mutex);
		status = error_converter(status_internal);
	}

	return status;
}

cy_rslt_t cy_rtos_get_mutex(cy_mutex_t *mutex, cy_time_t timeout_ms)
{
	cy_rtos_error_t status_internal;
	cy_rslt_t status;

	if (mutex == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else if (k_is_in_isr()) {
		/* Mutexes may not be locked in ISRs */
		status = CY_RTOS_GENERAL_ERROR;
	} else {
		/* Convert timeout value */
		k_timeout_t k_timeout =
			(timeout_ms == CY_RTOS_NEVER_TIMEOUT) ? K_FOREVER : K_MSEC(timeout_ms);

		/* Lock a mutex */
		status_internal = k_mutex_lock(mutex, k_timeout);
		status = error_converter(status_internal);
	}

	return status;
}

cy_rslt_t cy_rtos_set_mutex(cy_mutex_t *mutex)
{
	cy_rtos_error_t status_internal;
	cy_rslt_t status;

	if (mutex == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else if (k_is_in_isr()) {
		/* Mutexes may not be unlocked in ISRs */
		status = CY_RTOS_GENERAL_ERROR;
	} else {
		/* Unlock a mutex */
		status_internal = k_mutex_unlock(mutex);
		status = error_converter(status_internal);
	}

	return status;
}

cy_rslt_t cy_rtos_deinit_mutex(cy_mutex_t *mutex)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (mutex == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Force unlock mutex, do not care about return */
		(void)k_mutex_unlock(mutex);
	}

	return status;
}


/*
 *                 Semaphores
 */

cy_rslt_t cy_rtos_init_semaphore(cy_semaphore_t *semaphore, uint32_t maxcount, uint32_t initcount)
{
	cy_rtos_error_t status_internal;
	cy_rslt_t status;

	if (semaphore == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Initialize a semaphore object */
		status_internal = k_sem_init(semaphore, initcount, maxcount);
		status = error_converter(status_internal);
	}

	return status;
}

cy_rslt_t cy_rtos_get_semaphore(cy_semaphore_t *semaphore, cy_time_t timeout_ms, bool in_isr)
{
	CY_UNUSED_PARAMETER(in_isr);

	cy_rtos_error_t status_internal;
	cy_rslt_t status;

	if (semaphore == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Convert timeout value */
		k_timeout_t k_timeout;

		if (k_is_in_isr()) {
			/* NOTE: Based on Zephyr documentation when k_sem_take
			 * is called from ISR timeout must be set to K_NO_WAIT.
			 */
			k_timeout = K_NO_WAIT;
		} else if (timeout_ms == CY_RTOS_NEVER_TIMEOUT) {
			k_timeout = K_FOREVER;
		} else {
			k_timeout = K_MSEC(timeout_ms);
		}

		/* Take a semaphore */
		status_internal = k_sem_take(semaphore, k_timeout);
		status = error_converter(status_internal);

		if (k_is_in_isr() && (status_internal == -EBUSY)) {
			status = CY_RSLT_SUCCESS;
		}
	}

	return status;
}

cy_rslt_t cy_rtos_set_semaphore(cy_semaphore_t *semaphore, bool in_isr)
{
	CY_UNUSED_PARAMETER(in_isr);

	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (semaphore == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Give a semaphore */
		k_sem_give(semaphore);
	}

	return status;
}

cy_rslt_t cy_rtos_get_count_semaphore(cy_semaphore_t *semaphore, size_t *count)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if ((semaphore == NULL) || (count == NULL)) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		*count = k_sem_count_get(semaphore);
	}

	return status;
}

cy_rslt_t cy_rtos_deinit_semaphore(cy_semaphore_t *semaphore)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (semaphore == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		k_sem_reset(semaphore);
	}

	return status;
}


/*
 *                 Events
 */

cy_rslt_t cy_rtos_init_event(cy_event_t *event)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (event == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Initialize an event object */
		k_event_init(event);
	}

	return status;
}

cy_rslt_t cy_rtos_setbits_event(cy_event_t *event, uint32_t bits, bool in_isr)
{
	CY_UNUSED_PARAMETER(in_isr);

	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (event == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Post the new bits */
		k_event_post(event, bits);
	}

	return status;
}

cy_rslt_t cy_rtos_clearbits_event(cy_event_t *event, uint32_t bits, bool in_isr)
{
	CY_UNUSED_PARAMETER(in_isr);

	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (event == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		uint32_t current_bits;

		/* Reads events value */
		status = cy_rtos_getbits_event(event, &current_bits);

		/* Set masked value */
		k_event_set(event, (~bits & current_bits));
	}

	return status;
}

cy_rslt_t cy_rtos_getbits_event(cy_event_t *event, uint32_t *bits)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if ((event == NULL) || (bits == NULL)) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* NOTE: Zephyr does not provide function for get bits,
		 * retrieve it from event object.
		 */
		*bits = event->events;
	}

	return status;
}

cy_rslt_t cy_rtos_waitbits_event(cy_event_t *event, uint32_t *bits, bool clear, bool all,
				 cy_time_t timeout)
{
	cy_rslt_t status;

	if ((event == NULL) || (bits == NULL)) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		uint32_t wait_for = *bits;
		k_timeout_t k_timeout =
			(timeout == CY_RTOS_NEVER_TIMEOUT) ? K_FOREVER : K_MSEC(timeout);

		if (all) {
			/* Wait for all of the specified events */
			*bits = k_event_wait_all(event, wait_for, false, k_timeout);
		} else {
			/* Wait for any of the specified events */
			*bits = k_event_wait(event, wait_for, false, k_timeout);
		}

		/* Check timeout */
		status = (*bits == 0) ? CY_RTOS_TIMEOUT : CY_RSLT_SUCCESS;

		/* Return full current events */
		cy_rtos_getbits_event(event, bits);

		/* Crear bits if required */
		if ((status == CY_RSLT_SUCCESS) && (clear == true)) {
			cy_rtos_clearbits_event(event, wait_for, false);
		}
	}
	return status;
}

cy_rslt_t cy_rtos_deinit_event(cy_event_t *event)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (event != NULL) {
		/* Clear event */
		k_event_set(event, 0);
	} else {
		status = CY_RTOS_BAD_PARAM;
	}
	return status;
}


/*
 *                 Queues
 */

cy_rslt_t cy_rtos_init_queue(cy_queue_t *queue, size_t length, size_t itemsize)
{
	cy_rtos_error_t status_internal;
	cy_rslt_t status;

	if (queue == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Initialize a message queue */
		status_internal = k_msgq_alloc_init(queue, itemsize, length);
		status = error_converter(status_internal);
	}

	return status;
}

cy_rslt_t cy_rtos_put_queue(cy_queue_t *queue, const void *item_ptr, cy_time_t timeout_ms,
			    bool in_isr)
{
	CY_UNUSED_PARAMETER(in_isr);

	cy_rtos_error_t status_internal;
	cy_rslt_t status;

	if ((queue == NULL) || (item_ptr == NULL)) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Convert timeout value */
		k_timeout_t k_timeout;

		if (k_is_in_isr()) {
			k_timeout = K_NO_WAIT;
		} else if (timeout_ms == CY_RTOS_NEVER_TIMEOUT) {
			k_timeout = K_FOREVER;
		} else {
			k_timeout = K_MSEC(timeout_ms);
		}

		/* Send a message to a message queue */
		status_internal = k_msgq_put(queue, item_ptr, k_timeout);
		status = error_converter(status_internal);
	}

	return status;
}

cy_rslt_t cy_rtos_get_queue(cy_queue_t *queue, void *item_ptr, cy_time_t timeout_ms, bool in_isr)
{
	CY_UNUSED_PARAMETER(in_isr);

	cy_rtos_error_t status_internal;
	cy_rslt_t status;

	if ((queue == NULL) || (item_ptr == NULL)) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Convert timeout value */
		k_timeout_t k_timeout;

		if (k_is_in_isr()) {
			k_timeout = K_NO_WAIT;
		} else if (timeout_ms == CY_RTOS_NEVER_TIMEOUT) {
			k_timeout = K_FOREVER;
		} else {
			k_timeout = K_MSEC(timeout_ms);
		}

		/* Receive a message from a message queue */
		status_internal = k_msgq_get(queue, item_ptr, k_timeout);
		status = error_converter(status_internal);
	}

	return status;
}

cy_rslt_t cy_rtos_count_queue(cy_queue_t *queue, size_t *num_waiting)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if ((queue == NULL) || (num_waiting == NULL)) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Get the number of messages in a message queue */
		*num_waiting = k_msgq_num_used_get(queue);
	}

	return status;
}

cy_rslt_t cy_rtos_space_queue(cy_queue_t *queue, size_t *num_spaces)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if ((queue == NULL) || (num_spaces == NULL)) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Get the amount of free space in a message queue */
		*num_spaces = k_msgq_num_free_get(queue);
	}

	return status;
}

cy_rslt_t cy_rtos_reset_queue(cy_queue_t *queue)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (queue == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Reset a message queue */
		k_msgq_purge(queue);
	}

	return status;
}

cy_rslt_t cy_rtos_deinit_queue(cy_queue_t *queue)
{
	cy_rtos_error_t status_internal;
	cy_rslt_t status;

	if (queue == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Reset a message queue */
		status = cy_rtos_reset_queue(queue);

		if (status == CY_RSLT_SUCCESS) {
			/* Release allocated buffer for a queue */
			status_internal = k_msgq_cleanup(queue);
			status = error_converter(status_internal);
		}
	}

	return status;
}


/*
 *                 Timers
 */
static void zephyr_timer_event_handler(struct k_timer *timer)
{
	cy_timer_t *_timer = (cy_timer_t *)timer;

	((cy_timer_callback_t)_timer->callback_function)(
		(cy_timer_callback_arg_t)_timer->arg);
}

cy_rslt_t cy_rtos_init_timer(cy_timer_t *timer, cy_timer_trigger_type_t type,
			     cy_timer_callback_t fun, cy_timer_callback_arg_t arg)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if ((timer == NULL) || (fun == NULL)) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		timer->callback_function = (void *)fun;
		timer->trigger_type = (uint32_t)type;
		timer->arg = (void *)arg;

		k_timer_init(&timer->z_timer, zephyr_timer_event_handler, NULL);
	}

	return status;
}

cy_rslt_t cy_rtos_start_timer(cy_timer_t *timer, cy_time_t num_ms)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (timer == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		if ((cy_timer_trigger_type_t)timer->trigger_type == CY_TIMER_TYPE_ONCE) {
			/* Called once only */
			k_timer_start(&timer->z_timer, K_MSEC(num_ms), K_NO_WAIT);
		} else {
			/* Called periodically until stopped */
			k_timer_start(&timer->z_timer, K_MSEC(num_ms), K_MSEC(num_ms));
		}
	}

	return status;
}

cy_rslt_t cy_rtos_stop_timer(cy_timer_t *timer)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (timer == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Stop timer */
		k_timer_stop(&timer->z_timer);
	}

	return status;
}

cy_rslt_t cy_rtos_is_running_timer(cy_timer_t *timer, bool *state)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if ((timer == NULL) || (state == NULL)) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Check if running */
		*state = (k_timer_remaining_get(&timer->z_timer) != 0u);
	}

	return status;
}

cy_rslt_t cy_rtos_deinit_timer(cy_timer_t *timer)
{
	cy_rslt_t status;

	if (timer == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		bool running;

		/* Get current timer state */
		status = cy_rtos_is_running_timer(timer, &running);

		/* Check if running */
		if ((status == CY_RSLT_SUCCESS) && (running)) {
			/* Stop timer */
			status = cy_rtos_stop_timer(timer);
		}
	}

	return status;
}


/*
 *                 Time
 */

cy_rslt_t cy_rtos_get_time(cy_time_t *tval)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (tval == NULL) {
		status = CY_RTOS_BAD_PARAM;
	} else {
		/* Get system uptime (32-bit version) */
		*tval = k_uptime_get_32();
	}

	return status;
}

cy_rslt_t cy_rtos_delay_milliseconds(cy_time_t num_ms)
{
	cy_rslt_t status = CY_RSLT_SUCCESS;

	if (k_is_in_isr()) {
		status = CY_RTOS_GENERAL_ERROR;
	} else {
		k_msleep(num_ms);
	}

	return status;
}

#if defined(__cplusplus)
}
#endif
