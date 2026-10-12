/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#define SINGLE_WIRE_UART1_NODE DT_ALIAS(single_line_uart1)
#define SINGLE_WIRE_UART2_NODE DT_ALIAS(single_line_uart2)

static const struct device *const single_wire_uart1 = DEVICE_DT_GET(SINGLE_WIRE_UART1_NODE);
static const struct device *const single_wire_uart2 = DEVICE_DT_GET(SINGLE_WIRE_UART2_NODE);

int main(void)
{
	unsigned char received;

	if (!device_is_ready(single_wire_uart1) || !device_is_ready(single_wire_uart2)) {
		return 0;
	}

	while (true) {
		uart_poll_out(single_wire_uart1, 'a');

		/* Allow the byte to be transmitted before reading it. */
		k_sleep(K_MSEC(50));
		(void)uart_poll_in(single_wire_uart2, &received);

		uart_poll_out(single_wire_uart2, 'b');

		/* Allow the byte to be transmitted before reading it. */
		k_sleep(K_MSEC(50));
		(void)uart_poll_in(single_wire_uart1, &received);

		k_sleep(K_SECONDS(2));
	}

	return 0;
}
