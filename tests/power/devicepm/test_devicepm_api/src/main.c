/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_device_list_get(void);
extern void test_device_busy_set(void);
extern void test_device_busy_check(void);
extern void test_device_any_busy_check(void);
extern void test_device_busy_clear(void);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_devicepm_api,
		ztest_unit_test(test_device_list_get),
		ztest_unit_test(test_device_busy_set),
		ztest_unit_test(test_device_busy_check),
		ztest_unit_test(test_device_any_busy_check),
		ztest_unit_test(test_device_busy_clear));
	ztest_run_test_suite(test_devicepm_api);
}
