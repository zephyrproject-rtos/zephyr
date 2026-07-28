/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_CONTEXT_H_
#define TEST_CONTEXT_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>

/** @brief Payload size of @ref api_test_msg data field. */
#define API_TEST_MSG_DATA_SIZE 16

/**
 * @brief IPC service API test command identifiers.
 */
enum api_test_cmd {
	API_TEST_CMD_PING = 1,   /**< Request remote readiness check */
	API_TEST_CMD_PONG,       /**< Response to @ref API_TEST_CMD_PING */
	API_TEST_CMD_ECHO,       /**< Request echo of message payload */
	API_TEST_CMD_ECHO_RSP,   /**< Echo response */
	API_TEST_CMD_DEREGISTER, /**< Request endpoint deregistration */
};

/**
 * @brief IPC service API test message payload.
 */
struct api_test_msg {
	uint32_t cmd;                         /**< Command of type @ref api_test_cmd */
	uint8_t data[API_TEST_MSG_DATA_SIZE]; /**< Optional message data */
};

/**
 * @brief Test endpoint identifiers.
 */
enum {
	TEST_EP0 = 0,  /**< First test endpoint */
	TEST_EP1 = 1,  /**< Second test endpoint */
	TEST_EP_COUNT, /**< Total number of test endpoints */
};

/**
 * @brief Per-endpoint state shared between host and remote test images.
 */
struct test_context {
	struct ipc_ept ep;                            /**< IPC endpoint instance */
	struct ipc_ept_cfg ep_cfg;                    /**< IPC endpoint configuration */
	uint8_t rx_data[sizeof(struct api_test_msg)]; /**< Last received message buffer */
	size_t rx_len;                                /**< Length of data in @ref rx_data */
	const void *rx_hold_buf;                      /**< Held no-copy RX buffer, if any */
	volatile bool rx_pending;                     /**< Set when an RX message is pending */
	volatile bool rx_overflow;                    /**< Set when RX buffer overflow occurred */
	volatile bool bound;                          /**< Set when the endpoint is bound */
	volatile bool unbound;                        /**< Set when the endpoint is unbound */
};

/**
 * @brief Poll until a volatile flag becomes true or timeout expires.
 *
 * Function supports no-multithreading mode.
 *
 * @param flag Pointer to the flag to wait on.
 * @param timeout_ms Maximum wait time in milliseconds.
 *
 * @return true if the flag was set before timeout, false otherwise.
 */
static inline bool wait_for_flag(volatile bool *flag, uint32_t timeout_ms)
{
	uint32_t start = k_uptime_get_32();

	while (!*flag) {
		k_msleep(1);
		if ((k_uptime_get_32() - start) >= timeout_ms) {
			return false;
		}
	}

	return true;
}

#endif /* TEST_CONTEXT_H_ */
