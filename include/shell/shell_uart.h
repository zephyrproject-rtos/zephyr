/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_UART_H__
#define SHELL_UART_H__

#include <shell/shell.h>
#include <sys/ring_buffer.h>
#include <sys/atomic.h>
#include "mgmt/mcumgr/smp_shell.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const struct shell_transport_api shell_uart_transport_api;

/** @brief Shell UART transport instance control block (RW data). */
struct shell_uart_ctrl_blk {
	const struct device *dev;
	shell_transport_handler_t handler;
	void *context;
	atomic_t tx_busy;
	bool blocking_tx;
#ifdef CONFIG_MCUMGR_SMP_SHELL
	struct smp_shell_data smp;
#endif /* CONFIG_MCUMGR_SMP_SHELL */
};

#ifdef CONFIG_SHELL_BACKEND_SERIAL_INTERRUPT_DRIVEN
#define Z_UART_SHELL_TX_RINGBUF_DECLARE(_name, _size) \
	RING_BUF_DECLARE(_name##_tx_ringbuf, _size)

#define Z_UART_SHELL_RX_TIMER_DECLARE(_name) /* Empty */
#define Z_UART_SHELL_TX_RINGBUF_PTR(_name) (&_name##_tx_ringbuf)

#define Z_UART_SHELL_RX_TIMER_PTR(_name) NULL

#else /* CONFIG_SHELL_BACKEND_SERIAL_INTERRUPT_DRIVEN */
#define Z_UART_SHELL_TX_RINGBUF_DECLARE(_name, _size) /* Empty */
#define Z_UART_SHELL_RX_TIMER_DECLARE(_name) static struct k_timer _name##_timer
#define Z_UART_SHELL_TX_RINGBUF_PTR(_name) NULL
#define Z_UART_SHELL_RX_TIMER_PTR(_name) (&_name##_timer)
#endif /* CONFIG_SHELL_BACKEND_SERIAL_INTERRUPT_DRIVEN */

/** @brief Shell UART transport instance structure. */
struct shell_uart {
	struct shell_uart_ctrl_blk *ctrl_blk;
	struct k_timer *timer;
	struct ring_buf *tx_ringbuf;
	struct ring_buf *rx_ringbuf;
};

/** @brief Macro for creating shell UART transport instance. */
#define SHELL_UART_DEFINE(_name, _tx_ringbuf_size, _rx_ringbuf_size)	\
	static struct shell_uart_ctrl_blk _name##_ctrl_blk;		\
	Z_UART_SHELL_RX_TIMER_DECLARE(_name);				\
	Z_UART_SHELL_TX_RINGBUF_DECLARE(_name, _tx_ringbuf_size);	\
	RING_BUF_DECLARE(_name##_rx_ringbuf, _rx_ringbuf_size);		\
	static const struct shell_uart _name##_shell_uart = {		\
		.ctrl_blk = &_name##_ctrl_blk,				\
		.timer = Z_UART_SHELL_RX_TIMER_PTR(_name),		\
		.tx_ringbuf = Z_UART_SHELL_TX_RINGBUF_PTR(_name),	\
		.rx_ringbuf = &_name##_rx_ringbuf,			\
	};								\
	struct shell_transport _name = {				\
		.api = &shell_uart_transport_api,			\
		.ctx = (struct shell_uart *)&_name##_shell_uart		\
	}

/**
 * @brief This function provides pointer to shell uart backend instance.
 *
 * Function returns pointer to the shell uart instance. This instance can be
 * next used with shell_execute_cmd function in order to test commands behavior.
 *
 * @returns Pointer to the shell instance.
 */
const struct shell *shell_backend_uart_get_ptr(void);

/**
 * @typedef shell_uart_rx_callback_t
 * @brief Define the RX callback when forking the shell UART via
 * shell_backend_uart_fork_rx()
 *
 * @param dev		UART device structure.
 * @param user_data	Pointer to data specified by user.
 * @param data		Pointer to the start of the data coming from RX (this
 *			address range will be recycled, so data should be
 *			copied to persist).
 * @param length	Number of bytes available starting at {@code data}.
 */
typedef void (*shell_uart_rx_callback_t)(const struct device *dev,
	void *user_data, uint8_t *data, size_t length);

/**
 * @brief Fork the RX data obtained by the shell.
 *
 * When non {@code NULL}, the callback will be notified of any new data
 * received on this shell's instance. Note that a new callback will override
 * the previous and passing {@code NULL} will stop the fork.
 *
 * @param callback	The function to call when new data is available.
 * @param user_data	Pointer to data to pass into the callbeck when called.
 */
void shell_backend_uart_fork_rx(shell_uart_rx_callback_t callback,
	void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_UART_H__ */
