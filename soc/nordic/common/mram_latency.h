/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * nRF SoC specific public APIs for MRAM latency management
 * @brief Experimental. It will be replaced by the PM latency policy API in the future.
 */

#ifndef SOC_NORDIC_COMMON_MRAM_LATENCY_H_
#define SOC_NORDIC_COMMON_MRAM_LATENCY_H_

#include <zephyr/sys/onoff.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @internal For test purposes only. */
extern struct onoff_manager mram_latency_mgr;

/** @brief Request MRAM operations without latency.
 *
 * The return value indicates the success or failure of an attempt to initiate
 * an operation to request the MRAM low latency. If initiation of the
 * operation succeeds, the result of the request operation is provided through
 * the configured client notification method, possibly before this call returns.
 *
 * @param cli pointer to client state providing instructions on synchronous
 *            expectations and how to notify the client when the request
 *            completes. Behavior is undefined if client passes a pointer
 *            object associated with an incomplete service operation.
 *
 * @retval non-negative the observed state of the on-off service associated
 *                      with the MRAM latency service.
 * @retval -EIO if MRAM latency service returned error.
 * @retval -EINVAL if the parameters are invalid.
 * @retval -EAGAIN if the reference count would overflow.
 */
int mram_no_latency_request(struct onoff_client *cli);

/** @brief Request MRAM operations without latency.
 *
 * Request is synchronous and blocks until it is completed. It can be called only
 * from the thread context and cannot be called in the pre kernel stage.
 *
 * @retval 0 on successful request.
 * @retval -EIO if MRAM latency service returned error.
 * @retval -EAGAIN if request was not completed on time.
 */
int mram_no_latency_sync_request(void);

/**
 * @brief Safely cancel a request for MRAM operations without latency.
 *
 * It may be that a client has issued a reservation request but needs to
 * shut down before the request has completed. This function attempts to
 * cancel the request and issues a release if cancellation fails because
 * the request was completed. This synchronously ensures that ownership
 * data reverts to the client so is available for a future request.
 *
 * @param cli a pointer to the same client state that was provided
 *            when the operation to be cancelled was issued.
 *
 * @retval ONOFF_STATE_TO_ON if the cancellation occurred before the transition
 *                           completed.
 * @retval ONOFF_STATE_ON if the cancellation occurred after the transition
 *                        completed.
 * @retval -EINVAL if the parameters are invalid.
 * @retval -EIO if MRAM latency service returned error.
 * @retval negative other errors produced by onoff_release().
 */
int mram_no_latency_cancel_or_release(struct onoff_client *cli);

/**
 * @brief Release a request for MRAM operations without latency.
 *
 * It should match with a completed @ref mram_no_latency_sync_request call.
 *
 * @retval 0 on successful request.
 * @retval -EIO if MRAM latency service returned error.
 */
int mram_no_latency_sync_release(void);

#ifdef __cplusplus
}
#endif

#endif /* SOC_NORDIC_COMMON_MRAM_LATENCY_H_ */
