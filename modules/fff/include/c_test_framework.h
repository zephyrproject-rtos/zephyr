/* Copyright (c) 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MODULES_FFF_TEST_INCLUDE_C_TEST_FRAMEWORK_H_
#define MODULES_FFF_TEST_INCLUDE_C_TEST_FRAMEWORK_H_

#include <ztest.h>
#include <zephyr/sys/printk.h>

void setup(void);
void fff_test_suite(void);

#define PRINTF(FMT, args...)
#define TEST_F(SUITE, NAME) __attribute__((unused)) static void test_##NAME(void)
#define RUN_TEST(SUITE, NAME)                                                                      \
	do {                                                                                       \
		ztest_test_suite(                                                                  \
			SUITE##_##NAME,                                                            \
			ztest_unit_test_setup_teardown(test_##NAME, setup, unit_test_noop));       \
		ztest_run_test_suite(SUITE##_##NAME);                                              \
	} while (0)
#define ASSERT_EQ(A, B) zassert_equal((A), (B), NULL)
#define ASSERT_TRUE(A) zassert_true((A), NULL)

#endif /* MODULES_FFF_TEST_INCLUDE_C_TEST_FRAMEWORK_H_ */
