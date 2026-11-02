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

ZTEST_F(uart_emul, test_rx_flags_latched)
{
	const uint8_t rx_data[] = {0x11, 0x00, 0x22};
	const uint8_t rx_flags[] = {0, (UART_BREAK | UART_ERROR_FRAMING), 0};
	int errors;
	int rc;

	uart_emul_put_rx_data_flagged(fixture->dev, rx_data, rx_flags, sizeof(rx_data));

	/* Flags of bytes read without uart_fifo_read_flagged() are latched */
	for (size_t i = 0; i < sizeof(rx_data); i++) {
		rc = uart_poll_in(fixture->dev, &fixture->rx_content[i]);
		zassert_equal(rc, 0, "RX buffer should contain data");
	}
	zassert_mem_equal(fixture->rx_content, rx_data, sizeof(rx_data));

	errors = uart_err_check(fixture->dev);
	zassert_equal(errors, (UART_BREAK | UART_ERROR_FRAMING), "UART errors do not match");

	errors = uart_err_check(fixture->dev);
	zassert_equal(errors, 0, "Should be no errors");
}

ZTEST_F(uart_emul, test_config_get)
{
	struct uart_config cfg;

	Z_TEST_SKIP_IFNDEF(CONFIG_UART_USE_RUNTIME_CONFIGURE);

	/* Poison the configuration so that every field has to be written */
	memset(&cfg, 0xff, sizeof(cfg));

	/* Initial configuration comes from the euart0 node in uart_emul.overlay */
	zassert_ok(uart_config_get(fixture->dev, &cfg));
	zassert_equal(cfg.baudrate, 115200);
	zassert_equal(cfg.parity, UART_CFG_PARITY_EVEN);
	zassert_equal(cfg.stop_bits, UART_CFG_STOP_BITS_2);
	zassert_equal(cfg.data_bits, UART_CFG_DATA_BITS_7);
	zassert_equal(cfg.flow_ctrl, UART_CFG_FLOW_CTRL_RTS_CTS);
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

	while (true) {
		uart_irq_update(dev);

		if (uart_irq_is_pending(dev) <= 0) {
			break;
		}

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

static size_t uart_emul_fifo_read_flagged_all(const struct device *dev, uint8_t *rx_data,
					      uint8_t *flags, size_t size)
{
	size_t total = 0;
	int ret;

	do {
		ret = uart_fifo_read_flagged(dev, &rx_data[total], &flags[total], size - total);
		zassert_true(ret >= 0, "uart_fifo_read_flagged failed: %d", ret);
		total += (size_t)ret;
	} while ((ret > 0) && (total < size));

	return total;
}

ZTEST_F(uart_emul, test_fifo_read_flagged)
{
	const uint8_t clean_data[] = {0x55};
	const uint8_t rx_data[] = {0x11, 0x00, 0x22};
	const uint8_t rx_flags[] = {0, (UART_BREAK | UART_ERROR_FRAMING), 0};
	const uint8_t expected_data[] = {0x55, 0x11, 0x00, 0x22};
	const uint8_t expected_flags[] = {0, 0, (UART_BREAK | UART_ERROR_FRAMING), 0};
	uint8_t flags[sizeof(expected_flags)];
	size_t len;
	int errors;

	memset(flags, 0xff, sizeof(flags));

	uart_emul_put_rx_data(fixture->dev, clean_data, sizeof(clean_data));
	uart_emul_put_rx_data_flagged(fixture->dev, rx_data, rx_flags, sizeof(rx_data));

	/* The latch buffer limits each read, so this also covers reads split into batches */
	len = uart_emul_fifo_read_flagged_all(fixture->dev, fixture->rx_content, flags,
					      sizeof(expected_data));
	zassert_equal(len, sizeof(expected_data), "RX length does not match");
	zassert_mem_equal(fixture->rx_content, expected_data, sizeof(expected_data));
	zassert_mem_equal(flags, expected_flags, sizeof(expected_flags));

	/* Errors returned in flags are not reported again */
	errors = uart_err_check(fixture->dev);
	zassert_equal(errors, 0, "Should be no errors");

	/* No more data in RX buffer */
	zassert_equal(uart_fifo_read_flagged(fixture->dev, fixture->rx_content, flags, 1), 0,
		      "RX buffer should be empty");
}

ZTEST_F(uart_emul, test_fifo_read_flagged_overrun)
{
	uint8_t rx_data[EMUL_UART_RX_FIFO_SIZE];
	uint8_t flags[EMUL_UART_RX_FIFO_SIZE];
	size_t len;
	int errors;

	/* The second put fills the last free byte and loses the rest */
	uart_emul_put_rx_data(fixture->dev, fixture->sample_data, SAMPLE_DATA_SIZE);
	uart_emul_put_rx_data(fixture->dev, fixture->sample_data, SAMPLE_DATA_SIZE);

	len = uart_emul_fifo_read_flagged_all(fixture->dev, rx_data, flags, sizeof(rx_data));
	zassert_equal(len, EMUL_UART_RX_FIFO_SIZE, "RX length does not match");
	zassert_mem_equal(rx_data, fixture->sample_data, SAMPLE_DATA_SIZE);
	zassert_equal(rx_data[EMUL_UART_RX_FIFO_SIZE - 1], fixture->sample_data[0]);

	/*
	 * The overrun is reported once, on the last byte of the first read
	 * after it, which the latch buffer limits to a single byte
	 */
	zassert_equal(flags[0], UART_ERROR_OVERRUN, "Overrun not reported on the first byte");
	for (size_t i = 1; i < len; i++) {
		zassert_equal(flags[i], 0, "Unexpected flags 0x%02x at %zu", flags[i], i);
	}

	errors = uart_err_check(fixture->dev);
	zassert_equal(errors, 0, "Should be no errors");
}

ZTEST_F(uart_emul, test_fifo_read_flagged_overrun_err_check_first)
{
	uint8_t rx_data[EMUL_UART_RX_FIFO_SIZE];
	uint8_t flags[EMUL_UART_RX_FIFO_SIZE];
	size_t len;
	int errors;

	uart_emul_put_rx_data(fixture->dev, fixture->sample_data, SAMPLE_DATA_SIZE);
	uart_emul_put_rx_data(fixture->dev, fixture->sample_data, SAMPLE_DATA_SIZE);

	/* An overrun consumed by uart_err_check() is not reported again in flags */
	errors = uart_err_check(fixture->dev);
	zassert_equal(errors, UART_ERROR_OVERRUN, "UART errors do not match");

	len = uart_emul_fifo_read_flagged_all(fixture->dev, rx_data, flags, sizeof(rx_data));
	zassert_equal(len, EMUL_UART_RX_FIFO_SIZE, "RX length does not match");
	for (size_t i = 0; i < len; i++) {
		zassert_equal(flags[i], 0, "Unexpected flags 0x%02x at %zu", flags[i], i);
	}
}

/* Check that byte arrives clean and the overrun is only reported by uart_err_check() */
static void uart_emul_assert_overrun_not_flagged(const struct device *dev, uint8_t byte)
{
	uint8_t data;
	uint8_t flags;
	int errors;

	uart_emul_put_rx_data(dev, &byte, 1);
	zassert_equal(uart_fifo_read_flagged(dev, &data, &flags, 1), 1, "No data received");
	zassert_equal(data, byte, "RX data does not match");
	zassert_equal(flags, 0, "Unexpected flags 0x%02x", flags);

	errors = uart_err_check(dev);
	zassert_equal(errors, UART_ERROR_OVERRUN, "UART errors do not match");
}

ZTEST_F(uart_emul, test_fifo_read_flagged_busy)
{
	uint8_t data;
	uint8_t flags;

	/* Reads that return no data leave reads with flags allowed */
	zassert_equal(uart_fifo_read(fixture->dev, &data, 1), 0, "RX buffer should be empty");
	zassert_equal(uart_poll_in(fixture->dev, &data), -1, "RX buffer should be empty");
	zassert_equal(uart_fifo_read_flagged(fixture->dev, &data, &flags, 1), 0,
		      "Read with flags refused after reads of nothing");

	/*
	 * Once a byte is read without flags, reads with flags are refused until
	 * uart_err_check(), also with nothing left to read
	 */
	uart_emul_put_rx_data(fixture->dev, fixture->sample_data, 1);
	zassert_equal(uart_fifo_read(fixture->dev, &data, 1), 1, "No data received");
	zassert_equal(uart_fifo_read_flagged(fixture->dev, &data, &flags, 1), -EBUSY,
		      "Read with flags not refused with the buffer empty");
	zassert_equal(uart_fifo_read_flagged(fixture->dev, &data, &flags, 0), 0,
		      "Zero size read should return 0");
	zassert_equal(uart_fifo_read_flagged(fixture->dev, &data, &flags, 1), -EBUSY,
		      "Refusal lifted before uart_err_check()");
	zassert_equal(uart_err_check(fixture->dev), 0, "Should be no errors");
	zassert_equal(uart_fifo_read_flagged(fixture->dev, &data, &flags, 1), 0,
		      "Read with flags refused after uart_err_check()");
}

ZTEST_F(uart_emul, test_fifo_read_flagged_after_unflagged_read)
{
	uint8_t rx_data[EMUL_UART_RX_FIFO_SIZE - 2];
	uint8_t flags[EMUL_UART_RX_FIFO_SIZE - 2];
	uint8_t data;
	size_t len;
	int errors;

	/* The second put fills the last free byte and loses the rest */
	uart_emul_put_rx_data(fixture->dev, fixture->sample_data, SAMPLE_DATA_SIZE);
	uart_emul_put_rx_data(fixture->dev, fixture->sample_data, SAMPLE_DATA_SIZE);

	/* Once data is read without flags, uart_err_check() must take its errors first */
	zassert_equal(uart_fifo_read(fixture->dev, &data, 1), 1, "No data received");
	zassert_equal(uart_fifo_read_flagged(fixture->dev, rx_data, flags, 1), -EBUSY,
		      "Read with flags not refused after uart_fifo_read()");
	errors = uart_err_check(fixture->dev);
	zassert_equal(errors, UART_ERROR_OVERRUN, "UART errors do not match");

	zassert_equal(uart_poll_in(fixture->dev, &data), 0, "No data received");
	zassert_equal(uart_fifo_read_flagged(fixture->dev, rx_data, flags, 1), -EBUSY,
		      "Read with flags not refused after uart_poll_in()");
	errors = uart_err_check(fixture->dev);
	zassert_equal(errors, 0, "Should be no errors");

	/* The refused reads consumed nothing, and the overrun is not reported again */
	len = uart_emul_fifo_read_flagged_all(fixture->dev, rx_data, flags, sizeof(rx_data));
	zassert_equal(len, sizeof(rx_data), "RX length does not match");
	zassert_mem_equal(rx_data, &fixture->sample_data[2], SAMPLE_DATA_SIZE - 2);
	zassert_equal(rx_data[sizeof(rx_data) - 1], fixture->sample_data[0]);
	for (size_t i = 0; i < len; i++) {
		zassert_equal(flags[i], 0, "Unexpected flags 0x%02x at %zu", flags[i], i);
	}
}

ZTEST_F(uart_emul, test_fifo_read_flagged_overrun_flush)
{
	uart_emul_put_rx_data(fixture->dev, fixture->sample_data, SAMPLE_DATA_SIZE);
	uart_emul_put_rx_data(fixture->dev, fixture->sample_data, SAMPLE_DATA_SIZE);

	/* Flushing the data before the loss leaves the overrun to uart_err_check() */
	uart_emul_flush_rx_data(fixture->dev);

	uart_emul_assert_overrun_not_flagged(fixture->dev, 'Z');
}

ZTEST_F(uart_emul, test_fifo_read_flagged_set_errors)
{
	/* An overrun set with uart_emul_set_errors() is not reported against a byte */
	uart_emul_set_errors(fixture->dev, UART_ERROR_OVERRUN);

	uart_emul_assert_overrun_not_flagged(fixture->dev, 'Z');
}

ZTEST_F(uart_emul, test_fifo_read_flagged_size_zero)
{
	const uint8_t rx_data[] = {0x11, 0x00};
	const uint8_t rx_flags[] = {0, UART_BREAK};
	uint8_t flags[sizeof(rx_flags)];
	size_t len;

	uart_emul_put_rx_data_flagged(fixture->dev, rx_data, rx_flags, sizeof(rx_data));

	/* A size of 0 consumes no received data */
	zassert_equal(uart_fifo_read_flagged(fixture->dev, fixture->rx_content, flags, 0), 0,
		      "Zero size read should return 0");

	/* Neither does a negative size, which must not be taken as a large one */
	zassert_equal(uart_fifo_read_flagged(fixture->dev, fixture->rx_content, flags, -1), 0,
		      "Negative size read should return 0");

	len = uart_emul_fifo_read_flagged_all(fixture->dev, fixture->rx_content, flags,
					      sizeof(rx_data));
	zassert_equal(len, sizeof(rx_data), "RX length does not match");
	zassert_mem_equal(fixture->rx_content, rx_data, sizeof(rx_data));
	zassert_mem_equal(flags, rx_flags, sizeof(rx_flags));
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
