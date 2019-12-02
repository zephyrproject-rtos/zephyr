/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <ctype.h>
#include <kernel.h>
#include <device.h>
#include <assert.h>
#include <drivers/uart.h>
#include <tracing_backend.h>
#include <debug/tracing_core.h>

static const struct tracing_backend tracing_backend_uart;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static u32_t cur;
static struct tracing_packet *cmd;

static void uart_isr(struct device *dev)
{
	u8_t byte;
	int rx;

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
			cmd = tracing_packet_alloc();
			if (!cmd) {
				return;
			}
			cmd->direction = TRACING_IN;
		}

		if (!isprint(byte)) {
			switch (byte) {
			case '\r':
				cmd->buf[cur] = '\0';
				cmd->length = cur;
				tracing_list_add_packet(cmd);
				cmd = NULL;
				cur = 0U;
				break;
			default:
				break;
			}

			continue;
		}

		if (cur < sizeof(cmd->buf) - 1) {
			cmd->buf[cur++] = byte;
		}
	}
}
#endif

static void tracing_backend_uart_output(const struct tracing_backend *backend,
		struct tracing_packet *packet)
{
	struct device *dev = backend->cb->ctx;

	for (u32_t i = 0; i < packet->length; i++) {
		uart_poll_out(dev, packet->buf[i]);
	}
}

static void tracing_backend_uart_init(void)
{
	struct device *dev;

	dev = device_get_binding(CONFIG_TRACING_BACKEND_UART_NAME);
	assert(dev);

	tracing_backend_ctx_set(&tracing_backend_uart, dev);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_rx_disable(dev);
	uart_irq_tx_disable(dev);

	uart_irq_callback_set(dev, uart_isr);

	while (uart_irq_rx_ready(dev)) {
		u8_t c;

		uart_fifo_read(dev, &c, 1);
	}

	uart_irq_rx_enable(dev);
#endif
}

const struct tracing_backend_api tracing_backend_uart_api = {
	.init = tracing_backend_uart_init,
	.output  = tracing_backend_uart_output
};

TRACING_BACKEND_DEFINE(tracing_backend_uart, tracing_backend_uart_api);
