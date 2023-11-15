/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_UART_H__
#define SHELL_UART_H__

#include <zephyr/drivers/serial/uart_async_rx.h>
#include <zephyr/device.h>
#include <zephyr/mgmt/mcumgr/transport/smp_shell.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

struct shell_uart_common {
	const struct device *dev;
	shell_transport_handler_t handler;
	void *context;
	bool blocking_tx;
#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL
	struct smp_shell_data smp;
#endif /* CONFIG_MCUMGR_TRANSPORT_SHELL */
};

struct shell_uart_int_driven {
	struct shell_uart_common common;
	struct ring_buf tx_ringbuf;
	struct ring_buf rx_ringbuf;
	struct k_timer dtr_timer;
	atomic_t tx_busy;
};

struct shell_uart_async {
	struct shell_uart_common common;
	struct k_sem tx_sem;
	struct uart_async_rx async_rx;
	atomic_t pending_rx_req;
};

struct shell_uart_polling {
	struct shell_uart_common common;
	struct ring_buf rx_ringbuf;
	struct k_timer rx_timer;
};

/**
 * @brief This function provides pointer to the shell UART backend instance.
 *
 * Function returns pointer to the shell UART instance. This instance can be
 * next used with shell_execute_cmd function in order to test commands behavior.
 *
 * @returns Pointer to the shell instance.
 */
const struct shell *shell_backend_uart_get_ptr(void);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_UART_H__ */
