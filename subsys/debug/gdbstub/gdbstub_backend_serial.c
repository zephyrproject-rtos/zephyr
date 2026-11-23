/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/__assert.h>

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

#ifdef CONFIG_GDBSTUB_TRACE
	printk("gdbstub_serial:%s enter\n", __func__);
#endif

	if (uart_dev == NULL) {
		uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_gdbstub_uart));

		__ASSERT(device_is_ready(uart_dev), "uart device is not ready");

		ret = uart_configure(uart_dev, &uart_cfg);
		__ASSERT(ret == 0, "Could not configure uart device");
	}

#ifdef CONFIG_GDBSTUB_TRACE
	printk("gdbstub_serial:%s exit\n", __func__);
#endif
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
