/* uart_console.c - UART-driven console */

/*
 * Copyright (c) 2011-2012, 2014-2015 Wind River Systems, Inc.
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
 * 3) Neither the name of Wind River Systems nor the names of its contributors
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

/*
  DESCRIPTION

  Serial console driver.
  Hooks into the printk and fputc (for printf) modules. Poll driven.
*/

#include <nanokernel.h>
#include <nanokernel/cpu.h>

#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#include <board.h>
#include <drivers/uart.h>
#include <console/uart_console.h>
#include <toolchain.h>
#include <sections.h>

#define UART CONFIG_UART_CONSOLE_INDEX
#if (UART < 0) || !(UART < CONFIG_UART_NUM_SYSTEM_PORTS)
#error UART number not within ranges (0 to CONFIG_UART_NUM_SYSTEM_PORTS-1)
#endif

#if 0 /* NOTUSED */
/******************************************************************************
 *
 * consoleIn - get a character from UART
 *
 * RETURNS: the character or EOF if nothing present
 */

static int consoleIn(void)
{
	unsigned char c;
	if (uart_poll_in(UART, &c) < 0)
		return EOF;
	else
		return (int)c;
}
#endif

#if defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE)
/******************************************************************************
 *
 * consoleOut - output one character to UART
 *
 * Outputs both line feed and carriage return in the case of a '\n'.
 *
 * RETURNS: The character passed as input.
 */

static int consoleOut(int c /* character to output */
	)
{
	uart_poll_out(UART, (unsigned char)c);
	if ('\n' == c) {
		uart_poll_out(UART, (unsigned char)'\r');
	}
	return c;
}
#endif

#if defined(CONFIG_STDOUT_CONSOLE)
extern void __stdout_hook_install(int (*hook)(int));
#else
#define __stdout_hook_install(x)		\
	do {/* nothing */			\
	} while ((0))
#endif

#if defined(CONFIG_PRINTK)
extern void __printk_hook_install(int (*fn)(int));
#else
#define __printk_hook_install(x)		\
	do {/* nothing */			\
	} while ((0))
#endif

#if defined(CONFIG_CONSOLE_HANDLER)
static size_t pos = 0;

static struct nano_fifo *avail_queue;
static struct nano_fifo *lines_queue;

static int read_uart(int uart, uint8_t *buf, unsigned int size)
{
	int rx;

	rx = uart_fifo_read(uart, buf, size);
	if (rx < 0) {
		/* Overrun issue. Stop the UART */
		uart_irq_rx_disable(uart);

		return -EIO;
	}

	return rx;
}

void uart_console_isr(void *unused)
{
	ARG_UNUSED(unused);

	while (uart_irq_update(UART) && uart_irq_is_pending(UART)) {
		/* Character(s) have been received */
		if (uart_irq_rx_ready(UART)) {
			static struct uart_console_input *cmd;
			uint8_t byte;
			int rx;


			if (!cmd) {
				cmd = nano_isr_fifo_get(avail_queue);
				if (!cmd)
					return;
			}

			rx = read_uart(UART, &byte, 1);
			if (rx < 0) {
				nano_isr_fifo_put(avail_queue, cmd);
				return;
			}

			/* Echo back to console */
			uart_poll_out(UART, byte);

			if (byte == '\r' || byte == '\n' ||
			    pos == sizeof(cmd->line) - 1) {
				cmd->line[pos] = '\0';
				uart_poll_out(UART, '\n');
				pos = 0;

				nano_isr_fifo_put(lines_queue, cmd);
				cmd = NULL;
			} else {
				cmd->line[pos++] = byte;
			}

		}
	}
}

static void console_input_init(void)
{
	uint8_t c;

	uart_irq_rx_disable(UART);
	uart_irq_tx_disable(UART);
	uart_int_connect(UART, uart_console_isr, NULL, NULL);

	/* Drain the fifo */
	while (uart_irq_rx_ready(UART))
		uart_fifo_read(UART, &c, 1);

	uart_irq_rx_enable(UART);
}

void uart_register_input(struct nano_fifo *avail, struct nano_fifo *lines)
{
	avail_queue = avail;
	lines_queue = lines;

	console_input_init();
}
#else
#define console_input_init(x)			\
	do {/* nothing */			\
	} while ((0))
#define uart_register_input(x)			\
	do {/* nothing */			\
	} while ((0))
#endif

/******************************************************************************
 *
 * uart_console_init - initialize one UART as the console/debug port
 *
 * RETURNS: N/A
 */

void uart_console_init(void)
{
	__stdout_hook_install(consoleOut);
	__printk_hook_install(consoleOut);
}
