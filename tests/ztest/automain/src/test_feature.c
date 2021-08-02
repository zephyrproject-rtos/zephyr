/*
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

/**
 * @brief A stub unit test.
 *
 * This is a mock unit test that is expected to run. Note that it is not added directly to a test
 * suite and run via ztest_run_test_suite(). It is instead registered below using
 * ztest_register_test_suite and will be run by the automatically generated test_main() function.
 */
static void test_stub(void)
{
}

ztest_register_test_suite(test_suite, NULL,
			  ztest_unit_test(test_stub));
