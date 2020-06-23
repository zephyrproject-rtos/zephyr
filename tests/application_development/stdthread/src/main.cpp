/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

BUILD_ASSERT(__cplusplus == 201703);

extern void test_this_thread_get_id(void);
extern void test_this_thread_yield(void);
extern void test_this_thread_sleep_until(void);
extern void test_this_thread_sleep_for(void);

extern void test_thread_get_id(void);
extern void test_thread_native_handle(void);
extern void test_thread_hardware_concurrency(void);
extern void test_thread_join(void);
extern void test_thread_detach(void);
extern void test_thread_swap(void);

void test_main(void)
{
	TC_PRINT("version %u\n", (uint32_t)__cplusplus);
	ztest_test_suite(libcxx_tests, ztest_unit_test(test_this_thread_get_id),
			 ztest_unit_test(test_this_thread_yield),
			 ztest_unit_test(test_this_thread_sleep_until),
			 ztest_unit_test(test_this_thread_sleep_for),

			 ztest_unit_test(test_thread_get_id),
			 ztest_unit_test(test_thread_native_handle),
			 ztest_unit_test(test_thread_hardware_concurrency),
			 ztest_unit_test(test_thread_join),
			 ztest_unit_test(test_thread_detach),
			 ztest_unit_test(test_thread_swap));

	ztest_run_test_suite(libcxx_tests);
}
