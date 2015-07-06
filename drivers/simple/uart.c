/** @file
 * @brief Simple UART driver
 *
 * A simple UART driver allowing application to handle all aspects of received
 * protocol data.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <arch/cpu.h>

#include <board.h>
#include <drivers/uart.h>

#include <simple/uart.h>
#include <misc/printk.h>

#define UART CONFIG_UART_SIMPLE_INDEX

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

static void uart_simple_setup(int uart, struct uart_init_info *info)
{
	uart_init(uart, info);

	uart_irq_rx_disable(uart);
	uart_irq_tx_disable(uart);
	IRQ_CONFIG(uart_simple, uart_irq_get(uart));
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
		.int_pri = CONFIG_UART_SIMPLE_INT_PRI,
	};

	recv_buf = buf;
	recv_buf_len = len;
	app_cb = cb;

	uart_simple_setup(UART, &info);
}
