/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/serial/uart_emul.h>

LOG_MODULE_REGISTER(test, CONFIG_SAMPLE_MODULE_LOG_LEVEL);

#define EMUL_UART_NUM             DT_NUM_INST_STATUS_OKAY(zephyr_uart_emul)
#define EMUL_UART_NODE(i)         DT_NODELABEL(emul_uart##i)
#define EMUL_UART_DEV_INIT(i, _)  DEVICE_DT_GET(EMUL_UART_NODE(i))
#define EMUL_UART_TX_FIFO_SIZE(i) DT_PROP(EMUL_UART_NODE(i), tx_fifo_size)
#define SAMPLE_DATA_SIZE          EMUL_UART_TX_FIFO_SIZE(0)

#define TEST_DATA "0123456789ABCDEF"
/* Assert that size of the test string (without '\0') fits in the SAMPLE_DATA_SIZE */
BUILD_ASSERT((sizeof(TEST_DATA) - 1) < SAMPLE_DATA_SIZE);

struct log_backend_uart_fixture {
	const struct device *dev[EMUL_UART_NUM];
};

static void *uart_emul_setup(void)
{
	static struct log_backend_uart_fixture fixture = {
		.dev = {LISTIFY(EMUL_UART_NUM, EMUL_UART_DEV_INIT, (,))}};

	for (size_t i = 0; i < EMUL_UART_NUM; i++) {
		zassert_not_null(fixture.dev[i]);
	}

	return &fixture;
}

static void uart_emul_before(void *f)
{
	struct log_backend_uart_fixture *fixture = f;

	for (size_t i = 0; i < EMUL_UART_NUM; i++) {
		uart_irq_tx_disable(fixture->dev[i]);
		uart_irq_rx_disable(fixture->dev[i]);

		uart_emul_flush_rx_data(fixture->dev[i]);
		uart_emul_flush_tx_data(fixture->dev[i]);

		uart_err_check(fixture->dev[i]);
	}
}

ZTEST_F(log_backend_uart, test_log_backend_uart_multi_instance)
{
	/* Prevent stack overflow by making it static */
	static uint8_t tx_content[SAMPLE_DATA_SIZE];
	size_t tx_len;

	zassert_equal(log_backend_count_get(), EMUL_UART_NUM, "Unexpected number of instance(s)");

	LOG_RAW(TEST_DATA);

	for (size_t i = 0; i < EMUL_UART_NUM; i++) {
		memset(tx_content, 0, sizeof(tx_content));

		tx_len = uart_emul_get_tx_data(fixture->dev[i], tx_content, sizeof(tx_content));
		zassert_equal(tx_len, strlen(TEST_DATA),
			      "%d: TX buffer length does not match. Expected %d, got %d",
			      i, strlen(TEST_DATA), tx_len);
		zassert_mem_equal(tx_content, TEST_DATA, strlen(TEST_DATA));
	}
}

ZTEST_SUITE(log_backend_uart, NULL, uart_emul_setup, uart_emul_before, NULL, NULL);
