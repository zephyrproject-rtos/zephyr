/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/mc/comm.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>

#if DT_HAS_ALIAS(mc_uart)
#define MC_UART_NODE DT_ALIAS(mc_uart)
#else
#error "MC UART test requires a mc-uart devicetree alias"
#endif

static const struct device *const uart = DEVICE_DT_GET(MC_UART_NODE);
static uint8_t uart_rx_buffer[128];
static size_t uart_rx_length;
RING_BUF_DECLARE(uart_rx_ring, 256);

static void uart_rx_callback(const struct device *dev, void *user_data)
{
	uint8_t buffer[32];

	ARG_UNUSED(user_data);
	uart_irq_update(dev);
	while (uart_irq_rx_ready(dev)) {
		int length = uart_fifo_read(dev, buffer, sizeof(buffer));

		if (length <= 0) {
			break;
		}
		(void)ring_buf_put(&uart_rx_ring, buffer, (uint32_t)length);
	}
}

static int uart_receive(void *context, uint8_t *data, size_t capacity)
{
	ARG_UNUSED(context);

	while (uart_rx_length < sizeof(uart_rx_buffer)) {
		uint32_t received = ring_buf_get(&uart_rx_ring,
			&uart_rx_buffer[uart_rx_length], 1U);

		if (received == 0U) {
			break;
		}
		uart_rx_length += received;
	}

	while (uart_rx_length >= 2U &&
	       (uart_rx_buffer[0] != 0xc7U || uart_rx_buffer[1] != 0xe9U)) {
		memmove(uart_rx_buffer, &uart_rx_buffer[1], --uart_rx_length);
	}
	if (uart_rx_length < 12U) {
		return -EAGAIN;
	}

	size_t payload_length = ((size_t)uart_rx_buffer[8] << 8U) |
		uart_rx_buffer[9];
	size_t frame_length = 12U + payload_length;

	if (frame_length > capacity || frame_length > sizeof(uart_rx_buffer)) {
		memmove(uart_rx_buffer, &uart_rx_buffer[1], --uart_rx_length);
		return -EBADMSG;
	}
	if (uart_rx_length < frame_length) {
		return -EAGAIN;
	}

	memcpy(data, uart_rx_buffer, frame_length);
	uart_rx_length -= frame_length;
	memmove(uart_rx_buffer, &uart_rx_buffer[frame_length], uart_rx_length);

	return (int)frame_length;
}

static int uart_transmit(void *context, const uint8_t *data, size_t length)
{
	ARG_UNUSED(context);

	for (size_t i = 0U; i < length; i++) {
		uart_poll_out(uart, data[i]);
	}
	return (int)length;
}

static bool wait_for_packet(struct mc_comm *comm, const uint8_t *expected, size_t expected_length)
{
	uint8_t data[32];
	size_t length;
	int64_t deadline = k_uptime_get() + 5000;

	while (k_uptime_get() < deadline) {
		int err = mc_comm_receive(comm, data, sizeof(data), &length, 0U);

		if (err == 0) {
			return length == expected_length &&
			       (expected_length == 0U ||
				memcmp(data, expected, expected_length) == 0);
		}
		if (err != -ETIMEDOUT) {
			return false;
		}
		k_msleep(1U);
	}

	return false;
}

ZTEST(mc_uart, test_communication_transport)
{
	static uint8_t storage[512] __aligned(8);
	struct mc_comm *comm;
	struct mc_comm_config config = {
		.receive = uart_receive,
		.transmit = uart_transmit,
		.frame_size = 64U,
		.receive_capacity = 64U,
	};

	zassert_true(device_is_ready(uart), NULL);
	zassert_ok(uart_irq_callback_user_data_set(uart, uart_rx_callback, NULL), NULL);
	uart_irq_rx_enable(uart);
	zassert_ok(mc_comm_init(&comm, storage, sizeof(storage), &config), NULL);

	if (IS_ENABLED(CONFIG_MC_UART_SENDER)) {
		k_msleep(1000U);
		zassert_ok(mc_comm_send(comm, "hello", 5U, 5000000U), NULL);
		zassert_ok(mc_comm_send(comm, NULL, 0U, 5000000U), NULL);
		printk("MC UART sender: PASS\n");
	} else {
		zassert_true(wait_for_packet(comm, (const uint8_t *)"hello", 5U), NULL);
		zassert_true(wait_for_packet(comm, NULL, 0U), NULL);
		printk("MC UART receiver: PASS\n");
	}
}

ZTEST_SUITE(mc_uart, NULL, NULL, NULL, NULL, NULL);
