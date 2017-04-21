/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <uart.h>
#include <misc/printk.h>
#include <drivers/console/console.h>
#include <drivers/console/uart_console.h>

#if CONFIG_CONSOLE_GETCHAR_BUFSIZE & (CONFIG_CONSOLE_GETCHAR_BUFSIZE - 1) != 0
#error CONFIG_CONSOLE_GETCHAR_BUFSIZE must be power of 2
#endif

static K_SEM_DEFINE(uart_sem, 0, UINT_MAX);
static u8_t uart_ringbuf[CONFIG_CONSOLE_GETCHAR_BUFSIZE];
static u8_t i_get, i_put;

static int console_irq_input_hook(u8_t c)
{
	int i_next = (i_put + 1) & (CONFIG_CONSOLE_GETCHAR_BUFSIZE - 1);

	if (i_next == i_get) {
		printk("Console buffer overflow - char dropped\n");
		return 1;
	}

	uart_ringbuf[i_put] = c;
	i_put = i_next;
	k_sem_give(&uart_sem);

	return 1;
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

void console_getchar_init(void)
{
	uart_console_in_debug_hook_install(console_irq_input_hook);
	/* All NULLs because we're interested only in the callback above. */
	uart_register_input(NULL, NULL, NULL);
}
