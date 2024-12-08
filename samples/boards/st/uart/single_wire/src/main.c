/*
 * Copyright (c) 2021 Jonathan Hahn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>

#define UART_NODE1 DT_ALIAS(single_line_uart1)
#define UART_NODE2 DT_ALIAS(single_line_uart2)

const struct device *const sl_uart1 = DEVICE_DT_GET(UART_NODE1);
const struct device *const sl_uart2 = DEVICE_DT_GET(UART_NODE2);

int main(void)
{
	unsigned char recv;

	if (!device_is_ready(sl_uart1) || !device_is_ready(sl_uart2)) {
		printk("uart devices not ready\n");
		return 0;
	}

	while (true) {

		uart_poll_out(sl_uart1, 'c');

		/* give the uarts some time to get the data through */
		k_sleep(K_MSEC(50));

		int ret = uart_poll_in(sl_uart2, &recv);

		if (ret < 0) {
			printk("Receiving failed. Error: %d", ret);
		} else {
			printk("Received %c\n", recv);
		}

		k_sleep(K_MSEC(2000));
	}
	return 0;
}
