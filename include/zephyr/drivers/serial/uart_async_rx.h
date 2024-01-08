/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Helper module for receiving using UART Asynchronous API.
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_UART_ASYNC_RX_H_
#define ZEPHYR_DRIVERS_SERIAL_UART_ASYNC_RX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>

/* @brief RX buffer structure which holds the buffer and its state. */
struct uart_async_rx_buf {
	/* Write index which is incremented whenever new data is reported to be
	 * received to that buffer.
	 */
	uint8_t wr_idx;

	/* Read index which is incremented whenever data is consumed from the buffer.
	 * Read index cannot be higher than the write index.
	 */
	uint8_t rd_idx;

	/* Set to one if buffer is released by the driver. */
	uint8_t completed;

	/* Location which is passed to the UART driver. */
	uint8_t buffer[];
};

/** @brief UART asynchronous RX helper structure. */
struct uart_async_rx {
	/* Pointer to the configuration structure. Structure must be persistent. */
	const struct uart_async_rx_config *config;

	/* Total amount of pending bytes. Bytes may be spread across multiple RX buffers. */
	atomic_t pending_bytes;

	/* Number of buffers which are free. */
	atomic_t free_buf_cnt;

	/* Single buffer size. */
	uint8_t buf_len;

	/* Index of the next buffer to be provided to the driver. */
	uint8_t drv_buf_idx;

	/* Current buffer to which data is written. */
	uint8_t wr_buf_idx;

	/* Current buffer from which data is being consumed. */
	uint8_t rd_buf_idx;
};

/** @brief UART asynchronous RX helper configuration structure. */
struct uart_async_rx_config {
	/* Pointer to the buffer. */
	uint8_t *buffer;

	/* Buffer length. */
	size_t length;

	/* Number of buffers into provided space shall be split. */
	uint8_t buf_cnt;
};

/** @brief Get RX buffer length.
 *
 * @param async_rx Pointer to the helper instance.
 *
 * @return Buffer length.
 */
static inline uint8_t uart_async_rx_get_buf_len(struct uart_async_rx *async_rx)
{
	return async_rx->buf_len;
}

/** @brief Get amount of space dedicated for managing each buffer state.
 *
 * User buffer provided during the initialization is split into chunks and each
 * chunk has overhead. This overhead can be used to calculate actual space used
 * for UART data.
 *
 * @return Overhead space in bytes.
 */
#define UART_ASYNC_RX_BUF_OVERHEAD offsetof(struct uart_async_rx_buf, buffer)

/** @brief Initialize the helper instance.
 *
 * @param async_rx Pointer to the helper instance.
 * @param config   Configuration. Must be persistent.
 *
 * @retval 0 on successful initialization.
 */
int uart_async_rx_init(struct uart_async_rx *async_rx,
		       const struct uart_async_rx_config *config);

/** @brief Reset state of the helper instance.
 *
 * Helper can be reset after RX abort to discard all received data and bring
 * the helper to its initial state.
 *
 * @param async_rx Pointer to the helper instance.
 */
void uart_async_rx_reset(struct uart_async_rx *async_rx);

/** @brief Indicate received data.
 *
 * Function shall be called from @ref UART_RX_RDY context.
 *
 * @param async_rx Pointer to the helper instance.
 * @param buffer Buffer received in the UART driver event.
 * @param length Length received in the UART driver event.
 */
void uart_async_rx_on_rdy(struct uart_async_rx *async_rx, uint8_t *buffer, size_t length);

/** @brief Get next RX buffer.
 *
 * Returned pointer shall be provided to @ref uart_rx_buf_rsp or @ref uart_rx_enable.
 * If null is returned that indicates that there are no available buffers since all
 * buffers are used by the driver or contain not consumed data.
 *
 * @param async_rx Pointer to the helper instance.
 *
 * @return Pointer to the next RX buffer or null if no buffer available.
 */
uint8_t *uart_async_rx_buf_req(struct uart_async_rx *async_rx);

/** @brief Indicate that buffer is no longer used by the UART driver.
 *
 * Function shall be called on @ref UART_RX_BUF_RELEASED event.
 *
 * @param async_rx Pointer to the helper instance.
 * @param buf Buffer pointer received in the UART driver event.
 */
void uart_async_rx_on_buf_rel(struct uart_async_rx *async_rx, uint8_t *buf);

/** @brief Claim received data for processing.
 *
 * Helper module works in the zero copy mode. It provides a pointer to the buffer
 * that was directly used by the UART driver. Since received data is spread across
 * multiple buffers there is no possibility to read all data at once. It can only be
 * consumed in chunks. After data is processed, @ref uart_async_rx_data_consume is
 * used to indicate that data is consumed.
 *
 * @param async_rx Pointer to the helper instance.
 * @param data Location where address to the buffer is written. Untouched if no data to claim.
 * @param length Amount of requested data.
 *
 * @return Amount valid of data in the @p data buffer. 0 is returned when there is no data.
 */
size_t uart_async_rx_data_claim(struct uart_async_rx *async_rx, uint8_t **data, size_t length);

/** @brief Consume claimed data.
 *
 * It pairs with @ref uart_async_rx_data_claim.
 *
 * @param async_rx Pointer to the helper instance.
 * @param length Amount of data to consume. It must be less or equal than amount of claimed data.
 */
void uart_async_rx_data_consume(struct uart_async_rx *async_rx, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SERIAL_UART_ASYNC_RX_H_ */
