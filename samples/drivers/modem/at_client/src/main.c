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
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(at_client, CONFIG_MODEM_MODULES_LOG_LEVEL);

#define DEV_CONSOLE DEVICE_DT_GET(DT_CHOSEN(zephyr_console))
#define DEV_MODEM   DEVICE_DT_GET(DT_CHOSEN(zephyr_modem_uart))

#define UART_RX_BUF_SIZE    128
#define UART_TX_BUF_SIZE    128
#define CONSOLE_RX_BUF_SIZE 128

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

/* Console uart_pipe callback - receives data from console */
static uint8_t *console_recv_cb(uint8_t *buf, size_t *off)
{
	int ret;

	/* Data received from console, send directly to modem pipe */
	if (*off > 0) {
		ret = modem_pipe_transmit(data.uart_pipe, buf, *off);
		if (ret < 0) {
			LOG_ERR("Failed to transmit to modem: %d", ret);
		}
		*off = 0;
	}

	return buf;
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

	/* Keep application running */
	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
