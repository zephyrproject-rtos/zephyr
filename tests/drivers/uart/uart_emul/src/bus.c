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
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	struct k_sem tx_done_sem;
	struct k_sem rx_done_sem;
	size_t tx_remaining;
	size_t rx_remaining;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_ASYNC_API
	struct k_event async_events;
#endif /* CONFIG_UART_ASYNC_API */
};

static void *uart_emul_setup(void)
{
	static struct uart_emul_fixture fixture = {.dev = DEVICE_DT_GET(EMUL_UART_NODE)};

	for (size_t i = 0; i < SAMPLE_DATA_SIZE; i++) {
		fixture.sample_data[i] = i;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	k_sem_init(&fixture.tx_done_sem, 0, 1);
	k_sem_init(&fixture.rx_done_sem, 0, 1);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
	k_event_init(&fixture.async_events);
#endif /* CONFIG_UART_ASYNC_API */

	zassert_not_null(fixture.dev);
	return &fixture;
}

static void uart_emul_before(void *f)
{
	struct uart_emul_fixture *fixture = f;

	uart_emul_flush_rx_data(fixture->dev);
	uart_emul_flush_tx_data(fixture->dev);

	uart_err_check(fixture->dev);

	memset(fixture->tx_content, 0, sizeof(fixture->tx_content));
	memset(fixture->rx_content, 0, sizeof(fixture->rx_content));

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_tx_disable(fixture->dev);
	uart_irq_rx_disable(fixture->dev);

	k_sem_reset(&fixture->tx_done_sem);
	k_sem_reset(&fixture->rx_done_sem);

	fixture->tx_remaining = SAMPLE_DATA_SIZE;
	fixture->rx_remaining = SAMPLE_DATA_SIZE;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
	uart_tx_abort(fixture->dev);
	uart_rx_disable(fixture->dev);

	k_event_set(&fixture->async_events, 0);
#endif /* CONFIG_UART_ASYNC_API */
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

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
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

	if (fixture->rx_remaining) {
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
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
static void uart_emul_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	struct uart_emul_fixture *fixture = user_data;

	zassert_not_null(evt);
	k_event_post(&fixture->async_events, ((uint32_t)1 << evt->type));

	switch (evt->type) {
	case UART_TX_DONE:
		zassert_equal(evt->data.tx.len, sizeof(fixture->sample_data));
		zassert_equal(evt->data.tx.buf, fixture->sample_data);
		break;
	case UART_RX_RDY:
		zassert_equal(evt->data.rx.len, sizeof(fixture->sample_data));
		zassert_mem_equal(&evt->data.rx.buf[evt->data.rx.offset], fixture->sample_data,
				  sizeof(fixture->sample_data));
		break;
	case UART_RX_BUF_RELEASED:
		zassert_equal(evt->data.rx_buf.buf, fixture->rx_content);
		break;
	case UART_TX_ABORTED:
	case UART_RX_BUF_REQUEST:
	case UART_RX_DISABLED:
	case UART_RX_STOPPED:
		break;
	}
}

bool uart_emul_wait_for_event(struct uart_emul_fixture *fixture, enum uart_event_type event)
{
	return k_event_wait(&fixture->async_events, ((uint32_t)1 << event), false, K_SECONDS(1)) !=
	       0;
}

ZTEST_F(uart_emul, test_async_tx)
{
	size_t tx_len;

	uart_emul_set_release_buffer_on_timeout(fixture->dev, true);

	zassert_equal(uart_callback_set(fixture->dev, uart_emul_callback, fixture), 0);
	zassert_equal(uart_tx(fixture->dev, fixture->sample_data, sizeof(fixture->sample_data),
			      SYS_FOREVER_US),
		      0);

	/* Wait for all data to be received in full */
	zexpect_true(uart_emul_wait_for_event(fixture, UART_TX_DONE),
		     "UART_TX_DONE event expected");

	tx_len = uart_emul_get_tx_data(fixture->dev, fixture->tx_content, SAMPLE_DATA_SIZE);
	zassert_equal(tx_len, SAMPLE_DATA_SIZE, "TX buffer length does not match");
	zassert_mem_equal(fixture->tx_content, fixture->sample_data, SAMPLE_DATA_SIZE);

	/* No more data in TX buffer */
	tx_len = uart_emul_get_tx_data(fixture->dev, fixture->tx_content,
				       sizeof(fixture->tx_content));
	zassert_equal(tx_len, 0, "TX buffer should be empty");
}

ZTEST_F(uart_emul, test_async_rx)
{
	zassert_equal(uart_callback_set(fixture->dev, uart_emul_callback, fixture), 0);
	zassert_equal(uart_rx_enable(fixture->dev, fixture->rx_content, sizeof(fixture->rx_content),
				     SYS_FOREVER_US),
		      0);
	uart_emul_put_rx_data(fixture->dev, fixture->sample_data, SAMPLE_DATA_SIZE);
	zexpect_true(uart_emul_wait_for_event(fixture, UART_RX_BUF_REQUEST),
		     "UART_RX_BUF_REQUEST event expected");
	zexpect_true(uart_emul_wait_for_event(fixture, UART_RX_RDY), "UART_RX_RDY event expected");
	zassert_mem_equal(fixture->rx_content, fixture->sample_data, SAMPLE_DATA_SIZE);
	zexpect_true(uart_emul_wait_for_event(fixture, UART_RX_BUF_RELEASED),
		     "UART_RX_BUF_RELEASED event expected");
	zexpect_true(uart_emul_wait_for_event(fixture, UART_RX_DISABLED),
		     "UART_RX_DISABLED event expected");
}

static void uart_emul_callback_rx_timeout(const struct device *dev, struct uart_event *evt,
					  void *user_data)
{
	struct uart_emul_fixture *fixture = user_data;

	zassert_not_null(evt);
	k_event_post(&fixture->async_events, ((uint32_t)1 << evt->type));
}

ZTEST_F(uart_emul, test_async_rx_buffer_release)
{
	zassert_equal(uart_callback_set(fixture->dev, uart_emul_callback_rx_timeout, fixture), 0);

	uint8_t rx_buffer[16];
	uint8_t rx_data[5];

	memset(rx_data, 1, sizeof(rx_data));

	zassert_equal(
		uart_rx_enable(fixture->dev, rx_buffer, sizeof(rx_buffer), 100 * USEC_PER_MSEC), 0);

	uart_emul_set_release_buffer_on_timeout(fixture->dev, false);
	uart_emul_put_rx_data(fixture->dev, rx_data, sizeof(rx_data));
	zexpect_false(uart_emul_wait_for_event(fixture, UART_RX_BUF_RELEASED),
		      "UART_RX_BUF_RELEASED event not expected");
	zexpect_true(uart_emul_wait_for_event(fixture, UART_RX_RDY));

	k_event_set(&fixture->async_events, 0);

	uart_emul_set_release_buffer_on_timeout(fixture->dev, true);
	uart_emul_put_rx_data(fixture->dev, rx_data, sizeof(rx_data));
	zexpect_true(uart_emul_wait_for_event(fixture, UART_RX_BUF_RELEASED),
		     "UART_RX_BUF_RELEASED event expected");
	zexpect_true(uart_emul_wait_for_event(fixture, UART_RX_RDY));
	zexpect_true(uart_emul_wait_for_event(fixture, UART_RX_DISABLED),
		     "UART_RX_DISABLED event expected");
}
#endif /* CONFIG_UART_ASYNC_API */

ZTEST_SUITE(uart_emul, NULL, uart_emul_setup, uart_emul_before, NULL, NULL);
