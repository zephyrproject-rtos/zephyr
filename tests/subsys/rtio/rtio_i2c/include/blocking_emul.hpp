/* Copyright (c) 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>

#define CUSTOM_FFF_FUNCTION_TEMPLATE(RETURN, FUNCNAME, ...)                                        \
	std::function<RETURN(__VA_ARGS__)> FUNCNAME

#include <zephyr/fff.h>

DECLARE_FAKE_VALUE_FUNC(int, blocking_emul_i2c_transfer, const struct emul *, struct i2c_msg *, int,
			int);
