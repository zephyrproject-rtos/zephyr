/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 *
 * @brief Ztest mocking support
 */

#ifndef __ZTEST_MOCK_H__
#define __ZTEST_MOCK_H__

/**
 * @defgroup ztest_mock Ztest mocking support
 * @ingroup ztest
 *
 * This module provides simple mocking functions for unit testing. These
 * need CONFIG_ZTEST_MOCKING=y.
 *
 * @{
 */

/**
 * @brief Tell function @a func to expect the value @a value for @a param
 *
 * When using ztest_check_expected_value(), tell that the value of @a param
 * should be @a value. The value will internally be stored as an `uintptr_t`.
 *
 * @param func Function in question
 * @param param Parameter for which the value should be set
 * @param value Value for @a param
 */
#define ztest_expect_value(func, param, value) \
	_ztest_expect_value(STRINGIFY(func), STRINGIFY(param), \
			    (uintptr_t)(value))

/**
 * @brief If @a param doesn't match the value set by ztest_expect_value(),
 * fail the test
 *
 * This will first check that does @a param have a value to be expected, and
 * then checks whether the value of the parameter is equal to the expected
 * value. If either of these checks fail, the current test will fail. This
 * must be called from the called function.
 *
 * @param param Parameter to check
 */
#define ztest_check_expected_value(param) \
	_ztest_check_expected_value(__func__, STRINGIFY(param), \
				    (uintptr_t)(param))

/**
 * @brief Tell @a func that it should return @a value
 *
 * @param func Function that should return @a value
 * @param value Value to return from @a func
 */
#define ztest_returns_value(func, value) \
	_ztest_returns_value(STRINGIFY(func), (uintptr_t)(value))

/**
 * @brief Get the return value for current function
 *
 * The return value must have been set previously with ztest_returns_value().
 * If no return value exists, the current test will fail.
 *
 * @returns The value the current function should return
 */
#define ztest_get_return_value() \
	_ztest_get_return_value(__func__)

/**
 * @brief Get the return value as a pointer for current function
 *
 * The return value must have been set previously with ztest_returns_value().
 * If no return value exists, the current test will fail.
 *
 * @returns The value the current function should return as a `void *`
 */
#define ztest_get_return_value_ptr() \
	((void *)_ztest_get_return_value(__func__))

/**
 * @}
 */

#ifdef CONFIG_ZTEST_MOCKING

#include <stdint.h>

void _init_mock(void);
int _cleanup_mock(void);

void _ztest_expect_value(const char *fn, const char *name, uintptr_t value);
void _ztest_check_expected_value(const char *fn, const char *param,
				 uintptr_t value);

void _ztest_returns_value(const char *fn, uintptr_t value);
uintptr_t _ztest_get_return_value(const char *fn);

#else /* !CONFIG_ZTEST_MOCKING */

#define _init_mock()
#define _cleanup_mock() 0

#endif  /* CONFIG_ZTEST_MOCKING */

#endif  /* __ZTEST_H__ */
