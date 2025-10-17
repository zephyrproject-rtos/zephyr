/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2025 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_uart.h"

static volatile bool data_received;
static volatile bool data_transmit_done;
static volatile bool data_receive_done;
static int char_sent;
static int tx_size;
static const char fifo_data[] = "This is a FIFO test.";

#define DATA_SIZE (sizeof(fifo_data))

static void uart_fifo_callback(const struct device *dev, void *user_data)
{
	static int tx_data_idx;
	int ret;

	ARG_UNUSED(user_data);

	/* Verify uart_irq_update() */
	if (!uart_irq_update(dev)) {
		TC_PRINT("retval should always be 1\n");
		return;
	}

	/* Verify uart_irq_tx_ready() */
	/* Note that TX IRQ may be disabled, but uart_irq_tx_ready() may
	 * still return true when ISR is called for another UART interrupt,
	 * hence additional check for i < DATA_SIZE.
	 */
	if (uart_irq_tx_ready(dev) && tx_data_idx < DATA_SIZE) {
		/* We arrive here by "tx ready" interrupt, so should always
		 * be able to put at least one byte into a FIFO. If not,
		 * well, we'll fail test.
		 */
		tx_size++;
		ret = uart_fifo_fill(dev, (uint8_t *)&fifo_data[tx_data_idx],
				     MIN(tx_size, DATA_SIZE - char_sent));
		if (ret > 0) {
			char_sent += ret;
			tx_data_idx += ret;
		} else {
			TC_PRINT("Failed to fill %d to %s\n", ret, dev->name);
			return;
		}

		if (tx_data_idx == DATA_SIZE) {
			/* If we transmitted everything, stop IRQ stream,
			 * otherwise main app might never run.
			 */
			data_transmit_done = true;
		}
	}
}

static int test_fifo_tx_sizes(void)
{
	const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE);

	if (!device_is_ready(uart_dev)) {
		TC_PRINT("UART device not ready\n");
		return TC_FAIL;
	}

	char_sent = 0;

	/* Verify uart_irq_callback_set() */
	uart_irq_callback_set(uart_dev, uart_fifo_callback);

	/* Enable Tx/Rx interrupt before using fifo */
	/* Verify uart_irq_tx_enable() */
	uart_irq_tx_enable(uart_dev);

	k_sleep(K_MSEC(500));

	/* Verify uart_irq_tx_disable() */
	uart_irq_tx_disable(uart_dev);

	if (!data_transmit_done) {
		return TC_FAIL;
	}

	return TC_PASS;
}

ZTEST(uart_interrupt_api, test_uart_fifo_tx_sizes)
{
#ifndef CONFIG_UART_INTERRUPT_DRIVEN
	ztest_test_skip();
#endif
	zassert_true(test_fifo_tx_sizes() == TC_PASS);
}

ZTEST_SUITE(uart_interrupt_api, NULL, NULL, NULL, NULL, NULL);
