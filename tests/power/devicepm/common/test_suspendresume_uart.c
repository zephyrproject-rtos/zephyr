/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <ztest.h>
#include <device.h>
#include <uart.h>

void test_uart_state(int state)
{
	/*uart console has been suspended as system device*/
	/*uart console has been resumed as system device*/
	return;
}

/*todo: leverage basic tests from uart test set*/
static const char *banner1 = "hello from uart\n";
static volatile bool data_transmitted;
static void interrupt_handler(struct device *dev)
{
	uart_irq_update(dev);

	if (uart_irq_tx_ready(dev)) {
		data_transmitted = true;
	}
}
void test_uart_func(void)
{
	struct device *dev = device_get_binding(
		CONFIG_UART_CONSOLE_ON_DEV_NAME);

	/**TESTPOINT: uart poll out*/
	for (int i = 0; i < strlen(banner1); i++) {
		uart_poll_out(dev, banner1[i]);
	}

	/**TESTPOINT: uart tx*/
	uart_irq_callback_set(dev, interrupt_handler);
	uart_irq_tx_enable(dev);
	for (int i = 0; i < strlen(banner1); i++) {
		data_transmitted = false;
		while (uart_fifo_fill(dev, &banner1[i], 1) == 0)
			;
		while (data_transmitted == false)
			;
	}
	uart_irq_tx_disable(dev);
}

int wait_for_test_start(void)
{
	unsigned char recv_char;
	struct device *uart_dev = NULL;
	int i = 0;
	int cmd_start = 0;
	unsigned char cmd[12];

	uart_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	if (!uart_dev) {
		TC_PRINT("Cannot get UART device\n");
		return TC_FAIL;
	}

	memset(cmd, '\0', sizeof(cmd));
	TC_PRINT("Please send test_start to uart to start the sleep test\n");

	/* Verify uart_poll_in() */
	while (1) {
		while (uart_poll_in(uart_dev, &recv_char) < 0)
			;

		if (recv_char == 't') {
			cmd_start = 1;
		}

		if ((recv_char == '\n') || (recv_char == '\r')) {
			break;
		}

		if (cmd_start) {
			cmd[i] = recv_char;
			i++;
		}
	}

	TC_PRINT("%s\n", cmd);
	if (!strcmp(cmd, "test_start"))
		return TC_PASS;

	return TC_FAIL;
}
