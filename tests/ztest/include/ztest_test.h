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

#ifndef __ZTEST_TEST_H__
#define __ZTEST_TEST_H__

struct unit_test {
	const char *name;
	void (*test)(void);
	void (*setup)(void);
	void (*teardown)(void);
};

void _ztest_run_test_suite(const char *name, struct unit_test *suite);

void ztest_test_fail(void);

static inline void unit_test_noop(void)
{
}

#define ztest_unit_test_setup_teardown(fn, setup, teardown) { \
		STRINGIFY(fn), fn, setup, teardown \
}

#define ztest_unit_test(fn) \
	ztest_unit_test_setup_teardown(fn, unit_test_noop, unit_test_noop)

#define ztest_test_suite(name, ...) \
	struct unit_test _##name[] = { \
		__VA_ARGS__, { 0 } \
	}
#define ztest_run_test_suite(suite) \
	_ztest_run_test_suite(#suite, _##suite)

#endif /* __ZTEST_ASSERT_H__ */
