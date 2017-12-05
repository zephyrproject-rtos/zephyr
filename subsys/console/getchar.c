/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <uart.h>
#include <misc/printk.h>
#include <console.h>
#include <drivers/console/console.h>
#include <drivers/console/uart_console.h>

#if (CONFIG_CONSOLE_GETCHAR_BUFSIZE & (CONFIG_CONSOLE_GETCHAR_BUFSIZE - 1)) != 0
#error CONFIG_CONSOLE_GETCHAR_BUFSIZE must be power of 2
#endif

#if (CONFIG_CONSOLE_PUTCHAR_BUFSIZE & (CONFIG_CONSOLE_PUTCHAR_BUFSIZE - 1)) != 0
#error CONFIG_CONSOLE_PUTCHAR_BUFSIZE must be power of 2
#endif

static K_SEM_DEFINE(uart_sem, 0, UINT_MAX);
static u8_t uart_ringbuf[CONFIG_CONSOLE_GETCHAR_BUFSIZE];
static u8_t i_get, i_put;

static K_SEM_DEFINE(tx_sem, 0, UINT_MAX);
static u8_t tx_ringbuf[CONFIG_CONSOLE_PUTCHAR_BUFSIZE];
static u8_t tx_get, tx_put;

static struct device *uart_dev;

static int console_irq_input_hook(u8_t c);

static void uart_isr(struct device *dev)
{
	uart_irq_update(dev);

	if (uart_irq_rx_ready(dev)) {
		u8_t c;

		while (1) {
			if (uart_fifo_read(dev, &c, 1) == 0) {
				break;
			}
			console_irq_input_hook(c);
		}
	}

	if (uart_irq_tx_ready(dev)) {
		if (tx_get == tx_put) {
			/* Output buffer empty, don't bother
			 * us with tx interrupts
			 */
			uart_irq_tx_disable(dev);
		} else {
			uart_fifo_fill(dev, &tx_ringbuf[tx_get++], 1);
			tx_get &= CONFIG_CONSOLE_PUTCHAR_BUFSIZE - 1;
		}
	}
}

static int console_irq_input_hook(u8_t c)
{
	int i_next = (i_put + 1) & (CONFIG_CONSOLE_GETCHAR_BUFSIZE - 1);

	if (i_next == i_get) {
		/* Try to give a clue to user that some input was lost */
		console_putchar('~');
		console_putchar('\n');
		return 1;
	}

	uart_ringbuf[i_put] = c;
	i_put = i_next;
	k_sem_give(&uart_sem);

	return 1;
}

int console_putchar(char c)
{
	unsigned int key;
	int tx_next;

	key = irq_lock();
	tx_next = (tx_put + 1) & (CONFIG_CONSOLE_PUTCHAR_BUFSIZE - 1);
	if (tx_next == tx_get) {
		irq_unlock(key);
		return -1;
	}

	tx_ringbuf[tx_put] = (u8_t)c;
	tx_put = tx_next;

	irq_unlock(key);
	uart_irq_tx_enable(uart_dev);
	return 0;
}

u8_t console_getchar(void)
{
	unsigned int key;
	u8_t c;

	k_sem_take(&uart_sem, K_FOREVER);
	key = irq_lock();
	c = uart_ringbuf[i_get++];
	i_get &= CONFIG_CONSOLE_GETCHAR_BUFSIZE - 1;
	irq_unlock(key);

	return c;
}

void console_init(void)
{
	uart_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	uart_irq_callback_set(uart_dev, uart_isr);
	uart_irq_rx_enable(uart_dev);
}
