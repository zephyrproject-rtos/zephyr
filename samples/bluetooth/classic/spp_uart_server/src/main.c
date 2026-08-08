/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Sample: Bluetooth Classic SPP UART
 *
 * Demonstrates how to use the SPP UART driver to exchange serial data
 * over Bluetooth Classic (RFCOMM). The device acts as an SPP server;
 * a remote device (phone/PC) connects via SPP and can send/receive data
 * through the virtual UART.
 *
 * Usage:
 *   1. Build and flash this sample
 *   2. Make the device discoverable (it starts discoverable by default)
 *   3. From a phone/PC SPP client, connect to "Zephyr SPP"
 *   4. Data sent from the remote appears on the Zephyr console
 *   5. Zephyr echoes received data back to the remote
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <stdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spp_uart_sample, LOG_LEVEL_INF);

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
				LOG_INF("RX %d bytes", len);
				/* Echo back */
				uart_fifo_fill(dev, rx_buf, len);
			}
		}
	}
}

int main(void)
{
	int err;

	LOG_INF("Bluetooth Classic SPP UART sample");

	if (!device_is_ready(spp_dev)) {
		LOG_ERR("SPP UART device not ready");
		return -ENODEV;
	}

	/* Initialize Bluetooth */
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}

	LOG_INF("Bluetooth initialized");

	/* Make device connectable and discoverable */
	err = bt_br_set_connectable(true, NULL);
	if (err) {
		LOG_ERR("BR/EDR set connectable failed (err %d)", err);
		return err;
	}

	err = bt_br_set_discoverable(true, false);
	if (err) {
		LOG_ERR("BR/EDR set discoverable failed (err %d)", err);
		return err;
	}

	LOG_INF("Discoverable as \"%s\", waiting for SPP connection...",
		CONFIG_BT_DEVICE_NAME);

	/* Set up UART IRQ-driven RX with echo */
	uart_irq_callback_user_data_set(spp_dev, uart_irq_handler, NULL);
	uart_irq_rx_enable(spp_dev);
	uart_irq_tx_enable(spp_dev);

	LOG_INF("SPP UART ready (IRQ-driven echo mode)");

	/* Main loop: also demonstrate poll-based TX */
	int count = 0;

	while (1) {
		k_sleep(K_SECONDS(10));
		count++;

		/* Periodic heartbeat message via poll_out */
		char msg[32];
		int len = snprintf(msg, sizeof(msg), "[heartbeat %d]\r\n", count);

		for (int i = 0; i < len; i++) {
			uart_poll_out(spp_dev, msg[i]);
		}
	}

	return 0;
}
