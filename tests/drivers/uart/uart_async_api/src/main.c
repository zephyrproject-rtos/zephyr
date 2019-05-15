/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_driver_uart
 * @{
 * @defgroup t_uart_async test_uart_async
 * @}
 */

#include "test_uart.h"

void test_main(void)
{
	ztest_test_suite(uart_async_test,
			 ztest_unit_test(test_single_read),
			 ztest_unit_test(test_chained_read),
			 ztest_unit_test(test_double_buffer),
			 ztest_unit_test(test_read_abort),
			 ztest_unit_test(test_chained_write),
			 ztest_unit_test(test_long_buffers),
			 ztest_unit_test(test_write_abort));
	ztest_run_test_suite(uart_async_test);
}
