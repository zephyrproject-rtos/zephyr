/*
 * Copyright (c) 2026 christian.sander85@gmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>

#define UART_DEVICE_NODE DT_NODELABEL(usart2)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* uart configuration structure */
const struct uart_config uart_cfg = {.baudrate = DT_PROP(UART_DEVICE_NODE, current_speed),
				     .parity = UART_CFG_PARITY_NONE,
				     .stop_bits = UART_CFG_STOP_BITS_1,
				     .data_bits = UART_CFG_DATA_BITS_8,
				     .flow_ctrl = UART_CFG_FLOW_CTRL_NONE};

void uart_cb(const struct device *dev, void *user_data)
{
}

int main(void)
{
	if (!uart_dev) {
		printk("Failed to get UART device");
		return 1;
	}

	if (!device_is_ready(uart_dev)) {
		printk("Device not ready");
		return 1;
	}

	/* uart configuration parameters */
	int err = uart_configure(uart_dev, &uart_cfg);
	if (err != 0) {
		printk("UART configuration failed: %d", err);
		return 1;
	}

	/* Configure uart callback */
	uart_irq_callback_set(uart_dev, uart_cb);

	/* Enable RX irq */
	uart_irq_rx_enable(uart_dev);

	printk("ZLI enabled");

	while (1) {
	}

	return 0;
}
