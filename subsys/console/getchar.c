/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sys/ring_buffer.h>
#include <console/console.h>
#include <console/tty.h>
#include <drivers/uart.h>

static const struct device *uart_dev;

#if defined(CONFIG_CONSOLE_RX_INTERRUPT_DRIVEN)

static struct tty_serial console_serial;
static uint8_t console_txbuf[CONFIG_CONSOLE_PUTCHAR_BUFSIZE];
static uint8_t console_rxbuf[CONFIG_CONSOLE_GETCHAR_BUFSIZE];

#else

#define POLL_PERIOD K_MSEC(CONFIG_CONSOLE_POLLING_RX_POLL_PERIOD)

void poll_timer_handler(struct k_timer *dummy);

RING_BUF_DECLARE(console_rx_ringbuf, CONFIG_CONSOLE_GETCHAR_BUFSIZE);

K_SEM_DEFINE(rx_sem, 0, CONFIG_CONSOLE_GETCHAR_BUFSIZE);

K_TIMER_DEFINE(poll_timer, poll_timer_handler, NULL);


void poll_timer_handler(struct k_timer *dummy)
{
	int ret = 0;
	uint8_t c;

	while (true) {
		if (ring_buf_space_get(&console_rx_ringbuf) == 0) {
			return;
		}

		ret = uart_poll_in(uart_dev, &c);
		if (ret != 0) {
			return;
		}

		ring_buf_put(&console_rx_ringbuf, &c, 1);

		k_sem_give(&rx_sem);
	}
}

#endif


ssize_t console_write(void *dummy, const void *buf, size_t size)
{
	ARG_UNUSED(dummy);

#if defined(CONFIG_CONSOLE_RX_INTERRUPT_DRIVEN)
	return tty_write(&console_serial, buf, size);
#else
	for (int i = 0; i < size; i++) {
		uint8_t c = ((uint8_t *)buf)[i];

		uart_poll_out(uart_dev, c);
	}
	return size;
#endif
}

ssize_t console_read(void *dummy, void *buf, size_t size)
{
	ARG_UNUSED(dummy);
#if defined(CONFIG_CONSOLE_RX_INTERRUPT_DRIVEN)
	return tty_read(&console_serial, buf, size);
#else
	return 0;
#endif
}

int console_putchar(char c)
{
#if defined(CONFIG_CONSOLE_RX_INTERRUPT_DRIVEN)
	return tty_write(&console_serial, &c, 1);
#else
	uart_poll_out(uart_dev, c);
	return 0;
#endif
}

int console_getchar(void)
{
	uint8_t c;
#if defined(CONFIG_CONSOLE_RX_INTERRUPT_DRIVEN)
	int res;

	res = tty_read(&console_serial, &c, 1);
	if (res < 0) {
		return res;
	}
#else
	int res = k_sem_take(&rx_sem, K_FOREVER);

	if (res < 0) {
		return res;
	}

	ring_buf_get(&console_rx_ringbuf, &c, sizeof(c));
#endif

	return c;
}

int console_init(void)
{
	uart_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);

#if defined(CONFIG_CONSOLE_RX_INTERRUPT_DRIVEN)
	int ret;

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

#else
	k_timer_start(&poll_timer, POLL_PERIOD, POLL_PERIOD);
#endif

	return 0;
}
