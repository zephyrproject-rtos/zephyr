/*
 * Copyright 2024 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT uart_dummy

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/uart_emul.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/drivers/emul_stub_device.h>
#include <zephyr/ztest.h>

#define UART_DUMMY_NODE        DT_NODELABEL(dummy)
#define EMUL_UART_NODE         DT_PARENT(UART_DUMMY_NODE)
#define EMUL_UART_RX_FIFO_SIZE DT_PROP(EMUL_UART_NODE, rx_fifo_size)
#define EMUL_UART_TX_FIFO_SIZE DT_PROP(EMUL_UART_NODE, tx_fifo_size)

/*
 * Leave one byte left in tx to avoid filling it completely which will block the UART
 * tx ready IRQ event.
 */
#define SAMPLE_DATA_SIZE MIN(EMUL_UART_RX_FIFO_SIZE, EMUL_UART_TX_FIFO_SIZE) - 1

struct uart_emul_device_fixture {
	const struct device *dev;
	uint8_t sample_data[SAMPLE_DATA_SIZE];
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

static void *uart_emul_device_setup(void)
{
	static struct uart_emul_device_fixture fixture = {.dev = DEVICE_DT_GET(EMUL_UART_NODE)};

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

static void uart_emul_device_before(void *f)
{
	struct uart_emul_device_fixture *fixture = f;

	uart_emul_flush_rx_data(fixture->dev);
	uart_emul_flush_tx_data(fixture->dev);

	uart_err_check(fixture->dev);

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

ZTEST_F(uart_emul_device, test_polling)
{
	uint32_t len;
	uint8_t byte;
	int ret;

	for (size_t i = 0; i < SAMPLE_DATA_SIZE; ++i) {
		uart_poll_out(fixture->dev, i);
	}

	len = uart_emul_get_tx_data(fixture->dev, NULL, UINT32_MAX);
	zassert_equal(len, 0, "TX buffer should be empty");

	for (size_t i = 0; i < SAMPLE_DATA_SIZE; ++i) {
		ret = uart_poll_in(fixture->dev, &byte);
		zassert_equal(ret, 0);
		zassert_equal(byte, i);
	}

	ret = uart_poll_in(fixture->dev, &byte);
	zassert_equal(ret, -1, "RX buffer should be empty");
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_emul_device_isr_handle_tx_ready(struct uart_emul_device_fixture *fixture)
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

static void uart_emul_device_isr_handle_rx_ready(struct uart_emul_device_fixture *fixture)
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

static void uart_emul_device_isr(const struct device *dev, void *user_data)
{
	struct uart_emul_device_fixture *fixture = user_data;

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_tx_ready(fixture->dev)) {
			uart_emul_device_isr_handle_tx_ready(fixture);
		}
		if (uart_irq_rx_ready(fixture->dev)) {
			uart_emul_device_isr_handle_rx_ready(fixture);
		}
	}
}

ZTEST_F(uart_emul_device, test_irq)
{
	size_t tx_len;
	int rc;

	uart_irq_callback_user_data_set(fixture->dev, uart_emul_device_isr, fixture);
	/* enabling the rx irq will call the callback, if set */
	uart_irq_rx_enable(fixture->dev);
	/* enabling the tx irq will call the callback, if set */
	uart_irq_tx_enable(fixture->dev);

	/* Wait for all data to be received in full */
	zassert_ok(k_sem_take(&fixture->tx_done_sem, K_SECONDS(1)),
		   "Timeout waiting for UART TX ISR");

	tx_len = uart_emul_get_tx_data(fixture->dev, NULL, SAMPLE_DATA_SIZE);
	zassert_equal(tx_len, 0, "TX buffer should be empty");

	zassert_ok(k_sem_take(&fixture->rx_done_sem, K_SECONDS(1)),
		   "Timeout waiting for UART RX ISR");
	zassert_mem_equal(fixture->rx_content, fixture->sample_data, SAMPLE_DATA_SIZE);

	/* No more data in RX buffer */
	rc = uart_poll_in(fixture->dev, &fixture->rx_content[0]);
	zassert_equal(rc, -1, "RX buffer should be empty");

	uart_irq_rx_disable(fixture->dev);
}
#endif

#ifdef CONFIG_UART_ASYNC_API
static void uart_emul_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	struct uart_emul_device_fixture *fixture = user_data;

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

static bool uart_emul_device_wait_for_event(struct uart_emul_device_fixture *fixture,
					    enum uart_event_type event)
{
	return k_event_wait(&fixture->async_events, ((uint32_t)1 << event), false, K_SECONDS(1)) !=
	       0;
}

ZTEST_F(uart_emul_device, test_async)
{
	size_t tx_len;

	uart_emul_set_release_buffer_on_timeout(fixture->dev, true);

	zassert_ok(uart_callback_set(fixture->dev, uart_emul_callback, fixture));
	zassert_ok(uart_tx(fixture->dev, fixture->sample_data, sizeof(fixture->sample_data),
			   SYS_FOREVER_US));
	zassert_ok(uart_rx_enable(fixture->dev, fixture->rx_content, sizeof(fixture->rx_content),
				  SYS_FOREVER_US));

	/* Wait for all data to be received in full */
	zexpect_true(uart_emul_device_wait_for_event(fixture, UART_TX_DONE),
		     "UART_TX_DONE event expected");

	tx_len = uart_emul_get_tx_data(fixture->dev, NULL, SAMPLE_DATA_SIZE);
	zassert_equal(tx_len, 0, "TX buffer should be empty");

	zexpect_true(uart_emul_device_wait_for_event(fixture, UART_RX_BUF_REQUEST),
		     "UART_RX_BUF_REQUEST event expected");
	zexpect_true(uart_emul_device_wait_for_event(fixture, UART_RX_RDY),
		     "UART_RX_RDY event expected");
	zassert_mem_equal(fixture->rx_content, fixture->sample_data, SAMPLE_DATA_SIZE);
	zexpect_true(uart_emul_device_wait_for_event(fixture, UART_RX_BUF_RELEASED),
		     "UART_RX_BUF_RELEASED event expected");
	zexpect_true(uart_emul_device_wait_for_event(fixture, UART_RX_DISABLED),
		     "UART_RX_DISABLED event expected");
}
#endif /* CONFIG_UART_ASYNC_API */

ZTEST_SUITE(uart_emul_device, NULL, uart_emul_device_setup, uart_emul_device_before, NULL, NULL);

/* Driver details */

/* Our dummy device echoes all data received */
static void uart_dummy_emul_tx_ready(const struct device *dev, size_t size,
				     const struct emul *target)
{
	uint32_t ret;
	uint8_t byte;

	zassert_equal(target->bus_type, EMUL_BUS_TYPE_UART, "UART bus required");

	for (size_t i = 0; i < size; ++i) {
		ret = uart_emul_get_tx_data(dev, &byte, 1);
		zassert_equal(ret, 1);

		ret = uart_emul_put_rx_data(dev, &byte, 1);
		zassert_equal(ret, 1);
	}
}

static const struct uart_emul_device_api dummy_api = {
	.tx_data_ready = uart_dummy_emul_tx_ready,
};

static int uart_dummy_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);

	return 0;
}

#define UART_DUMMY_DEFINE(inst)                                                                    \
	EMUL_DT_INST_DEFINE(inst, uart_dummy_emul_init, NULL, NULL, &dummy_api, NULL)

/* Define both device and emulated driver */
DT_INST_FOREACH_STATUS_OKAY(EMUL_STUB_DEVICE)
DT_INST_FOREACH_STATUS_OKAY(UART_DUMMY_DEFINE)
