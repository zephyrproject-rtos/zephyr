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

#ifdef CONFIG_SHELL
TC_CMD_DEFINE(test_uart_configure)
TC_CMD_DEFINE(test_uart_config_get)
TC_CMD_DEFINE(test_uart_fifo_read)
TC_CMD_DEFINE(test_uart_fifo_fill)
TC_CMD_DEFINE(test_uart_poll_in)
TC_CMD_DEFINE(test_uart_poll_out)
TC_CMD_DEFINE(test_uart_pending)

SHELL_CMD_REGISTER(test_uart_configure, NULL, NULL,
			TC_CMD_ITEM(test_uart_configure));
SHELL_CMD_REGISTER(test_uart_config_get, NULL, NULL,
			TC_CMD_ITEM(test_uart_config_get));
SHELL_CMD_REGISTER(test_uart_fifo_read, NULL, NULL,
			TC_CMD_ITEM(test_uart_fifo_read));
SHELL_CMD_REGISTER(test_uart_fifo_fill, NULL, NULL,
			TC_CMD_ITEM(test_uart_fifo_fill));
SHELL_CMD_REGISTER(test_uart_poll_in, NULL, NULL,
			TC_CMD_ITEM(test_uart_poll_in));
SHELL_CMD_REGISTER(test_uart_poll_out, NULL, NULL,
			TC_CMD_ITEM(test_uart_poll_out));
SHELL_CMD_REGISTER(test_uart_pending, NULL, NULL,
			TC_CMD_ITEM(test_uart_pending));
#endif

#ifndef CONFIG_UART_INTERRUPT_DRIVEN
void test_uart_fifo_fill(void)
{
	ztest_test_skip();
}

void test_uart_fifo_read(void)
{
	ztest_test_skip();
}

void test_uart_pending(void)
{
	ztest_test_skip();
}
#endif

void test_main(void)
{
#ifndef CONFIG_SHELL
	ztest_test_suite(uart_basic_test,
			 ztest_unit_test(test_uart_configure),
			 ztest_unit_test(test_uart_config_get),
			 ztest_unit_test(test_uart_fifo_fill),
			 ztest_unit_test(test_uart_fifo_read),
			 ztest_unit_test(test_uart_poll_in),
			 ztest_unit_test(test_uart_poll_out),
			 ztest_unit_test(test_uart_pending));
	ztest_run_test_suite(uart_basic_test);
#endif
}
