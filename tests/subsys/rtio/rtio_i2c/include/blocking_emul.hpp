/* Copyright (c) 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>

/*
 * We need to override fff's function signature away from a standard function pointer.
 * By using std::function<> we're able to leverage capture variables since additional storage must
 * be allocated for them. In the tests, we will include asserts in the lambda functions. This
 * provides the ability to assert inside the lambda and get errors that map to a line within the
 * test function. If we use plain function pointers, we have to move the expected value outside
 * of the test function and make it a static global of the compilational unit.
 *
 * Example:
 * @code
 * const uint8_t expected_value = 40;
 * my_function_fake.custom_fake = [&expected_value](uint8_t value) {
 *   zassert_equal(expected_value, value);
 * };
 * my_function(expected_value);
 * @endcode
 */
#define CUSTOM_FFF_FUNCTION_TEMPLATE(RETURN, FUNCNAME, ...)                                        \
	std::function<RETURN(__VA_ARGS__)> FUNCNAME

#include <zephyr/fff.h>

DECLARE_FAKE_VALUE_FUNC(int, blocking_emul_i2c_transfer, const struct emul *, struct i2c_msg *, int,
			int);
