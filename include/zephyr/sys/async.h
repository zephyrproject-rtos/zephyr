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
 * Common function pointer type used for asynchronous completion callbacks.
 * Akin to k_poll_signal as a callback rather than a pollable object. If
 * additional context around the callback is needed then a container
 * struct for data should be used and CONTAINER_OF to extract the added
 * context.
 *
 * The prime example here would be a k_poll_signal that is waiting on a particular
 * device event. In which case the k_poll_signal should be placed inside another struct
 * with the required context.
 *
 * @note Implementations of this callback should be ISR ok.
 *
 * @param result The result of the asynchronous call.
 * @param data Data to be given back to the callback.
 */
typedef void (*async_callback_t)(int result, void *data);

/**
 * Signal completion callback
 *
 * Raises a k_poll_signal with the given result. The context parameter is intentionally
 * not forwarded in any way to enable maximum compatibility with existing k_poll_signal
 * uses. The intended use of this callback is to provide a nice way of moving from
 * k_poll_signal which is a common asynchronous notification mechanism in async calls
 * to a callback notification mechanism.
 *
 * @funcprops \isr_ok
 *
 * @param result Result to assign to @ref k_poll_signal.result
 * @param signal Pointer to a @ref k_poll_signal
 */
void async_signal_cb(int result, void *signal);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ASYNC_H_ */
