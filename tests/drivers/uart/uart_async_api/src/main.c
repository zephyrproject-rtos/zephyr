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
	init_test();

#ifdef CONFIG_USERSPACE
	set_permissions();
#endif

	ztest_test_suite(uart_async_test,
			 ztest_unit_test(test_single_read_setup),
			 ztest_user_unit_test(test_single_read),
			 ztest_unit_test(test_multiple_rx_enable_setup),
			 ztest_user_unit_test(test_multiple_rx_enable),
			 ztest_unit_test(test_chained_read_setup),
			 ztest_user_unit_test(test_chained_read),
			 ztest_unit_test(test_double_buffer_setup),
			 ztest_user_unit_test(test_double_buffer),
			 ztest_unit_test(test_read_abort_setup),
			 ztest_user_unit_test(test_read_abort),
			 ztest_unit_test(test_chained_write_setup),
			 ztest_user_unit_test(test_chained_write),
			 ztest_unit_test(test_long_buffers_setup),
			 ztest_user_unit_test(test_long_buffers),
			 ztest_unit_test(test_write_abort_setup),
			 ztest_user_unit_test(test_write_abort),
			 ztest_unit_test(test_forever_timeout_setup),
			 ztest_user_unit_test(test_forever_timeout));
	ztest_run_test_suite(uart_async_test);
}
