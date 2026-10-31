/*
 * Copyright (c) 2026 Mahdi Sahebi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#include "transport.h"

#define MC_UART_NODE       DT_ALIAS(mc_uart)
#define UART_RING_SIZE     131072U
#define UART_FRAME_SIZE    2048U
#define UART_FRAME_END     0xc0U
#define UART_FRAME_ESCAPE  0xdbU
#define UART_ESCAPED_END   0xdcU
#define UART_ESCAPED_ESCAPE 0xddU

static const struct device *const uart = DEVICE_DT_GET(MC_UART_NODE);
RING_BUF_DECLARE(uart_ring, UART_RING_SIZE);
static uint8_t frame[UART_FRAME_SIZE];
static uint8_t send_frame[UART_FRAME_SIZE];
static size_t frame_length;
static bool frame_started;
static bool frame_escaped;
static bool frame_discarded;
static bool uart_ready;
static uint32_t random_state;
static uint32_t frame_count;
static uint8_t current_loss_rate;
static bool inject_duplicate;
static bool inject_corruption;
static bool inject_short_frame;
static bool inject_disconnection;

static uint32_t next_random(void)
{
	random_state = random_state * 1664525U + 1013904223U;
	return random_state;
}

static void uart_rx_callback(const struct device *device, void *user_data)
{
	uint8_t bytes[64];

	ARG_UNUSED(user_data);
	uart_irq_update(device);
	while (uart_irq_rx_ready(device)) {
		int length = uart_fifo_read(device, bytes, sizeof(bytes));

		if (length <= 0) {
			break;
		}
		(void)ring_buf_put(&uart_ring, bytes, (uint32_t)length);
	}
}

int stress_transport_init(void)
{
	if (uart_ready) {
		return 0;
	}
	if (!device_is_ready(uart)) {
		return -ENODEV;
	}
	int err = uart_irq_callback_user_data_set(uart, uart_rx_callback, NULL);

	if (err != 0) {
		return err;
	}
	uart_irq_rx_enable(uart);
	uart_ready = true;
	return 0;
}

void stress_transport_reset(uint8_t loss_rate, bool duplicate, bool corrupt,
			    bool shorten, bool disconnect)
{
	uart_irq_rx_disable(uart);
	ring_buf_reset(&uart_ring);
	uart_irq_rx_enable(uart);
	frame_length = 0U;
	frame_started = false;
	frame_escaped = false;
	frame_discarded = false;
	random_state = 1U;
	frame_count = 0U;
	current_loss_rate = loss_rate;
	inject_duplicate = duplicate;
	inject_corruption = corrupt;
	inject_short_frame = shorten;
	inject_disconnection = disconnect;
}

void *stress_transport_context(bool sender)
{
	ARG_UNUSED(sender);
	return NULL;
}

void stress_transport_set_peer(bool sender, struct mc_comm *peer)
{
	ARG_UNUSED(sender);
	ARG_UNUSED(peer);
}

int stress_transport_receive(void *context, uint8_t *data, size_t capacity)
{
	uint8_t byte;

	ARG_UNUSED(context);
	while (ring_buf_get(&uart_ring, &byte, 1U) == 1U) {
		if (byte == UART_FRAME_END) {
			if (frame_started && !frame_discarded && !frame_escaped &&
			    frame_length > 0U) {
				size_t length = frame_length;

				frame_length = 0U;
				if (length > capacity) {
					return -EMSGSIZE;
				}
				memcpy(data, frame, length);
				return (int)length;
			}
			frame_length = 0U;
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
		if (frame_length < sizeof(frame)) {
			frame[frame_length++] = byte;
		} else {
			frame_discarded = true;
		}
	}
	return -EAGAIN;
}

static void write_frame(const uint8_t *data, size_t length)
{
	uart_poll_out(uart, UART_FRAME_END);
	for (size_t i = 0U; i < length; i++) {
		if (data[i] == UART_FRAME_END || data[i] == UART_FRAME_ESCAPE) {
			uart_poll_out(uart, UART_FRAME_ESCAPE);
			uart_poll_out(uart, data[i] == UART_FRAME_END ?
					      UART_ESCAPED_END : UART_ESCAPED_ESCAPE);
		} else {
			uart_poll_out(uart, data[i]);
		}
	}
	uart_poll_out(uart, UART_FRAME_END);
}

int stress_transport_transmit(void *context, const uint8_t *data, size_t length)
{
	size_t original_length = length;

	ARG_UNUSED(context);
	if (length > sizeof(send_frame)) {
		return -EMSGSIZE;
	}
	frame_count++;
	if ((inject_disconnection && frame_count % 32U < 8U) ||
	    (current_loss_rate > 0U && next_random() % 100U < current_loss_rate)) {
		return (int)length;
	}
	memcpy(send_frame, data, length);
	if (inject_corruption && frame_count % 11U == 0U) {
		send_frame[length - 1U] ^= 1U;
	}
	if (inject_short_frame && frame_count % 13U == 0U && length > 1U) {
		length--;
	}
	write_frame(send_frame, length);
	if (inject_duplicate && frame_count % 7U == 0U) {
		k_msleep(5U);
		write_frame(send_frame, length);
	}
	return (int)original_length;
}
