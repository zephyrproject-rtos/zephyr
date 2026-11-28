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

#define MC_UART_NODE        DT_ALIAS(mc_uart)
#define UART_FRAME_END      0xc0U
#define UART_FRAME_ESCAPE   0xdbU
#define UART_ESCAPED_END    0xdcU
#define UART_ESCAPED_ESCAPE 0xddU

static const struct device *const uart = DEVICE_DT_GET(MC_UART_NODE);
static uint8_t rx_frame[128];
static size_t rx_length;
static bool frame_started;
static bool frame_escaped;
static bool frame_discarded;
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
	uint8_t byte;

	ARG_UNUSED(context);

	while (ring_buf_get(&rx_ring, &byte, 1U) == 1U) {
		if (byte == UART_FRAME_END) {
			if (frame_started && !frame_discarded && !frame_escaped && rx_length > 0U) {
				size_t length = rx_length;

				rx_length = 0U;
				if (length > capacity) {
					frame_started = true;
					frame_escaped = false;
					frame_discarded = false;
					continue;
				}
				memcpy(data, rx_frame, length);
				return (int)length;
			}
			rx_length = 0U;
			frame_started = true;
			frame_escaped = false;
			frame_discarded = false;
			continue;
		}
		if (!frame_started || frame_discarded) {
			continue;
		}
		if (frame_escaped) {
			frame_escaped = false;
			if (byte == UART_ESCAPED_END) {
				byte = UART_FRAME_END;
			} else if (byte == UART_ESCAPED_ESCAPE) {
				byte = UART_FRAME_ESCAPE;
			} else {
				frame_discarded = true;
				continue;
			}
		} else if (byte == UART_FRAME_ESCAPE) {
			frame_escaped = true;
			continue;
		}
		if (rx_length < sizeof(rx_frame)) {
			rx_frame[rx_length++] = byte;
		} else {
			frame_discarded = true;
		}
	}
	return -EAGAIN;
}

static int write_frame(void *context, const uint8_t *data, size_t length)
{
	ARG_UNUSED(context);

	uart_poll_out(uart, UART_FRAME_END);
	for (size_t i = 0U; i < length; i++) {
		if (data[i] == UART_FRAME_END || data[i] == UART_FRAME_ESCAPE) {
			uart_poll_out(uart, UART_FRAME_ESCAPE);
			uart_poll_out(uart, data[i] == UART_FRAME_END ? UART_ESCAPED_END
								      : UART_ESCAPED_ESCAPE);
		} else {
			uart_poll_out(uart, data[i]);
		}
	}
	uart_poll_out(uart, UART_FRAME_END);
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
		.receive_capacity = 32U,
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
