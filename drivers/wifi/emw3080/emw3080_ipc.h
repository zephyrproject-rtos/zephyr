/*
 * Copyright (c) 2026 Witekio
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_EMW3080_IPC_H
#define ZEPHYR_DRIVERS_WIFI_EMW3080_IPC_H

#include <stddef.h>
#include <stdint.h>

#define EMW3080_IPC_HDR_LEN    6
#define EMW3080_IPC_MAX_PKT    2500
#define EMW3080_IPC_MAX_PARAMS (EMW3080_IPC_MAX_PKT - EMW3080_IPC_HDR_LEN)

#define EMW3080_IPC_OK 0

/* Called from the SPI transport thread; must not block. */
typedef void (*emw3080_ipc_event_cb_t)(uint16_t api_id, const uint8_t *payload, size_t len,
				       void *user);

/**
 * @brief Initialize the IPC layer and the SPI transport below it.
 *
 * @retval 0 Success.
 * @retval -errno Transport initialization error, see emw3080_spi_init().
 */
int emw3080_ipc_init(emw3080_ipc_event_cb_t event_cb, void *user);

/**
 * @brief Send a command and wait for its response.
 *
 * Only one command is outstanding at a time; concurrent callers are
 * serialized.
 *
 * If @p res_len is NULL, the response must be exactly @p res_size bytes.
 * Otherwise any response up to @p res_size bytes is accepted and its length
 * is written to @p res_len.
 *
 * @param api_id Command identifier.
 * @param req Command parameters, may be NULL if @p req_len is 0.
 * @param req_len Size of the command parameters.
 * @param res Response buffer, may be NULL if @p res_size is 0.
 * @param res_size Size of the response buffer.
 * @param res_len Received response length, or NULL to require exactly
 *                @p res_size bytes. Only written on success.
 * @param timeout_ms Timeout covering serialization, transmission and response.
 *
 * @retval EMW3080_IPC_OK Response received and copied to @p res.
 * @retval -EINVAL Invalid buffer.
 * @retval -EMSGSIZE Command too large, or response larger than @p res_size.
 * @retval -EBADMSG Response shorter than @p res_size while @p res_len is NULL.
 * @retval -ETIMEDOUT IPC busy, or no response before the timeout.
 * @retval -EPROTO Invalid peripheral header.
 * @retval -EIO Transport error.
 */
int emw3080_ipc_request(uint16_t api_id, const void *req, uint16_t req_len, void *res,
			uint16_t res_size, uint16_t *res_len, uint32_t timeout_ms);

#endif /* ZEPHYR_DRIVERS_WIFI_EMW3080_IPC_H */
