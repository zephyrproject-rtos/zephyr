/*
 *==================================================================================================
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *==================================================================================================
 */
/*
 ***********************************************************************************************//**
 * \file cyabs_rtos_impl.h
 *
 * \brief
 * Internal definitions for RTOS abstraction layer
 *
 ***************************************************************************************************
 */

#pragma once

#include <stdbool.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 *                 Constants
 */
#define CY_RTOS_MIN_STACK_SIZE      300                 /** Minimum stack size in bytes */
#define CY_RTOS_ALIGNMENT           0x00000008UL        /** Minimum alignment for RTOS objects */
#define CY_RTOS_ALIGNMENT_MASK      0x00000007UL        /** Mask for checking the alignment of */

/*
 *                 Type Definitions
 */

/* RTOS thread priority */
typedef enum {
	CY_RTOS_PRIORITY_MIN            = CONFIG_NUM_PREEMPT_PRIORITIES - 1,
	CY_RTOS_PRIORITY_LOW            = (CONFIG_NUM_PREEMPT_PRIORITIES * 6 / 7),
	CY_RTOS_PRIORITY_BELOWNORMAL    = (CONFIG_NUM_PREEMPT_PRIORITIES * 5 / 7),
	CY_RTOS_PRIORITY_NORMAL         = (CONFIG_NUM_PREEMPT_PRIORITIES * 4 / 7),
	CY_RTOS_PRIORITY_ABOVENORMAL    = (CONFIG_NUM_PREEMPT_PRIORITIES * 3 / 7),
	CY_RTOS_PRIORITY_HIGH           = (CONFIG_NUM_PREEMPT_PRIORITIES * 2 / 7),
	CY_RTOS_PRIORITY_REALTIME       = (CONFIG_NUM_PREEMPT_PRIORITIES * 1 / 7),
	CY_RTOS_PRIORITY_MAX            = 0
} cy_thread_priority_t;

/** \cond INTERNAL */
typedef struct {
	struct k_thread z_thread;
	void *memptr;
} k_thread_wrapper_t;

typedef struct {
	struct k_timer z_timer;
	uint32_t trigger_type;
	uint32_t status;
	void *callback_function;
	void *arg;
} k_timer_wrapper_t;
/** \endcond */

/* Zephyr definition of a thread handle */
typedef k_thread_wrapper_t *cy_thread_t;

/* Argument passed to the entry function of a thread */
typedef void *cy_thread_arg_t;

/* Zephyr definition of a mutex */
typedef struct k_mutex cy_mutex_t;

/* Zephyr definition of a semaphore */
typedef struct k_sem cy_semaphore_t;

/* Zephyr definition of an event */
typedef struct k_event cy_event_t;

/* Zephyr definition of a message queue */
typedef struct k_msgq cy_queue_t;

/* Zephyr definition of a timer */
typedef k_timer_wrapper_t cy_timer_t;

/* Argument passed to the timer callback function */
typedef void *cy_timer_callback_arg_t;

/* Time in milliseconds */
typedef uint32_t cy_time_t;

/* Zephyr definition of a error status */
typedef int cy_rtos_error_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
