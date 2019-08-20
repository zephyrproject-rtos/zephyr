/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Tests for the FIFO kernel object
 *
 * Verify zephyr fifo apis under different context
 *
 * - API coverage
 *   -# k_fifo_init K_FIFO_DEFINE
 *   -# k_fifo_put k_fifo_put_list k_fifo_put_slist
 *   -# k_fifo_get *
 *
 * @defgroup kernel_fifo_tests FIFOs
 * @ingroup all_tests
 * @{
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
	ztest_test_suite(fifo_api,
			 ztest_1cpu_unit_test(test_fifo_thread2thread),
			 ztest_unit_test(test_fifo_thread2isr),
			 ztest_unit_test(test_fifo_isr2thread),
			 ztest_unit_test(test_fifo_get_fail),
			 ztest_1cpu_unit_test(test_fifo_loop),
			 ztest_1cpu_unit_test(test_fifo_cancel_wait),
			 ztest_unit_test(test_fifo_is_empty_thread),
			 ztest_unit_test(test_fifo_is_empty_isr));
	ztest_run_test_suite(fifo_api);
}
