/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_driver_wdt
 * @{
 * @defgroup t_wdt_basic test_wdt_basic_operations
 * @}
 */

#include <zephyr.h>
#include <ztest.h>

extern void test_wdt_int_reset_26(void);
extern void test_wdt_reset_26(void);

void test_main(void)
{
	ztest_test_suite(wdt_basic_test,
			 ztest_unit_test(test_wdt_int_reset_26),
			 ztest_unit_test(test_wdt_reset_26));
	ztest_run_test_suite(wdt_basic_test);
}
