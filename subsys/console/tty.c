/*
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <zephyr/console/tty.h>
#include <zephyr/sys/clock.h>

enum tty_signal {
	TTY_SIGNAL_RXRDY = BIT(0),
	TTY_SIGNAL_TXDONE = BIT(1),
};

static void uart_tx_handle(const struct device *dev, struct tty_serial *tty);
static void uart_rx_handle(const struct device *dev, struct tty_serial *tty);

static void tty_uart_isr(const struct device *dev, void *user_data)
{
	struct tty_serial *tty = user_data;

	uart_irq_update(dev);

	if (uart_irq_rx_ready(dev)) {
		uart_rx_handle(dev, tty);
	}

	if (uart_irq_tx_ready(dev)) {
		uart_tx_handle(dev, tty);
	}
}

static void uart_rx_handle(const struct device *dev, struct tty_serial *tty)
{
	uint8_t *data;
	uint32_t len;
	uint32_t rd_len;
	bool new_data = false;
	int err;

	do {
		len = ring_buf_put_claim(&tty->rx_buf, &data, ring_buf_capacity_get(&tty->rx_buf));
		if (len > 0) {
			rd_len = uart_fifo_read(dev, data, len);

			if (rd_len > 0) {
				new_data = true;
			}

			err = ring_buf_put_finish(&tty->rx_buf, rd_len);
			__ASSERT_NO_MSG(err == 0);
			ARG_UNUSED(err);
		} else {
			uint8_t dummy;
			const char dummy_char = '~';

			/* Try to give a clue to user that some input was lost */
			tty_write(tty, &dummy_char, sizeof(dummy_char));

			/* No space in the ring buffer - consume byte. */
			rd_len = uart_fifo_read(dev, &dummy, 1);
		}
	} while (rd_len && (rd_len == len));

	if (new_data) {
		k_event_post(&tty->signal_event, TTY_SIGNAL_RXRDY);
	}
}

static void uart_tx_handle(const struct device *dev, struct tty_serial *tty)
{
	uint32_t len;
	uint8_t *data;
	int err;

	len = ring_buf_get_claim(&tty->tx_buf, &data, ring_buf_capacity_get(&tty->tx_buf));
	if (len > 0) {
		len = uart_fifo_fill(dev, data, len);
		err = ring_buf_get_finish(&tty->tx_buf, len);
		__ASSERT_NO_MSG(err == 0);
		ARG_UNUSED(err);
	} else {
		uart_irq_tx_disable(dev);
		atomic_clear(&tty->tx_busy);
	}

	k_event_post(&tty->signal_event, TTY_SIGNAL_TXDONE);
}

ssize_t tty_write(struct tty_serial *tty, const void *buf, size_t size)
{
	const uint8_t *p = buf;
	ssize_t out_size = 0;
	uint32_t write_size;
	int res = 0;

	if (ring_buf_capacity_get(&tty->tx_buf) == 0U) {
		/* Unbuffered operation, implicitly blocking. */
		out_size = size;

		while (size--) {
			uart_poll_out(tty->uart_dev, *p++);
		}

		return out_size;
	}

	while (size > 0) {
		write_size = ring_buf_put(&tty->tx_buf, p, size);

		if (atomic_set(&tty->tx_busy, 1) == 0) {
			uart_irq_tx_enable(tty->uart_dev);
		}

		if (write_size == 0) {
			/* Output buffer full, wait for space. */
			res = k_event_wait_safe(&tty->signal_event, TTY_SIGNAL_TXDONE, false,
						k_is_in_isr() ? K_NO_WAIT : tty->tx_timeout);
			if (res == 0) {
				break;
			}
		} else {
			out_size += write_size;
			p += write_size;
			size -= write_size;
		}
	}

	if (out_size == 0) {
		errno = -EAGAIN;
		return -EAGAIN;
	}

	return out_size;
}

static ssize_t tty_read_unbuf(struct tty_serial *tty, void *buf, size_t size)
{
	uint8_t *p = buf;
	size_t out_size = 0;
	int res = 0;
	k_timepoint_t timeout = sys_timepoint_calc(tty->rx_timeout);

	while (size) {
		uint8_t c;
		res = uart_poll_in(tty->uart_dev, &c);
		if (res <= -2) {
			/* Error occurred, best we can do is to return
			 * accumulated data w/o error, or return error
			 * directly if none.
			 */
			if (out_size == 0) {
				errno = res;
				return -1;
			}
			break;
		}

		if (res == 0) {
			*p++ = c;
			out_size++;
			size--;
		}

		if (size == 0 || sys_timepoint_expired(timeout)) {
			break;
		}

		/* Avoid 100% busy-polling, and yet try to process bursts
		 * of data without extra delays.
		 */
		if (res == -1) {
			k_sleep(K_MSEC(1));
		}
	}

	return out_size;
}

ssize_t tty_read(struct tty_serial *tty, void *buf, size_t size)
{
	uint8_t *p = buf;
	size_t out_size = 0;
	uint32_t read_size;
	int res = 0;

	if (ring_buf_capacity_get(&tty->rx_buf) == 0U) {
		return tty_read_unbuf(tty, buf, size);
	}

	while (size > 0) {
		read_size = ring_buf_get(&tty->rx_buf, p, size);

		if (read_size == 0) {
			/* Output buffer full, wait for space. */
			res = k_event_wait_safe(&tty->signal_event, TTY_SIGNAL_RXRDY, false,
						k_is_in_isr() ? K_NO_WAIT : tty->rx_timeout);
			if (res == 0) {
				break;
			}
		} else {
			out_size += read_size;
			p += read_size;
			size -= read_size;
		}
	}

	if (out_size == 0) {
		errno = -EAGAIN;
		return -EAGAIN;
	}

	return out_size;
}

int tty_init(struct tty_serial *tty, const struct device *uart_dev)
{
	if (!uart_dev) {
		return -ENODEV;
	}

	tty->uart_dev = uart_dev;

	/* We start in unbuffer mode. */
	ring_buf_init(&tty->rx_buf, 0, NULL);
	ring_buf_init(&tty->tx_buf, 0, NULL);

	tty->rx_timeout = K_FOREVER;
	tty->tx_timeout = K_FOREVER;

	k_event_init(&tty->signal_event);
	tty->tx_busy = 0;

	uart_irq_callback_user_data_set(uart_dev, tty_uart_isr, tty);

	return 0;
}

int tty_set_rx_buf(struct tty_serial *tty, void *buf, size_t size)
{
	uart_irq_rx_disable(tty->uart_dev);

	ring_buf_init(&tty->rx_buf, size, buf);

	if (size > 0) {
		uart_irq_rx_enable(tty->uart_dev);
	}

	return 0;
}

int tty_set_tx_buf(struct tty_serial *tty, void *buf, size_t size)
{
	uart_irq_tx_disable(tty->uart_dev);

	ring_buf_init(&tty->tx_buf, size, buf);

	/* New buffer is initially empty, no need to re-enable interrupts,
	 * it will be done when needed (on first output char).
	 */

	return 0;
}
