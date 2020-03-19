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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ztest_test_fail(void);
#if CONFIG_ZTEST_ASSERT_VERBOSE == 0

static inline void z_ztest_(bool cond, const char *file, int line)
{
	if (cond == false) {
		PRINT("\n    Assertion failed at %s:%d\n",
		      file, line);
		ztest_test_fail();
	}
}

#define z_ztest(cond, default_msg, file, line, func, msg, ...)	\
	z_ztest_(cond, file, line)

#else /* CONFIG_ZTEST_ASSERT_VERBOSE != 0 */

static inline void z_ztest(bool cond,
			    const char *default_msg,
			    const char *file,
			    int line, const char *func,
			    const char *msg, ...)
{
	if (cond == false) {
		va_list vargs;

		va_start(vargs, msg);
		PRINT("\n    Assertion failed at %s:%d: %s: %s\n",
		      file, line, func, default_msg);
		vprintk(msg, vargs);
		printk("\n");
		va_end(vargs);
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
 * instead use ztest_{condition} macros below.
 *
 * @param cond Condition to check
 * @param msg Optional, can be NULL. Message to print if @a cond is false.
 * @param default_msg Message to print if @a cond is false
 */

#define ztest(cond, default_msg, msg, ...)			    \
	z_ztest(cond, msg ? ("(" default_msg ")") : (default_msg), \
		 __FILE__, __LINE__, __func__, msg ? msg : "", ##__VA_ARGS__)

/**
 * @brief Assert that this function call won't be reached
 * @param msg Optional message to print if the assertion fails
 */
#define ztest_unreachable(msg, ...) ztest(0, "Reached unreachable code", \
					      msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a cond is true
 * @param cond Condition to check
 * @param msg Optional message to print if the assertion fails
 */
#define ztest_true(cond, msg, ...) ztest(cond, #cond " is false", \
					     msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a cond is false
 * @param cond Condition to check
 * @param msg Optional message to print if the assertion fails
 */
#define ztest_false(cond, msg, ...) ztest(!(cond), #cond " is true", \
					      msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a ptr is NULL
 * @param ptr Pointer to compare
 * @param msg Optional message to print if the assertion fails
 */
#define ztest_is_null(ptr, msg, ...) ztest((ptr) == NULL,	    \
					       #ptr " is not NULL", \
					       msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a ptr is not NULL
 * @param ptr Pointer to compare
 * @param msg Optional message to print if the assertion fails
 */
#define ztest_not_null(ptr, msg, ...) ztest((ptr) != NULL,	      \
						#ptr " is NULL", msg, \
						##__VA_ARGS__)

/**
 * @brief Assert that @a a equals @a b
 *
 * @a a and @a b won't be converted and will be compared directly.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assertion fails
 */
#define ztest_equal(a, b, msg, ...) ztest((a) == (b),	      \
					      #a " not equal to " #b, \
					      msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a a does not equal @a b
 *
 * @a a and @a b won't be converted and will be compared directly.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assertion fails
 */
#define ztest_not_equal(a, b, msg, ...) ztest((a) != (b),	      \
						  #a " equal to " #b, \
						  msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a a equals @a b
 *
 * @a a and @a b will be converted to `void *` before comparing.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assertion fails
 */
#define ztest_equal_ptr(a, b, msg, ...)			    \
	ztest((void *)(a) == (void *)(b), #a " not equal to " #b, \
		msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a a is within @a b with delta @a d
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param d Delta
 * @param msg Optional message to print if the assertion fails
 */
#define ztest_within(a, b, d, msg, ...)			     \
	ztest(((a) > ((b) - (d))) && ((a) < ((b) + (d))),	     \
		#a " not within " #b " +/- " #d,		     \
		msg, ##__VA_ARGS__)

/**
 * @brief Assert that 2 memory buffers have the same contents
 *
 * This macro calls the final memory comparison assertion macro.
 * Using double expansion allows providing some arguments by macros that
 * would expand to more than one values (ANSI-C99 defines that all the macro
 * arguments have to be expanded before macro call).
 *
 * @param ... Arguments, see @ref ztest_mem_equal__
 *            for real arguments accepted.
 */
#define ztest_mem_equal(...) \
	ztest_mem_equal__(__VA_ARGS__)

/**
 * @brief Internal assert that 2 memory buffers have the same contents
 *
 * @note This is internal macro, to be used as a second expansion.
 *       See @ref ztest_mem_equal.
 *
 * @param buf Buffer to compare
 * @param exp Buffer with expected contents
 * @param size Size of buffers
 * @param msg Optional message to print if the assertion fails
 */
#define ztest_mem_equal__(buf, exp, size, msg, ...)                    \
	ztest(memcmp(buf, exp, size) == 0, #buf " not equal to " #exp, \
	msg, ##__VA_ARGS__)

/**
 * @}
 */

/*
 * Legacy macros, do not use, remove for 2.4
 */

#define zassert_mem_equal	ztest_mem_equal		DEPRECATED_MACRO
#define zassert_within		ztest_within		DEPRECATED_MACRO
#define zassert_equal_ptr	ztest_equal_ptr		DEPRECATED_MACRO
#define zassert_not_equal	ztest_not_equal		DEPRECATED_MACRO
#define zassert_equal		ztest_equal		DEPRECATED_MACRO
#define zassert_not_null	ztest_not_null		DEPRECATED_MACRO
#define zassert_is_null		ztest_is_null		DEPRECATED_MACRO
#define zassert_false		ztest_false		DEPRECATED_MACRO
#define zassert_true		ztest_true		DEPRECATED_MACRO
#define zassert_unreachable	ztest_unreachable	DEPRECATED_MACRO
#define zassert			ztest			DEPRECATED_MACRO

#ifdef __cplusplus
}
#endif

#endif /* __ZTEST_ASSERT_H__ */
