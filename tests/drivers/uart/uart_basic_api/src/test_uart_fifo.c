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

#include <test_uart.h>

static volatile bool data_transmitted;
static volatile bool data_received;
static int char_sent;
static const char *fifo_data = "This is a FIFO test.\r\n";

#define DATA_SIZE	strlen(fifo_data)

static void uart_fifo_callback(struct device *dev)
{
	uint8_t recvData;

	/* Verify uart_irq_update() */
	uart_irq_update(dev);

	/* Verify uart_irq_tx_ready() */
	if (uart_irq_tx_ready(dev)) {
		data_transmitted = true;
		char_sent++;
	}

	/* Verify uart_irq_rx_ready() */
	if (uart_irq_rx_ready(dev)) {
		/* Verify uart_fifo_read() */
		uart_fifo_read(dev, &recvData, 1);
		TC_PRINT("%c", recvData);

		if (recvData == '\n') {
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

	/* Verify uart_fifo_fill() */
	for (int i = 0; i < DATA_SIZE; i++) {
		data_transmitted = false;
		while (!uart_fifo_fill(uart_dev, (uint8_t *) &fifo_data[i], 1))
			;
		while (data_transmitted == false)
			;
	}

	/* Verify uart_irq_tx_disable() */
	uart_irq_tx_disable(uart_dev);

	/* strlen() doesn't include \r\n*/
	if (char_sent - 1 == DATA_SIZE) {
		return TC_PASS;
	} else {
		return TC_FAIL;
	}

}

void test_uart_fifo_fill(void)
{
	assert_true(test_fifo_fill() == TC_PASS, NULL);
}

void test_uart_fifo_read(void)
{
	assert_true(test_fifo_read() == TC_PASS, NULL);
}
