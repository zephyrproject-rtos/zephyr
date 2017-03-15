/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_driver_aio
 * @{
 * @defgroup t_aio_basic_api test_aio_basic_api
 * @}
 *
 * Setup: loop PIN_OUT with PIN_IN on target board
 *
 * quark_se_c1000_devboard - x86
 * ------------------------
 * 1. PIN_OUT is GPIO_15
 * 2. PIN_IN is AIN_10
 *
 * quark_se_c1000_ss_evboard - arc
 * ------------------------
 * 1. PIN_OUT is GPIO_SS_3
 * 2. PIN_IN is AIN_10
 */

#include <ztest.h>

extern void test_aio_callback_rise(void);
extern void test_aio_callback_fall(void);
extern void test_aio_callback_rise_disable(void);

void test_main(void)
{
	ztest_test_suite(aio_basic_api_test,
			 ztest_unit_test(test_aio_callback_rise),
			 ztest_unit_test(test_aio_callback_fall),
			 ztest_unit_test(test_aio_callback_rise_disable));
	ztest_run_test_suite(aio_basic_api_test);
}
