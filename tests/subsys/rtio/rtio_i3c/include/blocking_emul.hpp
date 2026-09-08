/* Copyright (c) 2026 Meta Platforms
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>

/*
 * Mirror of tests/subsys/rtio/rtio_i2c/include/blocking_emul.hpp:
 * override FFF's function-pointer signature with std::function so test
 * lambdas can capture local state and call zassert_* inline. The lambda
 * captures give failure messages pointing into the test function itself
 * rather than a static-global setup helper.
 *
 * Example:
 * @code
 * const uint8_t expected = 0x42;
 * blocking_emul_i3c_xfers_fake.custom_fake =
 *     [&expected](const struct emul *, struct i3c_msg *msgs, uint8_t n) {
 *         zassert_equal(1, n);
 *         zassert_equal(expected, msgs[0].buf[0]);
 *         return 0;
 *     };
 * @endcode
 */
#define CUSTOM_FFF_FUNCTION_TEMPLATE(RETURN, FUNCNAME, ...)                                        \
	std::function<RETURN(__VA_ARGS__)> FUNCNAME

#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/ccc.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/fff.h>

DECLARE_FAKE_VALUE_FUNC(int, blocking_emul_i3c_xfers, const struct emul *, struct i3c_msg *,
			uint8_t);
DECLARE_FAKE_VALUE_FUNC(int, blocking_emul_i3c_do_ccc, const struct emul *,
			struct i3c_ccc_payload *);
