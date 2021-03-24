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

#ifndef ZEPHYR_TESTSUITE_ZTEST_ASSERT_H_
#define ZEPHYR_TESTSUITE_ZTEST_ASSERT_H_

#include <ztest.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if Z_C_GENERIC && defined(__GNUC__) && !defined(__cplusplus)
/* Printing values requires _Generic and __auto_type support. */
#define ZTEST_ASSERT_PRINT_VALUES 1
#endif

/** @brief Return index of an array that holds format specifiers strings in ztest
 * implementation.
 *
 * @param x variable.
 *
 * @return Index of format specifier for given variable.
 */
#define ZASSERT_FORMAT_SPECIFIER_IDX(x) _Generic((x), \
	char : 0, \
	signed char : 1, \
	unsigned char : 2, \
	signed short : 3, \
	unsigned short : 4, \
	int : 5, \
	unsigned int : 6, \
	long : 7, \
	unsigned long : 8, \
	long long : 9, \
	unsigned long long : 10, \
	float : 11, \
	double : 12, \
	long double : 13, \
	default : \
		14)

const char *ztest_relative_filename(const char *file);
void ztest_test_fail(void);

static inline void z_zassert(bool cond,
			    const char *default_msg,
			    const char *file,
			    int line, const char *func,
			    const char *msg, ...)
{
	if (cond == false) {
		if (CONFIG_ZTEST_ASSERT_VERBOSE == 0) {
			PRINT("\n    Assertion failed at %s:%d\n",
				ztest_relative_filename(file), line);
		} else {

			PRINT("\n    Assertion failed at %s:%d: %s",
			      ztest_relative_filename(file), line, func);
			if (default_msg) {
				PRINT(": %s\n", default_msg);
			} else {
				PRINT("\n");
			}
			va_list vargs;

			va_start(vargs, msg);
			vprintk(msg, vargs);
			printk("\n");
			va_end(vargs);
		}
		ztest_test_fail();
	} else if (CONFIG_ZTEST_ASSERT_VERBOSE == 2) {
		PRINT("\n   Assertion succeeded at %s:%d (%s)\n",
		      ztest_relative_filename(file), line, func);
	}
}

/** @brief Function prints a message with 2 variables that failed a check.
 *
 * Printing is deferred from macro to a function to reduce stack usage.
 *
 * @param msg User message that goes between printed values.
 * @param a_name Stringified name of the a variable.
 * @param b_name Stringified name of the b variable.
 * @param a_idx Index of format specifier to be used for a variable.
 * @param b_idx Index of format specifier to be used for b variable.
 * @param ... Variables. Each variable is provided twice (a, a, b, b).
 */
void ztest_print_values(const char *msg, const char *a_name, const char *b_name,
			int a_idx, int b_idx, ...);

#define z_zassert_2args(cond_op, a, b, default_msg, file, line, func, msg, ...) \
do { \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wpointer-arith\"") \
	__auto_type _a = (a) + 0; \
	__auto_type _b = (b) + 0; \
	bool cond = _a __DEBRACKET cond_op _b; \
	if ((cond == false) && (CONFIG_ZTEST_ASSERT_VERBOSE > 0)) { \
		int a_idx = ZASSERT_FORMAT_SPECIFIER_IDX(_a); \
		int b_idx = ZASSERT_FORMAT_SPECIFIER_IDX(_b); \
		ztest_print_values(default_msg, #a, #b, a_idx, b_idx, \
				   _a, _a, _b, _b); \
		z_zassert(cond, NULL, file, line, func, msg, ##__VA_ARGS__); \
	} else { \
		z_zassert(cond, NULL, file, line, NULL, NULL); \
	} \
	_Pragma("GCC diagnostic pop") \
} while (0)

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
 * @param default_msg Message to print if @a cond is false
 * @param msg Optional, can be NULL. Message to print if @a cond is false.
 */

#define zassert(cond, default_msg, msg, ...)			    \
	z_zassert(cond, msg ? ("(" default_msg ")") : (default_msg), \
		 __FILE__, __LINE__, __func__, msg ? msg : "", ##__VA_ARGS__)

/**
 * @brief Fail the test, if @a cond is false
 *
 * If test fails report contains values of @a a and @a b. It must be
 * conditionally compiled as it relies on C11 _Generic support. Additionally,
 * it must be ensured that @a a and @a b are evaluated only once. Because of
 * that only condition operator is passed and not full condition.
 *
 * @param cond_op Condition operator in parenthesis.
 * @param a First argument
 * @param b Second argument
 * @param default_msg Message to print if @a cond is false
 * @param msg Optional, can be NULL. Message to print if @a cond is false.
 */
#define zassert_2args(cond_op, a, b, default_msg, msg, ...) \
	z_zassert_2args(cond_op, a, b, default_msg, __FILE__, __LINE__, \
			__func__, msg ? msg : "", ##__VA_ARGS__)

/**
 * @brief Assert that this function call won't be reached
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_unreachable(msg, ...) zassert(0, "Reached unreachable code", \
					      msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a cond is true
 * @param cond Condition to check
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_true(cond, msg, ...) zassert(cond, #cond " is false", \
					     msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a cond is false
 * @param cond Condition to check
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_false(cond, msg, ...) zassert(!(cond), #cond " is true", \
					      msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a cond is 0 (success)
 * @param cond Condition to check
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_ok(cond, msg, ...) zassert(!(cond), #cond " is non-zero", \
					      msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a ptr is NULL
 * @param ptr Pointer to compare
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_is_null(ptr, msg, ...) zassert((ptr) == NULL,	    \
					       #ptr " is not NULL", \
					       msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a ptr is not NULL
 * @param ptr Pointer to compare
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_not_null(ptr, msg, ...) zassert((ptr) != NULL,	      \
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
#if ZTEST_ASSERT_PRINT_VALUES
#define zassert_equal(a, b, msg, ...) \
	zassert_2args((/**/==/**/), a, b, " not equal to ", msg, ##__VA_ARGS__)
#else
#define zassert_equal(a, b, msg, ...) \
	zassert((a) == (b), #a " not equal to " #b, msg, ##__VA_ARGS__)
#endif

/**
 * @brief Assert that @a a does not equal @a b
 *
 * @a a and @a b won't be converted and will be compared directly.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_not_equal(a, b, msg, ...) zassert((a) != (b),	      \
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
#define zassert_equal_ptr(a, b, msg, ...)			    \
	zassert((void *)(a) == (void *)(b), #a " not equal to " #b, \
		msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a a is within @a b with delta @a d
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param d Delta
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_within(a, b, d, msg, ...)			     \
	zassert(((a) >= ((b) - (d))) && ((a) <= ((b) + (d))),	     \
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
 * @param ... Arguments, see @ref zassert_mem_equal__
 *            for real arguments accepted.
 */
#define zassert_mem_equal(...) \
	zassert_mem_equal__(__VA_ARGS__)

/**
 * @brief Internal assert that 2 memory buffers have the same contents
 *
 * @note This is internal macro, to be used as a second expansion.
 *       See @ref zassert_mem_equal.
 *
 * @param buf Buffer to compare
 * @param exp Buffer with expected contents
 * @param size Size of buffers
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_mem_equal__(buf, exp, size, msg, ...)                    \
	zassert(memcmp(buf, exp, size) == 0, #buf " not equal to " #exp, \
	msg, ##__VA_ARGS__)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTSUITE_ZTEST_ASSERT_H_ */
