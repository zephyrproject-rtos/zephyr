/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define CANARY 0xa5

static const struct device *const uart = DEVICE_DT_GET(DT_NODELABEL(dut));
static const uint8_t payload[] = {0x3c, 0xc3};
static K_SEM_DEFINE(rx_done, 0, 1);

/* Ownership passes from the test thread to the callback and back via rx_done. */
static struct {
	uint8_t buffer[6];
	uint8_t empty_byte;
	int capacity;
	int first;
	int second;
	int empty;
	bool zero_preserved;
} result;

static void rx_callback(const struct device *dev, void *user_data)
{
	int offset;

	ARG_UNUSED(user_data);
	uart_irq_update(dev);
	if (uart_irq_rx_ready(dev) <= 0) {
		return;
	}

	result.first = uart_fifo_read(dev, &result.buffer[1], result.capacity);
	result.zero_preserved = result.buffer[1] == CANARY;
	/* Keep the test itself bounds-safe even if the driver returns a bad count. */
	offset = CLAMP(result.first, 0, (int)sizeof(payload));
	result.second = uart_fifo_read(dev, &result.buffer[1 + offset], 4 - offset);
	result.empty = uart_fifo_read(dev, &result.empty_byte, 1);
	uart_irq_rx_disable(dev);
	k_sem_give(&rx_done);
}

static void *setup(void)
{
	zassert_true(device_is_ready(uart));
	uart_irq_rx_disable(uart);
	uart_irq_tx_disable(uart);
	zassert_ok(uart_irq_callback_user_data_set(uart, rx_callback, NULL));
	return NULL;
}

static void check_read(int capacity, int expected_first)
{
	uint8_t byte;
	int err;

	while (uart_poll_in(uart, &byte) == 0) {
	}
	memset(&result, 0, sizeof(result));
	memset(result.buffer, CANARY, sizeof(result.buffer));
	result.empty_byte = CANARY;
	result.capacity = capacity;
	k_sem_reset(&rx_done);

	/* RX remains enabled in hardware, but its callback cannot drain the FIFO. */
	for (size_t i = 0; i < ARRAY_SIZE(payload); i++) {
		uart_poll_out(uart, payload[i]);
	}
	/* Both 8N1 characters at 115200 baud must finish before enabling RX IRQs. */
	k_sleep(K_MSEC(2));
	uart_irq_rx_enable(uart);
	err = k_sem_take(&rx_done, K_SECONDS(1));
	uart_irq_rx_disable(uart);
	zassert_ok(err, "No receive callback");

	zassert_equal(result.first, expected_first);
	zassert_equal(result.second, ARRAY_SIZE(payload) - expected_first);
	zassert_mem_equal(&result.buffer[1], payload, sizeof(payload));
	zassert_equal(result.buffer[0], CANARY);
	for (size_t i = 1 + ARRAY_SIZE(payload); i < ARRAY_SIZE(result.buffer); i++) {
		zassert_equal(result.buffer[i], CANARY);
	}
	zassert_equal(result.empty, 0);
	zassert_equal(result.empty_byte, CANARY);
	if (capacity == 0) {
		zassert_true(result.zero_preserved);
	}
}

/** @brief A short read consumes all bytes already queued in the receive FIFO. */
ZTEST(uart_fifo_read, test_read_all_available)
{
	check_read(4, 2);
}

/** @brief Reading an exactly full destination returns its capacity. */
ZTEST(uart_fifo_read, test_exact_capacity)
{
	check_read(2, 2);
}

/** @brief A capacity-limited read leaves the next byte queued and in order. */
ZTEST(uart_fifo_read, test_capacity_limit)
{
	check_read(1, 1);
}

/** @brief A zero-capacity read preserves pending data and the destination. */
ZTEST(uart_fifo_read, test_zero_capacity)
{
	check_read(0, 0);
}

ZTEST_SUITE(uart_fifo_read, NULL, setup, NULL, NULL, NULL);
