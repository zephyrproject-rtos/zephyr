/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/console/console.h>
#include <zephyr/console/tty.h>
#include <zephyr/drivers/uart.h>

static struct tty_serial console_serial;

static uint8_t console_rxbuf[CONFIG_CONSOLE_GETCHAR_BUFSIZE];
static uint8_t console_txbuf[CONFIG_CONSOLE_PUTCHAR_BUFSIZE];

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
	uint8_t c;
	int res;

	res = tty_read(&console_serial, &c, 1);
	if (res < 0) {
		return res;
	}

	return c;
}

int console_init(void)
{
	const struct device *uart_dev;
	int ret;

	uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	if (!device_is_ready(uart_dev)) {
		return -ENODEV;
	}

	ret = tty_init(&console_serial, uart_dev);

	if (ret) {
		return ret;
	}

	/* Checks device driver supports for interrupt driven data transfers. */
	if (CONFIG_CONSOLE_GETCHAR_BUFSIZE + CONFIG_CONSOLE_PUTCHAR_BUFSIZE) {
		const struct uart_driver_api *api =
			(const struct uart_driver_api *)uart_dev->api;
		if (!api->irq_callback_set) {
			return -ENOTSUP;
		}
	}

	tty_set_tx_buf(&console_serial, console_txbuf, sizeof(console_txbuf));
	tty_set_rx_buf(&console_serial, console_rxbuf, sizeof(console_rxbuf));

	return 0;
}
