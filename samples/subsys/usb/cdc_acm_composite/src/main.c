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
#include <device.h>
#include <drivers/uart.h>
#include <zephyr.h>
#include <sys/ring_buffer.h>

#include <usb/usb_device.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(cdc_acm_composite, LOG_LEVEL_INF);

#define RING_BUF_SIZE	(64 * 2)

uint8_t buffer0[RING_BUF_SIZE];
uint8_t buffer1[RING_BUF_SIZE];

static struct serial_data {
	const struct device *peer;
	struct serial_data *peer_data;
	struct ring_buf ringbuf;
} peers[2];

static void interrupt_handler(const struct device *dev, void *user_data)
{
	struct serial_data *dev_data = user_data;

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		const struct device *peer = dev_data->peer;

		LOG_DBG("dev %p dev_data %p", dev, dev_data);

		if (uart_irq_rx_ready(dev)) {
			uint8_t buf[64];
			size_t read, wrote;
			struct ring_buf *ringbuf =
					&dev_data->peer_data->ringbuf;

			read = uart_fifo_read(dev, buf, sizeof(buf));
			if (read) {
				wrote = ring_buf_put(ringbuf, buf, read);
				if (wrote < read) {
					LOG_ERR("Drop %zu bytes", read - wrote);
				}

				uart_irq_tx_enable(dev_data->peer);

				LOG_DBG("dev %p -> dev %p send %zu bytes",
					dev, peer, wrote);
			}
		}

		if (uart_irq_tx_ready(dev)) {
			uint8_t buf[64];
			size_t wrote, len;

			len = ring_buf_get(&dev_data->ringbuf, buf,
					   sizeof(buf));
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

void main(void)
{
	int ret;

	struct serial_data *dev_data0 = &peers[0];
	struct serial_data *dev_data1 = &peers[1];
	const struct device *dev0, *dev1;
	uint32_t dtr = 0U;

	dev0 = device_get_binding("CDC_ACM_0");
	if (!dev0) {
		LOG_DBG("CDC_ACM_0 device not found");
		return;
	}

	dev1 = device_get_binding("CDC_ACM_1");
	if (!dev1) {
		LOG_DBG("CDC_ACM_1 device not found");
		return;
	}

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	LOG_INF("Wait for DTR");

	while (1) {
		uart_line_ctrl_get(dev0, UART_LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		}

		k_sleep(K_MSEC(100));
	}

	while (1) {
		uart_line_ctrl_get(dev1, UART_LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		}

		k_sleep(K_MSEC(100));
	}

	LOG_INF("DTR set, start test");

	uart_line_set(dev0);
	uart_line_set(dev1);

	dev_data0->peer = dev1;
	dev_data0->peer_data = dev_data1;
	ring_buf_init(&dev_data0->ringbuf, sizeof(buffer0), buffer0);

	dev_data1->peer = dev0;
	dev_data1->peer_data = dev_data0;
	ring_buf_init(&dev_data1->ringbuf, sizeof(buffer1), buffer1);

	uart_irq_callback_user_data_set(dev0, interrupt_handler, dev_data0);
	uart_irq_callback_user_data_set(dev1, interrupt_handler, dev_data1);

	/* Enable rx interrupts */
	uart_irq_rx_enable(dev0);
	uart_irq_rx_enable(dev1);
}
