/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addgroup t_thread_contex
 * @{
 * @defgroup t_thread_contex test_thread
 * @brief TestPurpose: verify thread context
 * @details Check whether variable value per-thread and saved during
 *          context switch
 * - API coverage
 *   - k_thread_create
 * @}
 */
#include <ztest.h>

extern void test_critical(void);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(kernel_critical_test,
		ztest_unit_test(test_critical));

	ztest_run_test_suite(kernel_critical_test);
}
