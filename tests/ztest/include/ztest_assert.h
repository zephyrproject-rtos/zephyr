
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

#ifndef __ZTEST_ASSERT_H__
#define __ZTEST_ASSERT_H__

#include <ztest.h>

#ifdef assert
#undef assert
#endif

void ztest_test_fail(void);

#if CONFIG_ZTEST_ASSERT_VERBOSE == 0

static inline void _assert_(int cond, const char *file, int line)
{
	if (!(cond)) {
		PRINT("\n    Assertion failed at %s:%d\n",
		      file, line);
		ztest_test_fail();
	}
}

#define _assert(cond, msg, default_msg, file, line, func) \
	_assert_(cond, file, line)

#else /* CONFIG_ZTEST_ASSERT_VERBOSE != 0 */

static inline void _assert(int cond, const char *msg, const char *default_msg,
			   const char *file, int line, const char *func)
{
	if (!(cond)) {
		PRINT("\n    Assertion failed at %s:%d: %s: %s %s\n",
		      file, line, func, msg, default_msg);
		ztest_test_fail();
	}
#if CONFIG_ZTEST_ASSERT_VERBOSE == 2
	else {
		PRINT("\n   Assertion succeeded at %s:%d (%s)\n",
		      file, line, func);
	}
#endif
}

#endif /* CONFIG_ZTEST_ASSERT_VERBOSE */

#define assert(cond, msg, default_msg) \
	_assert(cond, msg, msg ? ("(" default_msg ")") : (default_msg), \
		__FILE__, __LINE__, __func__)

#define assert_unreachable(msg) assert(0, msg, "Reached unreachable code")

#define assert_true(cond, msg) assert(cond, msg, #cond " is false")
#define assert_false(cond, msg) assert(!(cond), msg, #cond " is true")

#define assert_is_null(ptr, msg) assert((ptr) == NULL, msg, #ptr " is not NULL")
#define assert_not_null(ptr, msg) assert((ptr) != NULL, msg, #ptr " is NULL")

#define assert_equal(a, b, msg) assert((a) == (b), msg, #a " not equal to " #b)
#define assert_equal_ptr(a, b, msg) \
	assert((void *)(a) == (void *)(b), msg, #a " not equal to  " #b)

#endif /* __ZTEST_ASSERT_H__ */
