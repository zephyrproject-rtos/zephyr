/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_wait_for(void);

/**
 * @defgroup sys_util_tests Sys Util Tests
 * @ingroup all_tests
 * @{
 * @}
 */

void test_main(void)
{
	ztest_test_suite(sys_util, ztest_unit_test(test_wait_for));
	ztest_run_test_suite(sys_util);
}
