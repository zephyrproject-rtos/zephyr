/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AT Command Client - Bidirectional UART bridge
 *
 * This application creates a transparent bridge between the console UART
 * and a cellular modem UART, allowing AT command interaction and firmware
 * updates via XMODEM protocol.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart_pipe.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(at_client, CONFIG_MODEM_MODULES_LOG_LEVEL);

#define DEV_CONSOLE DEVICE_DT_GET(DT_CHOSEN(zephyr_console))
#define DEV_MODEM   DEVICE_DT_GET(DT_CHOSEN(zephyr_modem_uart))

#define UART_RX_BUF_SIZE    128
#define UART_TX_BUF_SIZE    128
#define CONSOLE_RX_BUF_SIZE 128
#define CONSOLE_RING_SIZE   256
#define MODEM_TX_TIMEOUT    K_SECONDS(1)

/* Buffers for modem backend */
struct modem_data {
	struct {
		uint8_t uart_rx[UART_RX_BUF_SIZE];
		uint8_t uart_tx[UART_TX_BUF_SIZE];
	} buffers;

	struct modem_backend_uart uart_backend;
	struct modem_pipe *uart_pipe;
};

static struct modem_data data;

/* Console RX buffer for uart_pipe */
static uint8_t buf_rx_console[CONSOLE_RX_BUF_SIZE];

/* Hands console data from the uart_pipe callback (ISR) over to the main thread */
RING_BUF_DECLARE(console_ring, CONSOLE_RING_SIZE);
static K_SEM_DEFINE(console_data_sem, 0, 1);

/* Callback when modem pipe receives data */
static void modem_pipe_event_handler(struct modem_pipe *pipe, enum modem_pipe_event event,
				     void *user_data)
{
	uint8_t buf[CONSOLE_RX_BUF_SIZE];
	int ret;

	switch (event) {
	case MODEM_PIPE_EVENT_RECEIVE_READY:
		/* Read from modem pipe and send directly to console */
		do {
			ret = modem_pipe_receive(pipe, buf, sizeof(buf));
			if (ret > 0) {
				/* Send directly to console via uart_pipe */
				uart_pipe_send(buf, ret);
			}
		} while (ret > 0);
		break;

	case MODEM_PIPE_EVENT_TRANSMIT_IDLE:
		/* Can send more data if available */
		break;

	default:
		break;
	}
}

static int init_modem_pipe(void)
{
	int ret;

	const struct modem_backend_uart_config uart_backend_config = {
		.uart = DEV_MODEM,
		.receive_buf = data.buffers.uart_rx,
		.receive_buf_size = sizeof(data.buffers.uart_rx),
		.transmit_buf = data.buffers.uart_tx,
		.transmit_buf_size = sizeof(data.buffers.uart_tx),
	};

	data.uart_pipe = modem_backend_uart_init(&data.uart_backend, &uart_backend_config);
	if (data.uart_pipe == NULL) {
		LOG_ERR("Failed to initialize modem backend");
		return -1;
	}

	modem_pipe_attach(data.uart_pipe, modem_pipe_event_handler, &data);

	ret = modem_pipe_open(data.uart_pipe, K_MSEC(100));
	if (ret < 0) {
		LOG_ERR("Failed to open modem pipe");
		return ret;
	}

	LOG_INF("Modem pipe initialized and opened");
	return 0;
}

/* Console uart_pipe callback - runs in ISR context, so only queue the data */
static uint8_t *console_recv_cb(uint8_t *buf, size_t *off)
{
	if (*off > 0) {
		if (ring_buf_put(&console_ring, buf, *off) < *off) {
			LOG_WRN("Console data dropped");
		}
		k_sem_give(&console_data_sem);
		*off = 0;
	}

	return buf;
}

/* Forward queued console data to the modem pipe, blocking as long as the pipe is full */
static void forward_console_data(void)
{
	uint8_t *chunk;
	uint32_t size;
	int ret;

	while ((size = ring_buf_get_ptr(&console_ring, &chunk, 0)) > 0) {
		ret = modem_pipe_transmit_blocking(data.uart_pipe, chunk, size, MODEM_TX_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("Failed to transmit to modem: %d", ret);
		}
		ring_buf_consume(&console_ring, size);
	}
}

int main(void)
{
	int ret;

	LOG_INF("AT Command Client starting...");

	/* Verify console device is ready */
	if (!device_is_ready(DEV_CONSOLE)) {
		LOG_ERR("Console device not ready");
		return -ENODEV;
	}

	/* Verify modem device is ready */
	if (!device_is_ready(DEV_MODEM)) {
		LOG_ERR("Modem device not ready");
		return -ENODEV;
	}

	/* Register console uart_pipe for receiving */
	uart_pipe_register(buf_rx_console, sizeof(buf_rx_console), console_recv_cb);
	LOG_INF("Console UART pipe registered");

	/* Initialize modem pipe */
	ret = init_modem_pipe();
	if (ret < 0) {
		LOG_ERR("Failed to initialize modem pipe: %d", ret);
		return ret;
	}

	LOG_INF("Console <-> Modem communication established");
	LOG_INF("Ready to forward AT commands");

	while (1) {
		k_sem_take(&console_data_sem, K_FOREVER);
		forward_console_data();
	}

	return 0;
}
