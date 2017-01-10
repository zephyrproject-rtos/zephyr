/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_driver_adc
 * @{
 * @defgroup t_adc_basic test_adc_basic_operations
 * @}
 */

extern void test_adc_sample(void);

#include <zephyr.h>
#include <ztest.h>

void test_main(void)
{
	ztest_test_suite(adc_basic_test,
			 ztest_unit_test(test_adc_sample));
	ztest_run_test_suite(adc_basic_test);
}
