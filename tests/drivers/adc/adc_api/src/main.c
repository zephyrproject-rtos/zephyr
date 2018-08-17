/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
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

extern void test_adc_sample_one_channel(void);
extern void test_adc_sample_two_channels(void);
extern void test_adc_asynchronous_call(void);
extern void test_adc_sample_with_interval(void);
extern void test_adc_repeated_samplings(void);

#include <zephyr.h>
#include <ztest.h>

void test_main(void)
{
	ztest_test_suite(adc_basic_test,
			 ztest_unit_test(test_adc_sample_one_channel),
			 ztest_unit_test(test_adc_sample_two_channels),
			 ztest_unit_test(test_adc_asynchronous_call),
			 ztest_unit_test(test_adc_sample_with_interval),
			 ztest_unit_test(test_adc_repeated_samplings));
	ztest_run_test_suite(adc_basic_test);
}
