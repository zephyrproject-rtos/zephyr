/*
 * @brief Native TTY UART sample
 *
 * Copyright (c) 2023 Marko Sagadin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include <stdio.h>
#include <string.h>

const struct device *uart0 = DEVICE_DT_GET(DT_NODELABEL(uart0));
const struct device *uart2 = DEVICE_DT_GET(DT_NODELABEL(uart2));

struct uart_config uart_cfg = {
	.baudrate = 9600,
	.parity = UART_CFG_PARITY_NONE,
	.stop_bits = UART_CFG_STOP_BITS_1,
	.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	.data_bits = UART_CFG_DATA_BITS_8,
};

void send_str(const struct device *uart, char *str)
{
	int msg_len = strlen(str);

	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart, str[i]);
	}

	printk("Device %s sent: \"%s\"\n", uart->name, str);
}

void recv_str(const struct device *uart, char *str)
{
	char *head = str;
	char c;

	while (!uart_poll_in(uart, &c)) {
		*head++ = c;
	}
	*head = '\0';

	printk("Device %s received: \"%s\"\n", uart->name, str);
}

int main(void)
{
	int rc;
	char send_buf[64];
	char recv_buf[64];
	int i = 10;

	while (i--) {
		snprintf(send_buf, 64, "Hello from device %s, num %d", uart0->name, i);
		send_str(uart0, send_buf);
		/* Wait some time for the messages to arrive to the second uart. */
		k_sleep(K_MSEC(100));
		recv_str(uart2, recv_buf);

		k_sleep(K_MSEC(1000));
	}

	uart_cfg.baudrate = 9600;
	printk("\nChanging baudrate of both uart devices to %d!\n\n", uart_cfg.baudrate);

	rc = uart_configure(uart0, &uart_cfg);
	if (rc) {
		printk("Could not configure device %s", uart0->name);
	}
	rc = uart_configure(uart2, &uart_cfg);
	if (rc) {
		printk("Could not configure device %s", uart2->name);
	}

	i = 10;
	while (i--) {
		snprintf(send_buf, 64, "Hello from device %s, num %d", uart0->name, i);
		send_str(uart0, send_buf);
		/* Wait some time for the messages to arrive to the second uart. */
		k_sleep(K_MSEC(100));
		recv_str(uart2, recv_buf);

		k_sleep(K_MSEC(1000));
	}

	return 0;
}
