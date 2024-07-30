/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample app for CDC ACM class driver
 *
 * Sample app for USB CDC ACM class driver. The received data is echoed back
 * to the serial port.
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdc_acm_composite, LOG_LEVEL_INF);

#define RING_BUF_SIZE	(64 * 2)

uint8_t buffer0[RING_BUF_SIZE];
uint8_t buffer1[RING_BUF_SIZE];

struct serial_peer {
	const struct device *dev;
	struct serial_peer *data;
	struct ring_buf rb;
};

#define DEFINE_SERIAL_PEER(node_id) { .dev = DEVICE_DT_GET(node_id), },
static struct serial_peer peers[] = {
	DT_FOREACH_STATUS_OKAY(zephyr_cdc_acm_uart, DEFINE_SERIAL_PEER)
};

BUILD_ASSERT(ARRAY_SIZE(peers) >= 2, "Not enough CDC ACM instances");

static void interrupt_handler(const struct device *dev, void *user_data)
{
	struct serial_peer *peer = user_data;

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		LOG_DBG("dev %p peer %p", dev, peer);

		if (uart_irq_rx_ready(dev)) {
			uint8_t buf[64];
			int read;
			size_t wrote;
			struct ring_buf *rb = &peer->data->rb;

			read = uart_fifo_read(dev, buf, sizeof(buf));
			if (read < 0) {
				LOG_ERR("Failed to read UART FIFO");
				read = 0;
			};

			wrote = ring_buf_put(rb, buf, read);
			if (wrote < read) {
				LOG_ERR("Drop %zu bytes", read - wrote);
			}

			LOG_DBG("dev %p -> dev %p send %zu bytes",
				dev, peer->dev, wrote);
			if (wrote) {
				uart_irq_tx_enable(peer->dev);
			}
		}

		if (uart_irq_tx_ready(dev)) {
			uint8_t buf[64];
			size_t wrote, len;

			len = ring_buf_get(&peer->rb, buf, sizeof(buf));
			if (!len) {
				LOG_DBG("dev %p TX buffer empty", dev);
				uart_irq_tx_disable(dev);
			} else {
				wrote = uart_fifo_fill(dev, buf, len);
				LOG_DBG("dev %p wrote len %zu", dev, wrote);
			}
		}
	}
}

static void uart_line_set(const struct device *dev)
{
	uint32_t baudrate;
	int ret;

	/* They are optional, we use them to test the interrupt endpoint */
	ret = uart_line_ctrl_set(dev, UART_LINE_CTRL_DCD, 1);
	if (ret) {
		LOG_DBG("Failed to set DCD, ret code %d", ret);
	}

	ret = uart_line_ctrl_set(dev, UART_LINE_CTRL_DSR, 1);
	if (ret) {
		LOG_DBG("Failed to set DSR, ret code %d", ret);
	}

	/* Wait 1 sec for the host to do all settings */
	k_busy_wait(1000000);

	ret = uart_line_ctrl_get(dev, UART_LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret) {
		LOG_DBG("Failed to get baudrate, ret code %d", ret);
	} else {
		LOG_DBG("Baudrate detected: %d", baudrate);
	}
}

int main(void)
{
	uint32_t dtr = 0U;
	int ret;

	for (int idx = 0; idx < ARRAY_SIZE(peers); idx++) {
		if (!device_is_ready(peers[idx].dev)) {
			LOG_ERR("CDC ACM device %s is not ready",
				peers[idx].dev->name);
			return 0;
		}
	}

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}

	LOG_INF("Wait for DTR");

	while (1) {
		uart_line_ctrl_get(peers[0].dev, UART_LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		}

		k_sleep(K_MSEC(100));
	}

	while (1) {
		uart_line_ctrl_get(peers[1].dev, UART_LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		}

		k_sleep(K_MSEC(100));
	}

	LOG_INF("DTR set, start test");

	uart_line_set(peers[0].dev);
	uart_line_set(peers[1].dev);

	peers[0].data = &peers[1];
	peers[1].data = &peers[0];

	ring_buf_init(&peers[0].rb, sizeof(buffer0), buffer0);
	ring_buf_init(&peers[1].rb, sizeof(buffer1), buffer1);

	uart_irq_callback_user_data_set(peers[1].dev, interrupt_handler, &peers[0]);
	uart_irq_callback_user_data_set(peers[0].dev, interrupt_handler, &peers[1]);

	/* Enable rx interrupts */
	uart_irq_rx_enable(peers[0].dev);
	uart_irq_rx_enable(peers[1].dev);
	return 0;
}
