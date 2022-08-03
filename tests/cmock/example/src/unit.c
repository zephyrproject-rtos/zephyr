/*
 * Copyright (c) 2022 Martin Schröder Consulting
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <unity.h>
#include <mock_kernel.h>

/**
 * this is a unit test so we will include the file directly
 * this is the only place where we do this - it is the optimal solution.
 * #include "../../../../../some/driver/we/are/unit/testing/driver.c"
 **/

void test_example_object_init_should_return_einval_on_invalid_args(void)
{
	/**
	 * Here you can use mocks of any kernel functions
	 * __wrap_k_mutex_init_ExpectAndReturn(&inst.mx, 0);
	 * TEST_ASSERT_EQUAL(0, _some_driver_function(&inst));
	 **/
}
