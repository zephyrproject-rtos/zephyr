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

struct tty_serial {
	struct device *uart_dev;

	struct k_sem rx_sem;
	u8_t *rx_ringbuf;
	u32_t rx_ringbuf_sz;
	u16_t rx_get, rx_put;

	u8_t *tx_ringbuf;
	u32_t tx_ringbuf_sz;
	u16_t tx_get, tx_put;
};

static u8_t console_rxbuf[CONFIG_CONSOLE_GETCHAR_BUFSIZE];
static u8_t console_txbuf[CONFIG_CONSOLE_PUTCHAR_BUFSIZE];

static int tty_irq_input_hook(struct tty_serial *tty, u8_t c);

static struct tty_serial console_serial;

static void tty_uart_isr(void *user_data)
{
	struct tty_serial *tty = user_data;
	struct device *dev = tty->uart_dev;

	uart_irq_update(dev);

	if (uart_irq_rx_ready(dev)) {
		u8_t c;

		while (1) {
			if (uart_fifo_read(dev, &c, 1) == 0) {
				break;
			}
			tty_irq_input_hook(tty, c);
		}
	}

	if (uart_irq_tx_ready(dev)) {
		if (tty->tx_get == tty->tx_put) {
			/* Output buffer empty, don't bother
			 * us with tx interrupts
			 */
			uart_irq_tx_disable(dev);
		} else {
			uart_fifo_fill(dev, &tty->tx_ringbuf[tty->tx_get++], 1);
			if (tty->tx_get >= tty->tx_ringbuf_sz) {
				tty->tx_get = 0;
			}
		}
	}
}

static int tty_irq_input_hook(struct tty_serial *tty, u8_t c)
{
	int rx_next = tty->rx_put + 1;
	if (rx_next >= tty->rx_ringbuf_sz) {
		rx_next = 0;
	}

	if (rx_next == tty->rx_get) {
		/* Try to give a clue to user that some input was lost */
		console_putchar('~');
		console_putchar('\n');
		return 1;
	}

	tty->rx_ringbuf[tty->rx_put] = c;
	tty->rx_put = rx_next;
	k_sem_give(&tty->rx_sem);

	return 1;
}

int tty_putchar(struct tty_serial *tty, char c)
{
	unsigned int key;
	int tx_next;

	key = irq_lock();
	tx_next = tty->tx_put + 1;
	if (tx_next >= tty->tx_ringbuf_sz) {
		tx_next = 0;
	}
	if (tx_next == tty->tx_get) {
		irq_unlock(key);
		return -1;
	}

	tty->tx_ringbuf[tty->tx_put] = (u8_t)c;
	tty->tx_put = tx_next;

	irq_unlock(key);
	uart_irq_tx_enable(tty->uart_dev);
	return 0;
}

u8_t tty_getchar(struct tty_serial *tty)
{
	unsigned int key;
	u8_t c;

	k_sem_take(&tty->rx_sem, K_FOREVER);

	key = irq_lock();
	c = tty->rx_ringbuf[tty->rx_get++];
	if (tty->rx_get >= tty->rx_ringbuf_sz) {
		tty->rx_get = 0;
	}
	irq_unlock(key);

	return c;
}

void tty_init(struct tty_serial *tty, struct device *uart_dev,
	      u8_t *rxbuf, u16_t rxbuf_sz,
	      u8_t *txbuf, u16_t txbuf_sz)
{
	tty->uart_dev = uart_dev;
	tty->rx_ringbuf = rxbuf;
	tty->rx_ringbuf_sz = rxbuf_sz;
	tty->tx_ringbuf = txbuf;
	tty->tx_ringbuf_sz = txbuf_sz;
	tty->rx_get = tty->rx_put = tty->tx_get = tty->tx_put = 0;
	k_sem_init(&tty->rx_sem, 0, UINT_MAX);

	uart_irq_callback_user_data_set(uart_dev, tty_uart_isr, tty);
	uart_irq_rx_enable(uart_dev);
}


int console_putchar(char c)
{
	return tty_putchar(&console_serial, c);
}

u8_t console_getchar(void)
{
	return tty_getchar(&console_serial);
}

void console_init(void)
{
	struct device *uart_dev;

	uart_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	tty_init(&console_serial, uart_dev,
		 console_rxbuf, sizeof(console_rxbuf),
		 console_txbuf, sizeof(console_txbuf));
}
