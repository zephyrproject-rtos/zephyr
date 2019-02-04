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
#include <uart.h>
#include <zephyr.h>

static const char *banner = "Send characters to another UART device\r\n";

static struct serial_data {
	struct device *dev;
	struct device *peer;
} peers[2];

static void interrupt_handler(void *user_data)
{
	const struct serial_data *dev_data = user_data;
	struct device *dev = dev_data->dev;

	uart_irq_update(dev);

	if (uart_irq_tx_ready(dev)) {
		/* TODO */
	}

	if (uart_irq_rx_ready(dev)) {
		struct device *peer = dev_data->peer;
		u8_t byte;

		uart_fifo_read(dev, &byte, sizeof(byte));
		uart_poll_out(peer, byte);

		printk("dev %p -> dev %p Sent byte %c\n", dev, peer, byte);
	}
}

static void write_data(struct device *dev, const char *buf, int len)
{
	uart_fifo_fill(dev, (const u8_t *)buf, len);
}

static void uart_line_set(struct device *dev)
{
	u32_t baudrate;
	int ret;

	/* They are optional, we use them to test the interrupt endpoint */
	ret = uart_line_ctrl_set(dev, LINE_CTRL_DCD, 1);
	if (ret) {
		printf("Failed to set DCD, ret code %d\n", ret);
	}

	ret = uart_line_ctrl_set(dev, LINE_CTRL_DSR, 1);
	if (ret) {
		printf("Failed to set DSR, ret code %d\n", ret);
	}

	/* Wait 1 sec for the host to do all settings */
	k_busy_wait(1000000);

	ret = uart_line_ctrl_get(dev, LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret) {
		printf("Failed to get baudrate, ret code %d\n", ret);
	} else {
		printf("Baudrate detected: %d\n", baudrate);
	}
}

void main(void)
{
	struct serial_data *dev_data0 = &peers[0];
	struct serial_data *dev_data1 = &peers[1];
	struct device *dev0, *dev1;
	u32_t dtr = 0U;

	dev0 = device_get_binding(CONFIG_CDC_ACM_PORT_NAME_0);
	if (!dev0) {
		printf("CDC ACM device %s not found\n",
		       CONFIG_CDC_ACM_PORT_NAME_0);
		return;
	}

	dev1 = device_get_binding(CONFIG_CDC_ACM_PORT_NAME_1);
	if (!dev1) {
		printf("CDC ACM device %s not found\n",
		       CONFIG_CDC_ACM_PORT_NAME_1);
		return;
	}

	printf("Wait for DTR\n");

	while (1) {
		uart_line_ctrl_get(dev0, LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		}

		k_sleep(100);
	}

	while (1) {
		uart_line_ctrl_get(dev1, LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		}

		k_sleep(100);
	}

	printf("DTR set, start test\n");

	uart_line_set(dev0);
	uart_line_set(dev1);

	dev_data0->dev = dev0;
	dev_data0->peer = dev1;

	dev_data1->dev = dev1;
	dev_data1->peer = dev0;

	uart_irq_callback_user_data_set(dev0, interrupt_handler, dev_data0);
	uart_irq_callback_user_data_set(dev1, interrupt_handler, dev_data1);

	write_data(dev0, banner, strlen(banner));
	write_data(dev1, banner, strlen(banner));

	/* Enable rx interrupts */
	uart_irq_rx_enable(dev0);
	uart_irq_rx_enable(dev1);
}
