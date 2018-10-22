/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <console.h>

static struct tty_serial console_serial;

static u8_t console_rxbuf[CONFIG_CONSOLE_GETCHAR_BUFSIZE];
static u8_t console_txbuf[CONFIG_CONSOLE_PUTCHAR_BUFSIZE];

int console_putchar(char c)
{
	return tty_putchar(&console_serial, (u8_t)c);
}

u8_t console_getchar(void)
{
	/* Console works in blocking mode, so we don't expect an error here */
	return (u8_t)tty_getchar(&console_serial);
}

void console_init(void)
{
	struct device *uart_dev;

	uart_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	tty_init(&console_serial, uart_dev,
		 console_rxbuf, sizeof(console_rxbuf),
		 console_txbuf, sizeof(console_txbuf));
}
