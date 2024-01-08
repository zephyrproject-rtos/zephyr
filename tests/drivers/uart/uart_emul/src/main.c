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

/*
 * Leave one byte left in tx to avoid filling it completely which will block the UART
 * tx ready IRQ event.
 */
#define SAMPLE_DATA_SIZE       MIN(EMUL_UART_RX_FIFO_SIZE, EMUL_UART_TX_FIFO_SIZE) - 1

struct uart_emul_fixture {
	const struct device *dev;
	uint8_t sample_data[SAMPLE_DATA_SIZE];
	uint8_t tx_content[SAMPLE_DATA_SIZE];
	uint8_t rx_content[SAMPLE_DATA_SIZE];
	struct k_sem tx_done_sem;
	struct k_sem rx_done_sem;
	size_t tx_remaining;
	size_t rx_remaining;
};

static void *uart_emul_setup(void)
{
	static struct uart_emul_fixture fixture = {.dev = DEVICE_DT_GET(EMUL_UART_NODE)};

	for (size_t i = 0; i < SAMPLE_DATA_SIZE; i++) {
		fixture.sample_data[i] = i;
	}

	k_sem_init(&fixture.tx_done_sem, 0, 1);
	k_sem_init(&fixture.rx_done_sem, 0, 1);

	zassert_not_null(fixture.dev);
	return &fixture;
}

static void uart_emul_before(void *f)
{
	struct uart_emul_fixture *fixture = f;

	uart_irq_tx_disable(fixture->dev);
	uart_irq_rx_disable(fixture->dev);

	uart_emul_flush_rx_data(fixture->dev);
	uart_emul_flush_tx_data(fixture->dev);

	uart_err_check(fixture->dev);

	k_sem_reset(&fixture->tx_done_sem);
	k_sem_reset(&fixture->rx_done_sem);

	memset(fixture->tx_content, 0, sizeof(fixture->tx_content));
	memset(fixture->rx_content, 0, sizeof(fixture->rx_content));

	fixture->tx_remaining = SAMPLE_DATA_SIZE;
	fixture->rx_remaining = SAMPLE_DATA_SIZE;
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
	int rc;

	uart_emul_put_rx_data(fixture->dev, fixture->sample_data, SAMPLE_DATA_SIZE);

	for (size_t i = 0; i < SAMPLE_DATA_SIZE; i++) {
		rc = uart_poll_in(fixture->dev, &fixture->rx_content[i]);
		zassert_equal(rc, 0, "RX buffer should contain data");
	}
	zassert_mem_equal(fixture->rx_content, fixture->sample_data, SAMPLE_DATA_SIZE);

	/* No more data in RX buffer */
	rc = uart_poll_in(fixture->dev, &fixture->rx_content[0]);
	zassert_equal(rc, -1, "RX buffer should be empty");
}

ZTEST_F(uart_emul, test_errors)
{
	int errors;

	uart_emul_set_errors(fixture->dev, (UART_ERROR_PARITY | UART_ERROR_FRAMING));
	errors = uart_err_check(fixture->dev);
	zassert_equal(errors, (UART_ERROR_PARITY | UART_ERROR_FRAMING), "UART errors do not match");

	/* uart_err_check should also clear existing errors */
	errors = uart_err_check(fixture->dev);
	zassert_equal(errors, 0, "Should be no errors");

	/* overflowing rx buffer should produce an overrun error */
	uart_emul_put_rx_data(fixture->dev, fixture->sample_data, SAMPLE_DATA_SIZE);
	errors = uart_err_check(fixture->dev);
	zassert_equal(errors, 0, "Should be no errors");
	uart_emul_put_rx_data(fixture->dev, fixture->sample_data, SAMPLE_DATA_SIZE);
	errors = uart_err_check(fixture->dev);
	zassert_equal(errors, UART_ERROR_OVERRUN, "UART errors do not match");
}

static void uart_emul_isr_handle_tx_ready(struct uart_emul_fixture *fixture)
{
	uint32_t sample_data_it;
	int ret;

	if (fixture->tx_remaining) {
		sample_data_it = sizeof(fixture->sample_data) - fixture->tx_remaining;
		ret = uart_fifo_fill(fixture->dev, &fixture->sample_data[sample_data_it],
				     fixture->tx_remaining);
		fixture->tx_remaining -= (size_t)ret;
	}

	if (fixture->tx_remaining == 0) {
		uart_irq_tx_disable(fixture->dev);
		k_sem_give(&fixture->tx_done_sem);
	}
}

static void uart_emul_isr_handle_rx_ready(struct uart_emul_fixture *fixture)
{
	uint32_t rx_content_it;
	int ret;

	if (fixture->tx_remaining) {
		rx_content_it = sizeof(fixture->rx_content) - fixture->rx_remaining;
		ret = uart_fifo_read(fixture->dev, &fixture->rx_content[rx_content_it],
				     fixture->rx_remaining);
		fixture->rx_remaining -= (size_t)ret;
	}

	if (fixture->rx_remaining == 0) {
		k_sem_give(&fixture->rx_done_sem);
	}
}

static void uart_emul_isr(const struct device *dev, void *user_data)
{
	struct uart_emul_fixture *fixture = user_data;

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_tx_ready(fixture->dev)) {
			uart_emul_isr_handle_tx_ready(fixture);
		}
		if (uart_irq_rx_ready(fixture->dev)) {
			uart_emul_isr_handle_rx_ready(fixture);
		}
	}
}

ZTEST_F(uart_emul, test_irq_tx)
{
	size_t tx_len;

	uart_irq_callback_user_data_set(fixture->dev, uart_emul_isr, fixture);
	/* enabling the tx irq will call the callback, if set */
	uart_irq_tx_enable(fixture->dev);
	/* Wait for all data to be received in full */
	zassert_equal(k_sem_take(&fixture->tx_done_sem, K_SECONDS(1)), 0,
		      "Timeout waiting for UART ISR");

	tx_len = uart_emul_get_tx_data(fixture->dev, fixture->tx_content, SAMPLE_DATA_SIZE);
	zassert_equal(tx_len, SAMPLE_DATA_SIZE, "TX buffer length does not match");
	zassert_mem_equal(fixture->tx_content, fixture->sample_data, SAMPLE_DATA_SIZE);

	/* No more data in TX buffer */
	tx_len = uart_emul_get_tx_data(fixture->dev, fixture->tx_content,
				       sizeof(fixture->tx_content));
	zassert_equal(tx_len, 0, "TX buffer should be empty");
}

ZTEST_F(uart_emul, test_irq_rx)
{
	int rc;

	uart_irq_callback_user_data_set(fixture->dev, uart_emul_isr, fixture);
	uart_irq_rx_enable(fixture->dev);

	/* putting rx data will call the irq callback, if enabled */
	uart_emul_put_rx_data(fixture->dev, fixture->sample_data, SAMPLE_DATA_SIZE);

	/* Wait for all data to be received in full */
	zassert_equal(k_sem_take(&fixture->rx_done_sem, K_SECONDS(1)), 0,
		      "Timeout waiting for UART ISR");

	zassert_mem_equal(fixture->rx_content, fixture->sample_data, SAMPLE_DATA_SIZE);

	/* No more data in RX buffer */
	rc = uart_poll_in(fixture->dev, &fixture->rx_content[0]);
	zassert_equal(rc, -1, "RX buffer should be empty");

	uart_irq_rx_disable(fixture->dev);
}

ZTEST_SUITE(uart_emul, NULL, uart_emul_setup, uart_emul_before, NULL, NULL);
