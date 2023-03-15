/*
 * Copyright (c) 2023 Fabian Blatz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/ztest.h>

#define EMUL_UART_NODE	       DT_NODELABEL(euart0)
#define EMUL_UART_RX_FIFO_SIZE DT_PROP(EMUL_UART_NODE, rx_fifo_size)
#define EMUL_UART_TX_FIFO_SIZE DT_PROP(EMUL_UART_NODE, tx_fifo_size)
#define SAMPLE_DATA_SIZE       MIN(EMUL_UART_RX_FIFO_SIZE, EMUL_UART_TX_FIFO_SIZE)

struct uart_emul_fixture {
	const struct device *dev;
	uint8_t sample_data[SAMPLE_DATA_SIZE];
};

static void *uart_emul_setup(void)
{
	static struct uart_emul_fixture fixture = {.dev = DEVICE_DT_GET(EMUL_UART_NODE)};

	for (size_t i = 0; i < SAMPLE_DATA_SIZE; i++) {
		fixture.sample_data[i] = i;
	}

	zassert_not_null(fixture.dev);
	return &fixture;
}

static void uart_emul_before(void *f)
{
	struct uart_emul_fixture *fixture = f;

	uart_emul_flush_rx_data(fixture->dev);
	uart_emul_flush_tx_data(fixture->dev);
}

ZTEST_F(uart_emul, test_polling_out)
{
	uint8_t tx_content[SAMPLE_DATA_SIZE] = {0};
	size_t tx_len;

	for (size_t i = 0; i < SAMPLE_DATA_SIZE; i++) {
		uart_poll_out(fixture->dev, fixture->sample_data[i]);
	}

	tx_len = uart_emul_get_tx_data(fixture->dev, tx_content, sizeof(tx_content));
	zassert_equal(tx_len, SAMPLE_DATA_SIZE, "TX buffer length does not match");
	zassert_mem_equal(tx_content, fixture->sample_data, SAMPLE_DATA_SIZE);

	/* No more data in TX buffer */
	tx_len = uart_emul_get_tx_data(fixture->dev, tx_content, sizeof(tx_content));
	zassert_equal(tx_len, 0, "TX buffer should be empty");
}

ZTEST_F(uart_emul, test_polling_in)
{
	uint8_t rx_content[SAMPLE_DATA_SIZE] = {0};
	int rc;

	uart_emul_put_rx_data(fixture->dev, fixture->sample_data, SAMPLE_DATA_SIZE);

	for (size_t i = 0; i < SAMPLE_DATA_SIZE; i++) {
		rc = uart_poll_in(fixture->dev, &rx_content[i]);
		zassert_equal(rc, 0, "RX buffer should contain data");
	}
	zassert_mem_equal(rx_content, fixture->sample_data, SAMPLE_DATA_SIZE);

	/* No more data in RX buffer */
	rc = uart_poll_in(fixture->dev, &rx_content[0]);
	zassert_equal(rc, -1, "RX buffer should be empty");
}

ZTEST_SUITE(uart_emul, NULL, uart_emul_setup, uart_emul_before, NULL, NULL);
