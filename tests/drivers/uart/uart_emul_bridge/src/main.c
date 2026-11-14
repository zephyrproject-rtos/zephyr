/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

/*
 * Each bridge joins two emulated UARTs. The test plays the wire of the "uart"
 * one with the backend API and the application on the "emul" one with the
 * serial API.
 */
#define UART_BOTH     DEVICE_DT_GET(DT_NODELABEL(test_uart_both))
#define EMUL_BOTH     DEVICE_DT_GET(DT_NODELABEL(test_emul_both))
#define UART_FORWARD  DEVICE_DT_GET(DT_NODELABEL(test_uart_forward))
#define EMUL_FORWARD  DEVICE_DT_GET(DT_NODELABEL(test_emul_forward))
#define UART_BACKWARD DEVICE_DT_GET(DT_NODELABEL(test_uart_backward))
#define EMUL_BACKWARD DEVICE_DT_GET(DT_NODELABEL(test_emul_backward))

/* Bytes the wire takes at a time, less than the TX FIFO of the UART holds */
#define WIRE_CHUNK 4U

#define SHORT_TIMEOUT K_MSEC(100)

struct writer {
	const uint8_t *data;
	size_t len;
	size_t sent;
};

/* What the UART sent, taken off the wire a few bytes per tick as a slow line does */
static size_t wire_read(const struct device *uart, uint8_t *buf, size_t len, k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);
	size_t got = 0;

	while ((got < len) && !sys_timepoint_expired(end)) {
		got += uart_emul_get_tx_data(uart, &buf[got], MIN(len - got, WIRE_CHUNK));
		k_msleep(1);
	}

	return got;
}

/* What the application receives on the emulated UART */
static size_t emul_read(const struct device *emul, uint8_t *buf, size_t len, k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);
	size_t got = 0;

	while ((got < len) && !sys_timepoint_expired(end)) {
		if (uart_poll_in(emul, &buf[got]) == 0) {
			got++;
		} else {
			k_msleep(1);
		}
	}

	return got;
}

static void emul_write(const struct device *emul, const uint8_t *data, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		uart_poll_out(emul, data[i]);
	}
}

/* An interrupt-driven application: sends what the UART accepts and waits for the rest */
static void writer_isr(const struct device *dev, void *user_data)
{
	struct writer *writer = user_data;

	uart_irq_update(dev);

	if (uart_irq_tx_ready(dev) <= 0) {
		return;
	}

	if (writer->sent < writer->len) {
		int n = uart_fifo_fill(dev, &writer->data[writer->sent],
				       (int)(writer->len - writer->sent));

		if (n > 0) {
			writer->sent += (size_t)n;
		}
	}

	if (writer->sent == writer->len) {
		uart_irq_tx_disable(dev);
	}
}

static void check_forward(const struct device *uart, const struct device *emul)
{
	static const uint8_t msg[] = "to the emulated UART";
	uint8_t buf[sizeof(msg)];

	zassert_equal(uart_emul_put_rx_data(uart, msg, sizeof(msg)), sizeof(msg));
	zassert_equal(emul_read(emul, buf, sizeof(buf), SHORT_TIMEOUT), sizeof(msg));
	zassert_mem_equal(buf, msg, sizeof(msg));
}

static void check_backward(const struct device *uart, const struct device *emul)
{
	static const uint8_t msg[] = "to the UART";
	uint8_t buf[sizeof(msg)];

	emul_write(emul, msg, sizeof(msg));
	zassert_equal(wire_read(uart, buf, sizeof(buf), SHORT_TIMEOUT), sizeof(msg));
	zassert_mem_equal(buf, msg, sizeof(msg));
}

ZTEST(uart_emul_bridge, test_both)
{
	check_forward(UART_BOTH, EMUL_BOTH);
	check_backward(UART_BOTH, EMUL_BOTH);
}

ZTEST(uart_emul_bridge, test_forward_only)
{
	uint8_t buf[4];

	check_forward(UART_FORWARD, EMUL_FORWARD);

	emul_write(EMUL_FORWARD, (const uint8_t *)"back", 4);
	zassert_equal(wire_read(UART_FORWARD, buf, sizeof(buf), SHORT_TIMEOUT), 0,
		      "the forward bridge sent data to the UART");
}

ZTEST(uart_emul_bridge, test_backward_only)
{
	uint8_t buf[4];

	check_backward(UART_BACKWARD, EMUL_BACKWARD);

	zassert_equal(uart_emul_put_rx_data(UART_BACKWARD, (const uint8_t *)"forw", 4), 4);
	zassert_equal(emul_read(EMUL_BACKWARD, buf, sizeof(buf), SHORT_TIMEOUT), 0,
		      "the backward bridge passed data to the emulated UART");
}

/* Polled output is not held back: a burst waits in the buffers until the UART sends it */
ZTEST(uart_emul_bridge, test_backward_polled_burst)
{
	uint8_t data[48];
	uint8_t buf[sizeof(data)];

	for (size_t i = 0; i < sizeof(data); i++) {
		data[i] = (uint8_t)('0' + i);
	}

	emul_write(EMUL_BOTH, data, sizeof(data));
	zassert_equal(wire_read(UART_BOTH, buf, sizeof(buf), K_SECONDS(1)), sizeof(data),
		      "data was lost on the way to the UART");
	zassert_mem_equal(buf, data, sizeof(data));
}

/* An interrupt-driven writer is held back by a UART much slower than it */
ZTEST(uart_emul_bridge, test_backward_slow_uart)
{
	static uint8_t data[512];
	static uint8_t buf[sizeof(data)];
	struct writer writer = {.data = data, .len = sizeof(data)};
	size_t got;

	for (size_t i = 0; i < sizeof(data); i++) {
		data[i] = (uint8_t)i;
	}

	zassert_ok(uart_irq_callback_user_data_set(EMUL_BOTH, writer_isr, &writer));
	uart_irq_tx_enable(EMUL_BOTH);

	got = wire_read(UART_BOTH, buf, sizeof(buf), K_SECONDS(10));

	uart_irq_tx_disable(EMUL_BOTH);
	zassert_ok(uart_irq_callback_user_data_set(EMUL_BOTH, NULL, NULL));

	zassert_equal(writer.sent, sizeof(data), "the writer sent %zu of %zu bytes", writer.sent,
		      sizeof(data));
	zassert_equal(got, sizeof(data), "the UART sent %zu of %zu bytes", got, sizeof(data));
	zassert_mem_equal(buf, data, sizeof(data));
}

static void uart_emul_bridge_before(void *fixture)
{
	static const struct device *const devs[] = {
		UART_BOTH, EMUL_BOTH, UART_FORWARD, EMUL_FORWARD, UART_BACKWARD, EMUL_BACKWARD,
	};

	uint8_t scratch[32];
	uint32_t left;

	ARG_UNUSED(fixture);

	/* Until the bridges have handed over what an earlier test left in them */
	do {
		left = 0U;
		ARRAY_FOR_EACH_PTR(devs, dev) {
			zassert_true(device_is_ready(*dev));
			left += uart_emul_flush_rx_data(*dev);
			left += uart_emul_get_tx_data(*dev, scratch, sizeof(scratch));
		}
		k_msleep(1);
	} while (left > 0U);
}

ZTEST_SUITE(uart_emul_bridge, NULL, NULL, uart_emul_bridge_before, NULL, NULL);
