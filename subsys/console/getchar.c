/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <console.h>
#include <tty.h>

static struct tty_serial console_serial;

static u8_t console_rxbuf[CONFIG_CONSOLE_GETCHAR_BUFSIZE];
static u8_t console_txbuf[CONFIG_CONSOLE_PUTCHAR_BUFSIZE];

ssize_t console_write(void *dummy, const void *buf, size_t size)
{
	ARG_UNUSED(dummy);

	return tty_write(&console_serial, buf, size);
}

ssize_t console_read(void *dummy, void *buf, size_t size)
{
	ARG_UNUSED(dummy);

	return tty_read(&console_serial, buf, size);
}

int console_putchar(char c)
{
	return tty_write(&console_serial, &c, 1);
}

int console_getchar(void)
{
	u8_t c;
	int res;

	res = tty_read(&console_serial, &c, 1);
	if (res < 0) {
		return res;
	}

	return c;
}

void console_init(void)
{
	struct device *uart_dev;

	uart_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	tty_init(&console_serial, uart_dev);
	tty_set_tx_buf(&console_serial, console_txbuf, sizeof(console_txbuf));
	tty_set_rx_buf(&console_serial, console_rxbuf, sizeof(console_rxbuf));
}
