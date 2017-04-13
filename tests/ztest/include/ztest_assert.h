
/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Zephyr testing framework assertion macros
 */

#ifndef __ZTEST_ASSERT_H__
#define __ZTEST_ASSERT_H__

#include <ztest.h>

void ztest_test_fail(void);

#if CONFIG_ZTEST_ASSERT_VERBOSE == 0

static inline void _zassert_(int cond, const char *file, int line)
{
	if (!(cond)) {
		PRINT("\n    Assertion failed at %s:%d\n",
		      file, line);
		ztest_test_fail();
	}
}

#define _zassert(cond, msg, default_msg, file, line, func) \
	_zassert_(cond, file, line)

#else /* CONFIG_ZTEST_ASSERT_VERBOSE != 0 */

static inline void _zassert(int cond, const char *msg, const char *default_msg,
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

/**
 * @defgroup ztest_assert Ztest assertion macros
 * @ingroup ztest
 *
 * This module provides assertions when using Ztest.
 *
 * @{
 */

/**
 * @brief Fail the test, if @a cond is false
 *
 * You probably don't need to call this macro directly. You should
 * instead use zassert_{condition} macros below.
 *
 * @param cond Condition to check
 * @param msg Optional, can be NULL. Message to print if @a cond is false.
 * @param default_msg Message to print if @a cond is false
 */

#define zassert(cond, msg, default_msg) \
	_zassert(cond, msg ? msg : "", msg ? ("(" default_msg ")") : (default_msg), \
		__FILE__, __LINE__, __func__)

/**
 * @brief Assert that this function call won't be reached
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_unreachable(msg) zassert(0, msg, "Reached unreachable code")

/**
 * @brief Assert that @a cond is true
 * @param cond Condition to check
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_true(cond, msg) zassert(cond, msg, #cond " is false")

/**
 * @brief Assert that @a cond is false
 * @param cond Condition to check
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_false(cond, msg) zassert(!(cond), msg, #cond " is true")

/**
 * @brief Assert that @a ptr is NULL
 * @param ptr Pointer to compare
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_is_null(ptr, msg) zassert((ptr) == NULL, msg, #ptr " is not NULL")

/**
 * @brief Assert that @a ptr is not NULL
 * @param ptr Pointer to compare
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_not_null(ptr, msg) zassert((ptr) != NULL, msg, #ptr " is NULL")

/**
 * @brief Assert that @a a equals @a b
 *
 * @a a and @a b won't be converted and will be compared directly.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_equal(a, b, msg) zassert((a) == (b), msg, #a " not equal to " #b)

/**
 * @brief Assert that @a a does not equal @a b
 *
 * @a a and @a b won't be converted and will be compared directly.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_not_equal(a, b, msg) zassert((a) != (b), msg, #a " equal to " #b)

/**
 * @brief Assert that @a a equals @a b
 *
 * @a a and @a b will be converted to `void *` before comparing.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_equal_ptr(a, b, msg) \
	zassert((void *)(a) == (void *)(b), msg, #a " not equal to  " #b)

/**
 * @}
 */

#endif /* __ZTEST_ASSERT_H__ */
