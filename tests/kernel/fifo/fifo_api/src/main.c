/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_fifo
 * @{
 * @defgroup t_fifo_api test_fifo_api
 * @}
 */

#include <ztest.h>
extern void test_fifo_thread2thread(void);
extern void test_fifo_thread2isr(void);
extern void test_fifo_isr2thread(void);
extern void test_fifo_get_fail(void);
extern void test_fifo_loop(void);
extern void test_fifo_cancel_wait(void);
extern void test_fifo_is_empty_thread(void);
extern void test_fifo_is_empty_isr(void);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_fifo_api,
			 ztest_unit_test(test_fifo_thread2thread),
			 ztest_unit_test(test_fifo_thread2isr),
			 ztest_unit_test(test_fifo_isr2thread),
			 ztest_unit_test(test_fifo_get_fail),
			 ztest_unit_test(test_fifo_loop),
			 ztest_unit_test(test_fifo_cancel_wait),
			 ztest_unit_test(test_fifo_is_empty_thread),
			 ztest_unit_test(test_fifo_is_empty_isr));
	ztest_run_test_suite(test_fifo_api);
}
