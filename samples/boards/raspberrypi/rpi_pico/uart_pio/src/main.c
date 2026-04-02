/*
 * Copyright (c) 2023 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

int main(void)
{
	char data;
	const struct device *uart0 = DEVICE_DT_GET(DT_NODELABEL(pio1_uart0));
	const struct device *uart1 = DEVICE_DT_GET(DT_NODELABEL(pio1_uart1));

	if (!device_is_ready(uart0)) {
		return 0;
	}

	if (!device_is_ready(uart1)) {
		return 0;
	}

	while (1) {
		if (!uart_poll_in(uart0, &data)) {
			uart_poll_out(uart0, data);
		}

		if (!uart_poll_in(uart1, &data)) {
			uart_poll_out(uart1, data);
		}
	}

	return 0;
}
