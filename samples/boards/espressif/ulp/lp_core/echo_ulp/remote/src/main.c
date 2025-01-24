/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#define UART_DEVICE_NODE DT_NODELABEL(lp_uart)

int main(void)
{
	const struct device *uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);
	char c;
	int ret = 0;

	printf("UART echo example started. Type something...\n");

	while (1) {
		ret = uart_poll_in(uart_dev, &c);
		if (ret == 0) {
			/* Echo character back out */
			uart_poll_out(uart_dev, c);
			if (c == '\r') {
				uart_poll_out(uart_dev, '\n');
			}
		}
	}

	return 0;
}
