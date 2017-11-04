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

#include "test_uart.h"

#ifdef CONFIG_CONSOLE_SHELL
TC_CMD_DEFINE(test_uart_fifo_read)
TC_CMD_DEFINE(test_uart_fifo_fill)
TC_CMD_DEFINE(test_uart_poll_in)
TC_CMD_DEFINE(test_uart_poll_out)
#endif

void test_main(void)
{
#ifdef CONFIG_CONSOLE_SHELL
	/* initialize shell commands */
	static const struct shell_cmd commands[] = {
		TC_CMD_ITEM(test_uart_fifo_read),
		TC_CMD_ITEM(test_uart_fifo_fill),
		TC_CMD_ITEM(test_uart_poll_in),
		TC_CMD_ITEM(test_uart_poll_out),
		{ NULL, NULL }
	};
	SHELL_REGISTER("uart", commands);
	shell_register_default_module("uart");
#else
	ztest_test_suite(uart_basic_test,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
			 ztest_unit_test(test_uart_fifo_fill),
			 ztest_unit_test(test_uart_fifo_read),
#endif
			 ztest_unit_test(test_uart_poll_in),
			 ztest_unit_test(test_uart_poll_out));
	ztest_run_test_suite(uart_basic_test);
#endif
}
