/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_driver_rtc
 * @{
 * @defgroup t_rtc_basic_api test_rtc_basic_api
 * @}
 */

#include "test_rtc.h"

void test_main(void)
{
	ztest_test_suite(rtc_basic_test,
			 ztest_unit_test(test_rtc_alarm),
			 ztest_unit_test(test_rtc_calendar));
	ztest_run_test_suite(rtc_basic_test);
}
