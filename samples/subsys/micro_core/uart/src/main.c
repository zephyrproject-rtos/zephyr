/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/mc/comm.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/ring_buffer.h>

#define MC_UART_NODE DT_ALIAS(mc_uart)

static const struct device *const uart = DEVICE_DT_GET(MC_UART_NODE);
static uint8_t rx_frame[128];
static size_t rx_length;
RING_BUF_DECLARE(rx_ring, 256);

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
		(void)ring_buf_put(&rx_ring, buffer, (uint32_t)length);
	}
}

static int read_frame(void *context, uint8_t *data, size_t capacity)
{
	ARG_UNUSED(context);

	while (rx_length < sizeof(rx_frame)) {
		uint32_t received = ring_buf_get(&rx_ring, &rx_frame[rx_length], 1U);

		if (received == 0U) {
			break;
		}
		rx_length += received;
	}
	while (rx_length >= 2U && (rx_frame[0] != 0xc7U || rx_frame[1] != 0xe9U)) {
		memmove(rx_frame, &rx_frame[1], --rx_length);
	}
	if (rx_length < 12U) {
		return -EAGAIN;
	}
	size_t frame_length = 12U + ((size_t)rx_frame[8] << 8U) + rx_frame[9];

	if (frame_length > capacity || frame_length > sizeof(rx_frame)) {
		memmove(rx_frame, &rx_frame[1], --rx_length);
		return -EBADMSG;
	}
	if (rx_length < frame_length) {
		return -EAGAIN;
	}
	memcpy(data, rx_frame, frame_length);
	rx_length -= frame_length;
	memmove(rx_frame, &rx_frame[frame_length], rx_length);
	return (int)frame_length;
}

static int write_frame(void *context, const uint8_t *data, size_t length)
{
	ARG_UNUSED(context);

	for (size_t i = 0U; i < length; i++) {
		uart_poll_out(uart, data[i]);
	}
	return (int)length;
}

static bool await_payload(struct mc_comm *comm, const uint8_t *expected, size_t expected_length)
{
	uint8_t data[32];
	size_t length;
	int64_t deadline = k_uptime_get() + 5000;

	while (k_uptime_get() < deadline) {
		int err = mc_comm_recv(comm, data, sizeof(data), &length, 0U);

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

int main(void)
{
	static uint8_t storage[512] __aligned(8);
	struct mc_comm *comm;
	struct mc_comm_cfg config = {
		.io.receive = read_frame,
		.io.transmit = write_frame,
		.recv.size = 64U,
		.send.size = 64U,
		.receive_capacity = 64U,
	};
	int err;

	if (!device_is_ready(uart) || mc_comm_req_size(&config) > sizeof(storage)) {
		printk("Micro Core UART: setup failed\n");
		return -ENODEV;
	}
	err = uart_irq_callback_user_data_set(uart, uart_rx_callback, NULL);
	if (err != 0) {
		return err;
	}
	uart_irq_rx_enable(uart);
	err = mc_comm_init(&comm, storage, sizeof(storage), &config);
	if (err != 0) {
		return err;
	}
	if (IS_ENABLED(CONFIG_MC_SAMPLE_SENDER)) {
		k_msleep(1000U);
		err = mc_comm_send(comm, "hello", 5U, 5000000U);
		if (err == 0) {
			err = mc_comm_send(comm, NULL, 0U, 5000000U);
		}
	} else {
		err = await_payload(comm, (const uint8_t *)"hello", 5U) &&
				      await_payload(comm, NULL, 0U)
			      ? 0
			      : -ETIMEDOUT;
	}
	printk("Micro Core UART: %s\n", err == 0 ? "PASS" : "FAIL");
	return err;
}
