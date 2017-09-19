/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup t_uart_basic
 * @{
 * @defgroup t_uart_fifo test_uart_fifo
 * @brief TestPurpose: verify UART works well in fifo mode
 * @details
 * - Test Steps
 *   - FIFO Output:
 *      -# Set UART IRQ callback using uart_irq_callback_set().
 *      -# Enable UART TX IRQ using uart_irq_tx_enable().
 *      -# Output the prepared data using uart_fifo_fill().
 *      -# Disable UART TX IRQ using uart_irq_tx_disable().
 *      -# Compare the number of characters sent out with the
 *         original data size.
 *   - FIFO Input:
 *      -# Set UART IRQ callback using uart_irq_callback_set().
 *      -# Enable UART RX IRQ using uart_irq_rx_enable().
 *      -# Wait for data sent to UART console and trigger RX IRQ.
 *      -# Read data from UART console using uart_fifo_read().
 *      -# Disable UART TX IRQ using uart_irq_rx_disable().
 * - Expected Results
 *   -# When test UART FIFO output, the number of characters actually
 *      sent out will be equal to the original length of the characters.
 *   -# When test UART FIFO input, the app will wait for input from UART
 *      console and exit after receiving one character.
 * @}
 */

#include "test_uart.h"

static volatile bool data_transmitted;
static volatile bool data_received;
static int char_sent;
static const char fifo_data[] = "This is a FIFO test.\r\n";

#define DATA_SIZE	(sizeof(fifo_data) - 1)

static void uart_fifo_callback(struct device *dev)
{
	u8_t recvData;
	static int tx_data_idx;

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
		if (uart_fifo_fill(dev,
				   (u8_t *)&fifo_data[tx_data_idx++], 1) > 0) {
			data_transmitted = true;
			char_sent++;
		}

		if (tx_data_idx == DATA_SIZE) {
			/* If we transmitted everything, stop IRQ stream,
			 * otherwise main app might never run.
			 */
			uart_irq_tx_disable(dev);
		}
	}

	/* Verify uart_irq_rx_ready() */
	if (uart_irq_rx_ready(dev)) {
		/* Verify uart_fifo_read() */
		uart_fifo_read(dev, &recvData, 1);
		TC_PRINT("%c", recvData);

		if ((recvData == '\n') || (recvData == '\r')) {
			data_received = true;
		}
	}
}

static int test_fifo_read(void)
{
	struct device *uart_dev = device_get_binding(UART_DEVICE_NAME);

	/* Verify uart_irq_callback_set() */
	uart_irq_callback_set(uart_dev, uart_fifo_callback);

	/* Enable Tx/Rx interrupt before using fifo */
	/* Verify uart_irq_rx_enable() */
	uart_irq_rx_enable(uart_dev);

	TC_PRINT("Please send characters to serial console\n");

	data_received = false;
	while (data_received == false)
		;
	/* Verify uart_irq_rx_disable() */
	uart_irq_rx_disable(uart_dev);

	return TC_PASS;
}

static int test_fifo_fill(void)
{
	struct device *uart_dev = device_get_binding(UART_DEVICE_NAME);

	char_sent = 0;

	/* Verify uart_irq_callback_set() */
	uart_irq_callback_set(uart_dev, uart_fifo_callback);

	/* Enable Tx/Rx interrupt before using fifo */
	/* Verify uart_irq_tx_enable() */
	uart_irq_tx_enable(uart_dev);

	k_sleep(500);

	/* Verify uart_irq_tx_disable() */
	uart_irq_tx_disable(uart_dev);

	if (!data_transmitted) {
		return TC_FAIL;
	}

	if (char_sent == DATA_SIZE) {
		return TC_PASS;
	} else {
		return TC_FAIL;
	}

}

void test_uart_fifo_fill(void)
{
	zassert_true(test_fifo_fill() == TC_PASS, NULL);
}

void test_uart_fifo_read(void)
{
	zassert_true(test_fifo_read() == TC_PASS, NULL);
}
