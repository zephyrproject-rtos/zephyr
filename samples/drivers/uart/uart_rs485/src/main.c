/*
 * @brief RS485 API UART sample
 *
 * Copyright (c) 2024 Grigorovich Sergey
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_interrupt_example, LOG_LEVEL_INF);

/* 1000 ms = 1 second */
#define SLEEP_TIME_MS 150

/* UART device */
const struct device *uart1;

/* Variables for UART and interrupt handling */
static struct k_sem tx_done_sem;
static const char *tx_data = "Hello, RS485!\r\n";
static volatile bool tx_busy;

/* UART callback function */
void uart_cb(const struct device *dev, void *user_data)
{
	static size_t tx_idx;
	size_t len = strlen(tx_data);

	/* If there is space in FIFO and data remains to be sent */
	if (uart_irq_tx_ready(dev) && tx_idx < len) {
		int bytes_written =
			uart_fifo_fill(dev, (const uint8_t *)&tx_data[tx_idx], len - tx_idx);

		tx_idx += bytes_written;
		LOG_DBG("TX Ready: Sent %d bytes, total sent: %d/%d", bytes_written, tx_idx, len);
	}

	/* Check if transmission is completely finished */
	if (uart_irq_tx_complete(dev) && tx_idx >= len) {
		uart_irq_tx_disable(dev);
		tx_idx = 0;
		tx_busy = false;
		LOG_DBG("TX Complete");
		k_sem_give(&tx_done_sem);
	}

	/* Handle received data (if necessary) */
	if (uart_irq_rx_ready(dev)) {
		uint8_t rx_data;

		while (uart_fifo_read(dev, &rx_data, 1)) {
			LOG_DBG("Received byte: 0x%02X", rx_data);
		}
	}
}

int main(void)
{
	uart1 = DEVICE_DT_GET(DT_NODELABEL(usart1));

	if (!device_is_ready(uart1)) {
		LOG_ERR("UART device is not ready");
		return 0;
	}

	/* Initialize semaphore */
	k_sem_init(&tx_done_sem, 0, 1);

	/* Set up UART interrupts */
	uart_irq_callback_set(uart1, uart_cb);
	uart_irq_rx_enable(uart1);
	uart_irq_tx_disable(uart1);

	LOG_INF("UART interrupt-driven example started.");

	while (1) {
		/* Data transmission */
		if (!tx_busy) {
			tx_busy = true;
			uart_irq_tx_enable(uart1);
			/* Initiate transmission of the first byte */
			uart_cb(uart1, NULL);
			/* Wait for transmission to complete */
			k_sem_take(&tx_done_sem, K_FOREVER);
		}

		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}
