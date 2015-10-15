/** @file
 * @brief Simple UART driver
 *
 * A simple UART driver allowing application to handle all aspects of received
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
#include <arch/cpu.h>

#include <board.h>
#include <uart.h>

#include <simple/uart.h>
#include <misc/printk.h>

#define UART (uart_devs[CONFIG_UART_SIMPLE_INDEX])

static uint8_t *recv_buf;
static size_t recv_buf_len;
static uart_simple_recv_cb app_cb;
static size_t recv_off;

void uart_simple_isr(void *unused)
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

		/* Call application callback with received data. Application
		 * may provide new buffer or alter data offset.
		 */
		recv_off += rx;
		recv_buf = app_cb(recv_buf, &recv_off);
	}
}

int uart_simple_send(const uint8_t *data, int len)
{
	return uart_fifo_fill(UART, data, len);
}

IRQ_CONNECT_STATIC(uart_simple, CONFIG_UART_SIMPLE_IRQ,
			CONFIG_UART_SIMPLE_INT_PRI, uart_simple_isr, 0);

static void uart_simple_setup(struct device *uart, struct uart_init_info *info)
{
	uart_init(uart, info);

	uart_irq_rx_disable(uart);
	uart_irq_tx_disable(uart);
	IRQ_CONFIG(uart_simple, uart_irq_get(uart), 0);
	irq_enable(uart_irq_get(uart));

	/* Drain the fifo */
	while (uart_irq_rx_ready(uart)) {
		unsigned char c;
		uart_fifo_read(uart, &c, 1);
	}

	uart_irq_rx_enable(uart);
}

void uart_simple_register(uint8_t *buf, size_t len, uart_simple_recv_cb cb)
{
	struct uart_init_info info = {
		.options = 0,
		.sys_clk_freq = CONFIG_UART_SIMPLE_FREQ,
		.baud_rate = CONFIG_UART_SIMPLE_BAUDRATE,
		.irq_pri = CONFIG_UART_SIMPLE_INT_PRI,
	};

	recv_buf = buf;
	recv_buf_len = len;
	app_cb = cb;

	uart_simple_setup(UART, &info);
}
