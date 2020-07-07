/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_H4_UART_H_
#define ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_H4_UART_H_

/**
 * @brief UART asynchronous transport for HCI H4
 * @defgroup bt_h4_uart Asynchronous UART transport
 * @ingroup bluetooth
 * @{
 */

#include <stdbool.h>
#include <net/buf.h>
#include <bluetooth/buf.h>
#include <drivers/uart.h>
#include <device.h>
#include <sys/ring_buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define H4_NONE 0x00
#define H4_CMD  0x01
#define H4_ACL  0x02
#define H4_SCO  0x03
#define H4_EVT  0x04
#define H4_INV  0xff

/** @brief If flag is set then type byte is send before buffer content. */
#define H4_UART_TX_ADD_TYPE BIT(0)

/** @brief Set if transport is sending type byte. */
#define H4_UART_TX_TYPE_IN_PROGRESS BIT(1)

/* Forward declarations. */
struct h4_uart;
struct h4_uart_rx;

/** @brief Prototype of process function.
 *
 * Process function is called from RX thread context. Use h4_uart_read to
 * get available data.
 */
typedef void (*h4_uart_process_t)(struct h4_uart *transport);

/** @brief Configuration of TX part. */
struct h4_uart_config_tx {
	uint32_t timeout;

	/** @brief Set to true to send type byte before provided content. */
	bool add_type;
};

/** @brief Configuration of RX part. */
struct h4_uart_config_rx {
	/** @brief Process function */
	h4_uart_process_t process;

	/** @brief Pointer to the memory area used for RX thread stack. */
	k_thread_stack_t *stack;

	/** @brief Size of RX thread stack. */
	size_t rx_thread_stack_size;

	/** @brief RX thread priority. */
	uint32_t thread_prio;
};

/** @brief Configuration structure. */
struct h4_uart_config {
	struct h4_uart_config_rx rx;
	struct h4_uart_config_tx tx;
};

/** @brief Receiver part. */
struct h4_uart_rx {
	struct k_sem sem;
	struct k_spinlock lock;
	h4_uart_process_t process;
	struct k_thread thread;

	bool enabled;
	bool stopped;
	uint8_t buf_space[CONFIG_BT_H4_UART_RX_BUFFER_SIZE];
	struct ring_buf buffer;
};

/** @brief Transmitter part. */
struct h4_uart_tx {
	struct k_fifo fifo;
	struct net_buf *curr;

	uint32_t timeout;
	uint8_t type;
	uint32_t flags;
};

/** @brief H4 UART transport instance. */
struct h4_uart {
	struct device *dev;
	struct h4_uart_rx rx;
	struct h4_uart_tx tx;
};

/** @brief Read data.
 *
 * It should be called from process function which is called from RX thread
 * context.
 *
 * @param transport	Transport instance.
 * @param dst		Location to store received data.
 * @param req_len	Amount of bytes to read.
 *
 * @return Number of actually returned bytes.
 */
size_t h4_uart_read(struct h4_uart *transport,
			   uint8_t *dst, size_t req_len);

/** @brief Write buffer.
 *
 * Send content of the buffer over UART. Net buffer is released on completion.
 *
 * @param transport	Transport instance.
 * @param buf		Buffer.
 *
 * @return Negative value on error, non-negative value on success.
 */
int h4_uart_write(struct h4_uart *transport, struct net_buf *buf);

/** @brief Initialize H4 UART transport.
 *
 * @param transport	Transport instance.
 * @param dev		UART device.
 * @param config	Configuration.
 *
 * @return Negative value on error, non-negative value on success.
 */
int h4_uart_init(struct h4_uart *transport, struct device *dev,
		 const struct h4_uart_config *config);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_H4_UART_H_ */
