/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_driver_uart
 * @{
 * @defgroup t_uart_basic test_uart_basic_operations
 * @}
 */

#include <zephyr/usb/usb_device.h>
#include "test_uart.h"

#ifdef CONFIG_SHELL
TC_CMD_DEFINE(test_uart_configure)
TC_CMD_DEFINE(test_uart_config_get)
TC_CMD_DEFINE(test_uart_poll_out)
TC_CMD_DEFINE(test_uart_poll_in)
#if CONFIG_UART_INTERRUPT_DRIVEN
TC_CMD_DEFINE(test_uart_fifo_read)
TC_CMD_DEFINE(test_uart_fifo_fill)
TC_CMD_DEFINE(test_uart_pending)
#endif

SHELL_CMD_REGISTER(test_uart_configure, NULL, NULL,
			TC_CMD_ITEM(test_uart_configure));
SHELL_CMD_REGISTER(test_uart_config_get, NULL, NULL,
			TC_CMD_ITEM(test_uart_config_get));
SHELL_CMD_REGISTER(test_uart_poll_in, NULL, NULL,
			TC_CMD_ITEM(test_uart_poll_in));
SHELL_CMD_REGISTER(test_uart_poll_out, NULL, NULL,
			TC_CMD_ITEM(test_uart_poll_out));
#if CONFIG_UART_INTERRUPT_DRIVEN
SHELL_CMD_REGISTER(test_uart_fifo_read, NULL, NULL,
			TC_CMD_ITEM(test_uart_fifo_read));
SHELL_CMD_REGISTER(test_uart_fifo_fill, NULL, NULL,
			TC_CMD_ITEM(test_uart_fifo_fill));
SHELL_CMD_REGISTER(test_uart_pending, NULL, NULL,
			TC_CMD_ITEM(test_uart_pending));
#endif


#endif

void *uart_basic_setup(void)
{
#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart)
	const struct device *dev;
	uint32_t dtr = 0;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	if (!device_is_ready(dev) || usb_enable(NULL)) {
		return NULL;
	}

	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}
#endif
	return NULL;
}

#ifndef CONFIG_SHELL
ZTEST_SUITE(uart_basic_api, NULL, uart_basic_setup, NULL, NULL, NULL);

/* The UART pending test should be test finally. */
ZTEST_SUITE(uart_basic_api_pending, NULL, uart_basic_setup, NULL, NULL, NULL);
#endif
