/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Sample: Bluetooth Classic SPP UART Client
 *
 * Demonstrates SPP client mode: the device initiates a BR/EDR connection
 * to a remote SPP server, then exchanges data over the virtual UART.
 *
 * Usage:
 *   1. Start an SPP server on the remote device (phone/PC/another Zephyr board)
 *   2. Set REMOTE_ADDR below to the server's Bluetooth address
 *   3. Build and flash this sample
 *   4. The device connects, discovers SPP via SDP, and opens RFCOMM
 *   5. Data is exchanged via the UART API (echo + periodic TX)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <stdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spp_client_sample, LOG_LEVEL_INF);

/* Remote SPP server address - change this to your target device */
#define REMOTE_ADDR "00:11:22:33:44:55"

#define SPP_UART_NODE DT_NODELABEL(spp_uart0)
static const struct device *const spp_dev = DEVICE_DT_GET(SPP_UART_NODE);

static uint8_t rx_buf[64];

static void uart_irq_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			int len = uart_fifo_read(dev, rx_buf, sizeof(rx_buf));

			if (len > 0) {
				LOG_INF("RX %d bytes from server", len);
				/* Echo back */
				uart_fifo_fill(dev, rx_buf, len);
			}
		}
	}
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("BR/EDR connection failed (err %d)", err);
		return;
	}

	LOG_INF("BR/EDR connected - SPP UART driver will auto-connect RFCOMM");
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("BR/EDR disconnected (reason %u)", reason);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

int main(void)
{
	int err;
	bt_addr_t addr;
	struct bt_conn *conn;

	LOG_INF("Bluetooth Classic SPP UART Client sample");

	if (!device_is_ready(spp_dev)) {
		LOG_ERR("SPP UART device not ready");
		return -ENODEV;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}

	LOG_INF("Bluetooth initialized");
	bt_conn_cb_register(&conn_callbacks);

	/* Set up UART IRQ-driven RX with echo */
	uart_irq_callback_user_data_set(spp_dev, uart_irq_handler, NULL);
	uart_irq_rx_enable(spp_dev);
	uart_irq_tx_enable(spp_dev);

	/* Initiate BR/EDR connection to remote SPP server.
	 * The SPP UART driver's conn callback will automatically trigger
	 * SDP discovery and RFCOMM connection once ACL is established.
	 */
	err = bt_addr_from_str(REMOTE_ADDR, &addr);
	if (err) {
		LOG_ERR("Invalid remote address");
		return err;
	}

	LOG_INF("Connecting to %s ...", REMOTE_ADDR);
	conn = bt_conn_create_br(&addr, BT_BR_CONN_PARAM_DEFAULT);
	if (!conn) {
		LOG_ERR("Failed to create BR/EDR connection");
		return -EIO;
	}
	bt_conn_unref(conn);

	/* Main loop: send periodic data once connected */
	int count = 0;

	while (1) {
		k_sleep(K_SECONDS(5));
		count++;

		char msg[48];
		int len = snprintf(msg, sizeof(msg), "[client msg %d]\r\n", count);

		for (int i = 0; i < len; i++) {
			uart_poll_out(spp_dev, msg[i]);
		}
	}

	return 0;
}
