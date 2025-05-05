/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_NRF_IRONSIDE_CALL_H_
#define ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_NRF_IRONSIDE_CALL_H_

#include <stdint.h>

/** @brief Maximum number of arguments to an IRONside call.
 *
 * This is chosen so that the containing message buffer size is minimal but
 * cache line aligned.
 */
#define NRF_IRONSIDE_CALL_NUM_ARGS 7

/** @brief Message buffer. */
struct ironside_call_buf {
	/** Status code. This is set by the API. */
	uint16_t status;
	/** Operation identifier. This is set by the user. */
	uint16_t id;
	/** Operation arguments. These are set by the user. */
	uint32_t args[NRF_IRONSIDE_CALL_NUM_ARGS];
};

/**
 * @name Message buffer status codes.
 * @{
 */

/** Buffer is idle and available for allocation. */
#define IRONSIDE_CALL_STATUS_IDLE                   0
/** Request was processed successfully by the server. */
#define IRONSIDE_CALL_STATUS_RSP_SUCCESS            1
/** Request status code is unknown. */
#define IRONSIDE_CALL_STATUS_RSP_ERR_UNKNOWN_STATUS 2
/** Request status code is no longer supported. */
#define IRONSIDE_CALL_STATUS_RSP_ERR_EXPIRED_STATUS 3
/** Operation identifier is unknown. */
#define IRONSIDE_CALL_STATUS_RSP_ERR_UNKNOWN_ID     4
/** Operation identifier is no longer supported. */
#define IRONSIDE_CALL_STATUS_RSP_ERR_EXPIRED_ID     5
/** Buffer contains a request from the client. */
#define IRONSIDE_CALL_STATUS_REQ                    6

/**
 * @}
 */

/**
 * @brief Allocate memory for an IRONside call.
 *
 * This function will block when no buffers are available, until one is
 * released by another thread on the client side.
 *
 * @return Pointer to the allocated buffer.
 */
struct ironside_call_buf *ironside_call_alloc(void);

/**
 * @brief Dispatch an IRONside call.
 *
 * This function will block until a response is received from the server.
 *
 * @param buf Buffer returned by ironside_call_alloc(). It should be populated
 *            with request data before calling this function. Upon returning,
 *            this data will have been replaced by response data.
 */
void ironside_call_dispatch(struct ironside_call_buf *buf);

/**
 * @brief Release an IRONside call buffer.
 *
 * This function must be called after processing the response.
 *
 * @param buf Buffer used to perform the call.
 */
void ironside_call_release(struct ironside_call_buf *buf);

#endif /* ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_NRF_IRONSIDE_CALL_H_ */
