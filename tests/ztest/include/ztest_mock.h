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

#ifndef __ZTEST_MOCK_H__
#define __ZTEST_MOCK_H__

#define ztest_expect_value(func, param, value) \
	_ztest_expect_value(STRINGIFY(func), STRINGIFY(param), \
			    (uintptr_t)(value))

#define ztest_check_expected_value(param) \
	_ztest_check_expected_value(__func__, STRINGIFY(param), \
				    (uintptr_t)(param))

#define ztest_returns_value(func, value) \
	_ztest_returns_value(STRINGIFY(func), (uintptr_t)(value))

#define ztest_get_return_value() \
	_ztest_get_return_value(__func__)

#define ztest_get_return_value_ptr() \
	((void *)_ztest_get_return_value(__func__))

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
