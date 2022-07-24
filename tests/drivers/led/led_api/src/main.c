/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>

#include "test_led_api.h"

void test_main(void)
{
	ztest_test_suite(led_user_test,
			 ztest_user_unit_test(test_led_setup),
			 ztest_user_unit_test(test_led_get_info),
			 ztest_user_unit_test(test_led_on),
			 ztest_user_unit_test(test_led_off),
			 ztest_user_unit_test(test_led_set_color),
			 ztest_user_unit_test(test_led_set_brightness));
	ztest_run_test_suite(led_user_test);
}
