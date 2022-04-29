/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Utilities supporting asynchronous calls
 *
 * Functions and datatypes useful for safely communicating to a thread or work
 * queue of an asynchronous function completion.
 *
 * @note All callback helpers here are safe to call from an ISR.
 */

#ifndef ZEPHYR_INCLUDE_SYS_ASYNC_H_
#define ZEPHYR_INCLUDE_SYS_ASYNC_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup async_utility_apis Async Callback Utilities
 * @{
 */

/**
 * @brief Async operation callback type
 *
 * Common function pointer type used for asynchronous completion callbacks
 * and implemented for some commonly used IPC notification mechanisms.
 *
 * @note Implementations of this callback should be ISR ok.
 *
 * @param context Optional context from which the callback originated;
 *                commonly a pointer to a struct device.
 * @param result The result of the asynchronous call.
 * @param data Data to be given back to the callback.
 */
typedef void (*async_callback_t)(void *context, int result, void *data);

/**
 * Semaphore completion object with return code.
 */
struct async_sem {
	/** Result of async call */
	int result;

	/** Semaphore that will be given on completion */
	struct k_sem *sem;
};

/**
 * Asynchronous semaphore completion callback
 *
 * Sets the result and gives the semaphore.
 *
 * @funcprops \isr_ok
 *
 * @param context Optional context from which the callback originated,
 *                commonly a struct device*.
 * @param result Result to assign to @ref async_sem.result
 * @param sem_data Pointer to a @ref async_sem
 */
void async_semaphore_cb(void *context, int result, void *sem_data);

/**
 * Signal completion callback
 *
 * Sets the result and raises a k_poll_signal
 *
 * @funcprops \isr_ok
 *
 * @param context Optional context from which the callback originated,
 *                commonly a struct device*.
 * @param result Result to assign to @ref k_poll_signal.result
 * @param sig_data Pointer to a @ref k_poll_signal
 */
void async_signal_cb(void *context, int result, void *sig_data);

/**
 * Mutex completion object with return code.
 */
struct async_mutex {
	/** Result of async call */
	int result;

	/** Mutex that will be unlocked on  completion */
	struct k_mutex *mutex;
};

/**
 * Asynchronous mutex completion callback
 *
 * Sets the result and unlocks the mutex
 *
 * @funcprops \isr_ok
 *
 * @param context Optional context from which the callback originated,
 *                commonly a struct device*.
 * @param result Result to assign to @ref async_mutex.result
 * @param mutex_data Pointer to @ref async_mutex
 */
void async_mutex_cb(void *context, int result, void *mutex_data);

/**
 * Work completion object with return code.
 */
struct async_work {
	/** Result of async call */
	int result;

	/** Work that will be submitted on completion */
	struct k_work *work;

	/** Optional work queue to submit to, if NULL uses the global work queue */
	struct k_work_q *workq;
};

/**
 * Asynchronous work completion callback
 *
 * Submits an associated k_work task to a work queue. Uses
 * the global work queue if @ref async_work.workq is NULL.
 *
 * @funcprops \isr_ok
 *
 * @param context Optional context from which the callback originated,
 *                commonly a struct device*.
 * @param result Result to assign to @ref async_work.result
 * @param work_data Pointer to @ref async_work
 */
void async_work_cb(void *context, int result, void *work_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ASYNC_H_ */
