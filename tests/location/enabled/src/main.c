/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>

extern void test_location_api_register(void);
extern void test_location_api_location_providers_get(void);
extern void test_location_api_position_get(void);
extern void test_location_api_bearing_get(void);
extern void test_location_api_speed_get(void);
extern void test_location_api_altitude_get(void);

extern void test_location_api_location_providers_get_user(void);
extern void test_location_api_position_get_user(void);
extern void test_location_api_bearing_get_user(void);
extern void test_location_api_speed_get_user(void);
extern void test_location_api_altitude_get_user(void);

extern void test_location_api_event_handler_register(void);
extern void test_location_api_raise_event(void);

void test_main(void)
{
	ztest_test_suite(location_test_supervisor,
			 ztest_unit_test(test_location_api_register),
			 ztest_unit_test(test_location_api_location_providers_get),
			 ztest_unit_test(test_location_api_position_get),
			 ztest_unit_test(test_location_api_bearing_get),
			 ztest_unit_test(test_location_api_speed_get),
			 ztest_unit_test(test_location_api_altitude_get));

	ztest_test_suite(location_test_user,
			 ztest_user_unit_test(test_location_api_location_providers_get_user),
			 ztest_user_unit_test(test_location_api_position_get_user),
			 ztest_user_unit_test(test_location_api_bearing_get_user),
			 ztest_user_unit_test(test_location_api_speed_get_user),
			 ztest_user_unit_test(test_location_api_altitude_get_user));

	ztest_test_suite(location_test_supervisor_event,
			 ztest_unit_test(test_location_api_event_handler_register),
			 ztest_unit_test(test_location_api_raise_event));

	ztest_run_test_suite(location_test_supervisor);
	ztest_run_test_suite(location_test_user);
	ztest_run_test_suite(location_test_supervisor_event);
}
