/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_UTIL_H_
#define MOCKS_UTIL_H_

#include <zephyr/types.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/ztest.h>

#define CHECK_EMPTY(_x) UTIL_BOOL(IS_EMPTY(_x))
#define COND_CODE_EMPTY(_x, _if_any_code, _else_code)                                              \
	COND_CODE_1(CHECK_EMPTY(_x), _if_any_code, _else_code)
#define IF_EMPTY(_a, _code) COND_CODE_EMPTY(_a, _code, ())
#define IF_NOT_EMPTY(_a, _code) COND_CODE_EMPTY(_a, (), _code)

#define expect_data(_func_name, _arg_name, _exp_data, _data, _len)                                 \
	IF_NOT_EMPTY(_exp_data, (expect_data_equal(_func_name, _arg_name, _exp_data, _data, _len);))

#define zexpect_call_count(_func_name, _expected, _actual)                                         \
	zexpect_equal(_expected, _actual, "'%s()' was called %u times, expected %u times",         \
		      _func_name, _actual, _expected)

static inline void expect_data_equal(const char *func_name, const char *arg_name,
				     const uint8_t *expect, const uint8_t *data, size_t len)
{
	for (size_t i = 0U; i < len; i++) {
		zexpect_equal(expect[i], data[i],
			      "'%s()' was called with incorrect %s[%zu]=0x%02x != 0x%02x value",
			      func_name, arg_name, i, data[i], expect[i]);
	}
}

#endif /* MOCKS_UTIL_H_ */
