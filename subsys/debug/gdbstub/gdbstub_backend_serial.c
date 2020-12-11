/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/uart.h>
#include <sys/__assert.h>

static const struct device *uart_dev;

int z_gdb_backend_init(void)
{
	int ret = 0;
	static const struct uart_config uart_cfg = {
		.baudrate = 115200,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
	};

	if (uart_dev == NULL) {
		uart_dev = device_get_binding(
			CONFIG_GDBSTUB_SERIAL_BACKEND_NAME);

		__ASSERT(uart_dev != NULL, "Could not get uart device");

		ret = uart_configure(uart_dev, &uart_cfg);
		__ASSERT(ret == 0, "Could not configure uart device");
	}

	return ret;
}

void z_gdb_putchar(unsigned char ch)
{
	uart_poll_out(uart_dev, ch);
}

unsigned char z_gdb_getchar(void)
{
	unsigned char ch;

	while (uart_poll_in(uart_dev, &ch) < 0) {
	}

	return ch;
}
