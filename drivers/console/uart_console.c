/* uart_console.c - UART-driven console */

/*
 * Copyright (c) 2011-2012, 2014-2015 Wind River Systems, Inc.
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

/*
 * DESCRIPTION
 *
 * Serial console driver.
 * Hooks into the printk and fputc (for printf) modules. Poll driven.
 */

#include <nanokernel.h>
#include <arch/cpu.h>

#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#include <device.h>
#include <init.h>

#include <board.h>
#include <uart.h>
#include <console/uart_console.h>
#include <toolchain.h>
#include <sections.h>

static struct device *uart_console_dev;

#if 0 /* NOTUSED */
/**
 *
 * @brief Get a character from UART
 *
 * @return the character or EOF if nothing present
 */

static int console_in(void)
{
	unsigned char c;

	if (uart_poll_in(uart_console_dev, &c) < 0)
		return EOF;
	else
		return (int)c;
}
#endif

#if defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE)
/**
 *
 * @brief Output one character to UART
 *
 * Outputs both line feed and carriage return in the case of a '\n'.
 *
 * @param c Character to output
 *
 * @return The character passed as input.
 */

static int console_out(int c)
{
	uart_poll_out(uart_console_dev, (unsigned char)c);
	if ('\n' == c) {
		uart_poll_out(uart_console_dev, (unsigned char)'\r');
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
static size_t pos;

static struct nano_fifo *avail_queue;
static struct nano_fifo *lines_queue;

static int read_uart(struct device *uart, uint8_t *buf, unsigned int size)
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

	while (uart_irq_update(uart_console_dev)
	       && uart_irq_is_pending(uart_console_dev)) {
		/* Character(s) have been received */
		if (uart_irq_rx_ready(uart_console_dev)) {
			static struct uart_console_input *cmd;
			uint8_t byte;
			int rx;

			rx = read_uart(uart_console_dev, &byte, 1);
			if (rx < 0) {
				return;
			}

			if (uart_irq_input_hook(uart_console_dev, byte) != 0) {
				/*
				 * The input hook indicates that no further processing
				 * should be done by this handler.
				 */
				return;
			}

			if (!cmd) {
				cmd = nano_isr_fifo_get(avail_queue);
				if (!cmd)
					return;
			}

			/* Echo back to console */
			uart_poll_out(uart_console_dev, byte);

			if (byte == '\r' || byte == '\n' ||
			    pos == sizeof(cmd->line) - 1) {
				cmd->line[pos] = '\0';
				uart_poll_out(uart_console_dev, '\n');
				pos = 0;

				nano_isr_fifo_put(lines_queue, cmd);
				cmd = NULL;
			} else {
				cmd->line[pos++] = byte;
			}

		}
	}
}

IRQ_CONNECT_STATIC(console, CONFIG_UART_CONSOLE_IRQ,
		   CONFIG_UART_CONSOLE_IRQ_PRI, uart_console_isr, 0,
		   UART_IRQ_FLAGS);

static void console_input_init(void)
{
	uint8_t c;

	uart_irq_rx_disable(uart_console_dev);
	uart_irq_tx_disable(uart_console_dev);
	IRQ_CONFIG(console, uart_irq_get(uart_console_dev), 0);
	irq_enable(uart_irq_get(uart_console_dev));

	/* Drain the fifo */
	while (uart_irq_rx_ready(uart_console_dev)) {
		uart_fifo_read(uart_console_dev, &c, 1);
	}

	uart_irq_rx_enable(uart_console_dev);
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

/**
 *
 * @brief Install printk/stdout hook for UART console output
 *
 * @return N/A
 */

void uart_console_hook_install(void)
{
	__stdout_hook_install(console_out);
	__printk_hook_install(console_out);
}

/**
 *
 * @brief Initialize one UART as the console/debug port
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static int uart_console_init(struct device *arg)
{
	ARG_UNUSED(arg);

	uart_console_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);

	uart_console_hook_install();

	return DEV_OK;
}
DECLARE_DEVICE_INIT_CONFIG(uart_console, "", uart_console_init, NULL);

/* UART consloe initializes after the UART device itself */
#if defined(CONFIG_EARLY_CONSOLE)
SYS_DEFINE_DEVICE(uart_console, NULL, PRIMARY, CONFIG_UART_CONSOLE_PRIORITY);
#else
SYS_DEFINE_DEVICE(uart_console, NULL, SECONDARY, CONFIG_UART_CONSOLE_PRIORITY);
#endif
