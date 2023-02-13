/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_UART_H__
#define SHELL_UART_H__

#include <zephyr/shell/shell.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/mgmt/mcumgr/transport/smp_shell.h>

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
#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL
	struct smp_shell_data smp;
#endif /* CONFIG_MCUMGR_TRANSPORT_SHELL */
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

#ifdef __cplusplus
}
#endif

#endif /* SHELL_UART_H__ */
