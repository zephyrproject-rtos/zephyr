/** @file
 * @brief Pipe UART driver
 *
 * A pipe UART driver allowing application to handle all aspects of received
 * protocol data.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>

#include <board.h>
#include <uart.h>

#include <console/uart_pipe.h>
#include <misc/printk.h>

#if !defined(CONFIG_UART_PIPE_INDEX) || !defined(CONFIG_UART_PIPE_IRQ)
#error One or more required values for this driver are not set.
#else
#define UART (uart_devs[CONFIG_UART_PIPE_INDEX])

static uint8_t *recv_buf;
static size_t recv_buf_len;
static uart_pipe_recv_cb app_cb;
static size_t recv_off;

void uart_pipe_isr(void *unused)
{
	ARG_UNUSED(unused);

	while (uart_irq_update(UART) && uart_irq_is_pending(UART)) {
		int rx;

		if (!uart_irq_rx_ready(UART)) {
			continue;
		}

		rx = uart_fifo_read(UART, recv_buf + recv_off,
				    recv_buf_len - recv_off);
		if (!rx) {
			continue;
		}

		/*
		 * Call application callback with received data. Application
		 * may provide new buffer or alter data offset.
		 */
		recv_off += rx;
		recv_buf = app_cb(recv_buf, &recv_off);
	}
}

int uart_pipe_send(const uint8_t *data, int len)
{
	return uart_fifo_fill(UART, data, len);
}

IRQ_CONNECT_STATIC(uart_pipe, CONFIG_UART_PIPE_IRQ,
		   CONFIG_UART_PIPE_INT_PRI, uart_pipe_isr, 0,
		   UART_IRQ_FLAGS);

static void uart_pipe_setup(struct device *uart)
{
	uart_irq_rx_disable(uart);
	uart_irq_tx_disable(uart);
	IRQ_CONFIG(uart_pipe, uart_irq_get(uart), 0);
	irq_enable(uart_irq_get(uart));

	/* Drain the fifo */
	while (uart_irq_rx_ready(uart)) {
		unsigned char c;

		uart_fifo_read(uart, &c, 1);
	}

	uart_irq_rx_enable(uart);
}

void uart_pipe_register(uint8_t *buf, size_t len, uart_pipe_recv_cb cb)
{
	recv_buf = buf;
	recv_buf_len = len;
	app_cb = cb;

	uart_pipe_setup(UART);
}
#endif
