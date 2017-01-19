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

#ifdef INT_RESET
extern void test_wdt_int_reset_26(void);
#else
extern void test_wdt_reset_26(void);
#endif

void test_main(void)
{
	ztest_test_suite(wdt_basic_test,
#ifdef INT_RESET
			 ztest_unit_test(test_wdt_int_reset_26));
#else
			 ztest_unit_test(test_wdt_reset_26));
#endif
	ztest_run_test_suite(wdt_basic_test);
}
