/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <ctype.h>
#include <kernel.h>
#include <device.h>
#include <drivers/uart.h>
#include <sys/__assert.h>
#include <tracing_core.h>
#include <tracing_buffer.h>
#include <tracing_backend.h>

static struct device *tracing_uart_dev;

#ifdef CONFIG_TRACING_HANDLE_HOST_CMD
static void uart_isr(struct device *dev, void *user_data)
{
	int rx;
	uint8_t byte;
	static uint8_t *cmd;
	static uint32_t length, cur;

	ARG_UNUSED(user_data);

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (!uart_irq_rx_ready(dev)) {
			continue;
		}

		rx = uart_fifo_read(dev, &byte, 1);
		if (rx < 0) {
			uart_irq_rx_disable(dev);
			return;
		}

		if (!cmd) {
			length = tracing_cmd_buffer_alloc(&cmd);
		}

		if (!isprint(byte)) {
			if (byte == '\r') {
				cmd[cur] = '\0';
				tracing_cmd_handle(cmd, cur);
				cmd = NULL;
				cur = 0U;
			}

			continue;
		}

		if (cur < length - 1) {
			cmd[cur++] = byte;
		}
	}
}
#endif

static void tracing_backend_uart_output(
		const struct tracing_backend *backend,
		uint8_t *data, uint32_t length)
{
	for (uint32_t i = 0; i < length; i++) {
		uart_poll_out(tracing_uart_dev, data[i]);
	}
}

static void tracing_backend_uart_init(void)
{
	tracing_uart_dev =
		device_get_binding(CONFIG_TRACING_BACKEND_UART_NAME);
	__ASSERT(tracing_uart_dev, "uart backend binding failed");

#ifdef CONFIG_TRACING_HANDLE_HOST_CMD
	uart_irq_rx_disable(tracing_uart_dev);
	uart_irq_tx_disable(tracing_uart_dev);

	uart_irq_callback_set(tracing_uart_dev, uart_isr);

	while (uart_irq_rx_ready(tracing_uart_dev)) {
		uint8_t c;

		uart_fifo_read(tracing_uart_dev, &c, 1);
	}

	uart_irq_rx_enable(tracing_uart_dev);
#endif
}

const struct tracing_backend_api tracing_backend_uart_api = {
	.init = tracing_backend_uart_init,
	.output  = tracing_backend_uart_output
};

TRACING_BACKEND_DEFINE(tracing_backend_uart, tracing_backend_uart_api);
