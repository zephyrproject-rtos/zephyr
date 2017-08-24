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
 *	context switch
 * - API coverage
 *   - k_thread_create
 * @}
 */
#include <ztest.h>
extern void test_thread_context(void);
/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_context_errno,
			ztest_unit_test(test_thread_context));
	ztest_run_test_suite(test_context_errno);
}
