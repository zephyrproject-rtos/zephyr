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

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *ztest_relative_filename(const char *file);
void ztest_test_fail(void);
void ztest_test_skip(void);
void ztest_test_expect_fail(void);
void ztest_skip_failed_assumption(void);
#if CONFIG_ZTEST_ASSERT_VERBOSE == 0

static inline bool z_zassert_(bool cond, const char *file, int line)
{
	if (cond == false) {
		PRINT_DATA("\n    Assertion failed at %s:%d\n", ztest_relative_filename(file),
			   line);
		ztest_test_fail();
		return false;
	}

	return true;
}

#define z_zassert(cond, default_msg, file, line, func, msg, ...) z_zassert_(cond, file, line)

static inline bool z_zassume_(bool cond, const char *file, int line)
{
	if (cond == false) {
		PRINT_DATA("\n    Assumption failed at %s:%d\n", ztest_relative_filename(file),
			   line);
		ztest_skip_failed_assumption();
		return false;
	}

	return true;
}

#define z_zassume(cond, default_msg, file, line, func, msg, ...) z_zassume_(cond, file, line)

static inline bool z_zexpect_(bool cond, const char *file, int line)
{
	if (cond == false) {
		PRINT_DATA("\n    Expectation failed at %s:%d\n", ztest_relative_filename(file),
			   line);
		ztest_test_expect_fail();
		return false;
	}

	return true;
}

#define z_zexpect(cond, default_msg, file, line, func, msg, ...) z_zexpect_(cond, file, line)

#else /* CONFIG_ZTEST_ASSERT_VERBOSE != 0 */

static inline bool z_zassert(bool cond, const char *default_msg, const char *file, int line,
			     const char *func, const char *msg, ...)
{
	if (cond == false) {
		va_list vargs;

		va_start(vargs, msg);
		PRINT_DATA("\n    Assertion failed at %s:%d: %s: %s\n",
			   ztest_relative_filename(file), line, func, default_msg);
		vprintk(msg, vargs);
		printk("\n");
		va_end(vargs);
		ztest_test_fail();
		return false;
	}
#if CONFIG_ZTEST_ASSERT_VERBOSE == 2
	else {
		PRINT_DATA("\n   Assertion succeeded at %s:%d (%s)\n",
			   ztest_relative_filename(file), line, func);
	}
#endif
	return true;
}

static inline bool z_zassume(bool cond, const char *default_msg, const char *file, int line,
			     const char *func, const char *msg, ...)
{
	if (cond == false) {
		va_list vargs;

		va_start(vargs, msg);
		PRINT_DATA("\n    Assumption failed at %s:%d: %s: %s\n",
			   ztest_relative_filename(file), line, func, default_msg);
		vprintk(msg, vargs);
		printk("\n");
		va_end(vargs);
		ztest_skip_failed_assumption();
		return false;
	}
#if CONFIG_ZTEST_ASSERT_VERBOSE == 2
	else {
		PRINT_DATA("\n   Assumption succeeded at %s:%d (%s)\n",
			   ztest_relative_filename(file), line, func);
	}
#endif
	return true;
}

static inline bool z_zexpect(bool cond, const char *default_msg, const char *file, int line,
			     const char *func, const char *msg, ...)
{
	if (cond == false) {
		va_list vargs;

		va_start(vargs, msg);
		PRINT_DATA("\n    Expectation failed at %s:%d: %s: %s\n",
			   ztest_relative_filename(file), line, func, default_msg);
		vprintk(msg, vargs);
		printk("\n");
		va_end(vargs);
		ztest_test_expect_fail();
		return false;
	}
#if CONFIG_ZTEST_ASSERT_VERBOSE == 2
	else {
		PRINT_DATA("\n   Expectation succeeded at %s:%d (%s)\n",
			   ztest_relative_filename(file), line, func);
	}
#endif
	return true;
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
 * Note that when CONFIG_MULTITHREADING=n macro returns from the function. It is
 * then expected that in that case ztest asserts will be used only in the
 * context of the test function.
 *
 * @param cond Condition to check
 * @param default_msg Message to print if @a cond is false
 * @param msg Optional, can be NULL. Message to print if @a cond is false.
 */
#define _zassert_base(cond, default_msg, msg, ...)                                                 \
	do {                                                                                       \
		bool _msg = (msg != NULL);                                                         \
		bool _ret =                                                                        \
			z_zassert(cond, _msg ? ("(" default_msg ")") : (default_msg), __FILE__,    \
				  __LINE__, __func__, _msg ? msg : "", ##__VA_ARGS__);             \
		(void)_msg;                                                                        \
		if (!_ret) {                                                                       \
			/* If kernel but without multithreading return. */                         \
			COND_CODE_1(KERNEL, (COND_CODE_1(CONFIG_MULTITHREADING, (), (return;))),   \
				    ())                                                            \
		}                                                                                  \
	} while (0)

#define _zassert_va(cond, default_msg, msg, ...)                                                   \
	_zassert_base(cond, default_msg, msg, ##__VA_ARGS__)

#define zassert(cond, default_msg, ...)                                                            \
	_zassert_va(cond, default_msg, COND_CODE_1(__VA_OPT__(1), (__VA_ARGS__), (NULL)))

/**
 * @brief Skip the test, if @a cond is false
 *
 * You probably don't need to call this macro directly. You should
 * instead use zassume_{condition} macros below.
 *
 * Note that when CONFIG_MULTITHREADING=n macro returns from the function. It's then expected that
 * in that case ztest assumes will be used only in the context of the test function.
 *
 * NOTE: zassume should not be used to replace zassert, the goal of zassume is to skip tests that
 * would otherwise fail due to a zassert on some other dependent behavior that is *not* under test,
 * thus reducing what could be tens to hundreds of assertion failures to investigate down to a few
 * failures only.
 *
 * @param cond Condition to check
 * @param default_msg Message to print if @a cond is false
 * @param msg Optional, can be NULL. Message to print if @a cond is false.
 */
#define _zassume_base(cond, default_msg, msg, ...)                                                 \
	do {                                                                                       \
		bool _msg = (msg != NULL);                                                         \
		bool _ret =                                                                        \
			z_zassume(cond, _msg ? ("(" default_msg ")") : (default_msg), __FILE__,    \
				  __LINE__, __func__, _msg ? msg : "", ##__VA_ARGS__);             \
		(void)_msg;                                                                        \
		if (!_ret) {                                                                       \
			/* If kernel but without multithreading return. */                         \
			COND_CODE_1(KERNEL, (COND_CODE_1(CONFIG_MULTITHREADING, (), (return;))),   \
				    ())                                                            \
		}                                                                                  \
	} while (0)

#define _zassume_va(cond, default_msg, msg, ...)                                                   \
	_zassume_base(cond, default_msg, msg, ##__VA_ARGS__)

#define zassume(cond, default_msg, ...)                                                            \
	_zassume_va(cond, default_msg, COND_CODE_1(__VA_OPT__(1), (__VA_ARGS__), (NULL)))

/**
 * @brief If @a cond is false, fail the test but continue its execution.
 *
 * You probably don't need to call this macro directly. You should
 * instead use zexpect_{condition} macros below.
 *
 * @param cond Condition to check
 * @param default_msg Message to print if @a cond is false
 * @param msg Optional, can be NULL. Message to print if @a cond is false.
 */
#define _zexpect_base(cond, default_msg, msg, ...)                                                 \
	do {                                                                                       \
		bool _msg = (msg != NULL);                                                         \
		bool _ret =                                                                        \
			z_zexpect(cond, _msg ? ("(" default_msg ")") : (default_msg), __FILE__,    \
				  __LINE__, __func__, _msg ? msg : "", ##__VA_ARGS__);             \
		(void)_msg;                                                                        \
		if (!_ret) {                                                                       \
			/* If kernel but without multithreading return. */                         \
			COND_CODE_1(KERNEL, (COND_CODE_1(CONFIG_MULTITHREADING, (), (return;))),   \
				    ())                                                            \
		}                                                                                  \
	} while (0)

#define _zexpect_va(cond, default_msg, msg, ...)                                                   \
	_zexpect_base(cond, default_msg, msg, ##__VA_ARGS__)

#define zexpect(cond, default_msg, ...)                                                            \
	_zexpect_va(cond, default_msg, COND_CODE_1(__VA_OPT__(1), (__VA_ARGS__), (NULL)))

/**
 * @brief Assert that this function call won't be reached
 * @param ... Optional message and variables to print if the assertion fails
 */
#define zassert_unreachable(...) zassert(0, "Reached unreachable code", ##__VA_ARGS__)

/**
 * @brief Assert that @a cond is true
 * @param cond Condition to check
 * @param ... Optional message and variables to print if the assertion fails
 */
#define zassert_true(cond, ...) zassert(cond, #cond " is false", ##__VA_ARGS__)

/**
 * @brief Assert that @a cond is false
 * @param cond Condition to check
 * @param ... Optional message and variables to print if the assertion fails
 */
#define zassert_false(cond, ...) zassert(!(cond), #cond " is true", ##__VA_ARGS__)

/**
 * @brief Assert that @a cond is 0 (success)
 * @param cond Condition to check
 * @param ... Optional message and variables to print if the assertion fails
 */
#define zassert_ok(cond, ...) zassert(!(cond), #cond " is non-zero", ##__VA_ARGS__)

/**
 * @brief Assert that @a cond is not 0 (failure)
 * @param cond Condition to check
 * @param ... Optional message and variables to print if the assertion fails
 */
#define zassert_not_ok(cond, ...) zassert(!!(cond), #cond " is zero", ##__VA_ARGS__)

/**
 * @brief Assert that @a ptr is NULL
 * @param ptr Pointer to compare
 * @param ... Optional message and variables to print if the assertion fails
 */
#define zassert_is_null(ptr, ...) zassert((ptr) == NULL, #ptr " is not NULL", ##__VA_ARGS__)

/**
 * @brief Assert that @a ptr is not NULL
 * @param ptr Pointer to compare
 * @param ... Optional message and variables to print if the assertion fails
 */
#define zassert_not_null(ptr, ...) zassert((ptr) != NULL, #ptr " is NULL", ##__VA_ARGS__)

/**
 * @brief Assert that @a a equals @a b
 *
 * @a a and @a b won't be converted and will be compared directly.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param ... Optional message and variables to print if the assertion fails
 */
#define zassert_equal(a, b, ...) zassert((a) == (b), #a " not equal to " #b, ##__VA_ARGS__)

/**
 * @brief Assert that @a a does not equal @a b
 *
 * @a a and @a b won't be converted and will be compared directly.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param ... Optional message and variables to print if the assertion fails
 */
#define zassert_not_equal(a, b, ...) zassert((a) != (b), #a " equal to " #b, ##__VA_ARGS__)

/**
 * @brief Assert that @a a equals @a b
 *
 * @a a and @a b will be converted to `void *` before comparing.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param ... Optional message and variables to print if the assertion fails
 */
#define zassert_equal_ptr(a, b, ...)                                                               \
	zassert((void *)(a) == (void *)(b), #a " not equal to " #b, ##__VA_ARGS__)

/**
 * @brief Assert that @a a is within @a b with delta @a d
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param d Delta
 * @param ... Optional message and variables to print if the assertion fails
 */
#define zassert_within(a, b, d, ...)                                                               \
	zassert(((a) >= ((b) - (d))) && ((a) <= ((b) + (d))), #a " not within " #b " +/- " #d,     \
		##__VA_ARGS__)

/**
 * @brief Assert that @a a is greater than or equal to @a l and less
 *        than or equal to @a u
 *
 * @param a Value to compare
 * @param l Lower limit
 * @param u Upper limit
 * @param ... Optional message and variables to print if the assertion fails
 */
#define zassert_between_inclusive(a, l, u, ...)                                                    \
	zassert(((a) >= (l)) && ((a) <= (u)), #a " not between " #l " and " #u " inclusive",       \
		##__VA_ARGS__)

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
#define zassert_mem_equal(...) zassert_mem_equal__(__VA_ARGS__)

/**
 * @brief Internal assert that 2 memory buffers have the same contents
 *
 * @note This is internal macro, to be used as a second expansion.
 *       See @ref zassert_mem_equal.
 *
 * @param buf Buffer to compare
 * @param exp Buffer with expected contents
 * @param size Size of buffers
 * @param ... Optional message and variables to print if the assertion fails
 */
#define zassert_mem_equal__(buf, exp, size, ...)                                                   \
	zassert(memcmp(buf, exp, size) == 0, #buf " not equal to " #exp, ##__VA_ARGS__)

/**
 * @brief Assert that 2 strings have the same contents
 *
 * @param s1 The first string
 * @param s2 The second string
 * @param ... Optional message and variables to print if the expectation fails
 */
#define zassert_str_equal(s1, s2, ...)                                                             \
	zassert(strcmp(s1, s2) == 0, #s1 " not equal to " #s2, ##__VA_ARGS__)

/**
 * @}
 */

/**
 * @defgroup ztest_assume Ztest assumption macros
 * @ingroup ztest
 *
 * This module provides assumptions when using Ztest.
 *
 * @{
 */

/**
 * @brief Assume that @a cond is true
 *
 * If the assumption fails, the test will be marked as "skipped".
 *
 * @param cond Condition to check
 * @param ... Optional message and variables to print if the assumption fails
 */
#define zassume_true(cond, ...) zassume(cond, #cond " is false", ##__VA_ARGS__)

/**
 * @brief Assume that @a cond is false
 *
 * If the assumption fails, the test will be marked as "skipped".
 *
 * @param cond Condition to check
 * @param ... Optional message and variables to print if the assumption fails
 */
#define zassume_false(cond, ...) zassume(!(cond), #cond " is true", ##__VA_ARGS__)

/**
 * @brief Assume that @a cond is 0 (success)
 *
 * If the assumption fails, the test will be marked as "skipped".
 *
 * @param cond Condition to check
 * @param ... Optional message and variables to print if the assumption fails
 */
#define zassume_ok(cond, ...) zassume(!(cond), #cond " is non-zero", ##__VA_ARGS__)

/**
 * @brief Assume that @a cond is not 0 (failure)
 *
 * If the assumption fails, the test will be marked as "skipped".
 *
 * @param cond Condition to check
 * @param ... Optional message and variables to print if the assumption fails
 */
#define zassume_not_ok(cond, ...) zassume(!!(cond), #cond " is zero", ##__VA_ARGS__)

/**
 * @brief Assume that @a ptr is NULL
 *
 * If the assumption fails, the test will be marked as "skipped".
 *
 * @param ptr Pointer to compare
 * @param ... Optional message and variables to print if the assumption fails
 */
#define zassume_is_null(ptr, ...) zassume((ptr) == NULL, #ptr " is not NULL", ##__VA_ARGS__)

/**
 * @brief Assume that @a ptr is not NULL
 *
 * If the assumption fails, the test will be marked as "skipped".
 *
 * @param ptr Pointer to compare
 * @param ... Optional message and variables to print if the assumption fails
 */
#define zassume_not_null(ptr, ...) zassume((ptr) != NULL, #ptr " is NULL", ##__VA_ARGS__)

/**
 * @brief Assume that @a a equals @a b
 *
 * @a a and @a b won't be converted and will be compared directly. If the
 * assumption fails, the test will be marked as "skipped".
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param ... Optional message and variables to print if the assumption fails
 */
#define zassume_equal(a, b, ...) zassume((a) == (b), #a " not equal to " #b, ##__VA_ARGS__)

/**
 * @brief Assume that @a a does not equal @a b
 *
 * @a a and @a b won't be converted and will be compared directly. If the
 * assumption fails, the test will be marked as "skipped".
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param ... Optional message and variables to print if the assumption fails
 */
#define zassume_not_equal(a, b, ...) zassume((a) != (b), #a " equal to " #b, ##__VA_ARGS__)

/**
 * @brief Assume that @a a equals @a b
 *
 * @a a and @a b will be converted to `void *` before comparing. If the
 * assumption fails, the test will be marked as "skipped".
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param ... Optional message and variables to print if the assumption fails
 */
#define zassume_equal_ptr(a, b, ...)                                                               \
	zassume((void *)(a) == (void *)(b), #a " not equal to " #b, ##__VA_ARGS__)

/**
 * @brief Assume that @a a is within @a b with delta @a d
 *
 * If the assumption fails, the test will be marked as "skipped".
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param d Delta
 * @param ... Optional message and variables to print if the assumption fails
 */
#define zassume_within(a, b, d, ...)                                                               \
	zassume(((a) >= ((b) - (d))) && ((a) <= ((b) + (d))), #a " not within " #b " +/- " #d,     \
		##__VA_ARGS__)

/**
 * @brief Assume that @a a is greater than or equal to @a l and less
 *        than or equal to @a u
 *
 * If the assumption fails, the test will be marked as "skipped".
 *
 * @param a Value to compare
 * @param l Lower limit
 * @param u Upper limit
 * @param ... Optional message and variables to print if the assumption fails
 */
#define zassume_between_inclusive(a, l, u, ...)                                                    \
	zassume(((a) >= (l)) && ((a) <= (u)), #a " not between " #l " and " #u " inclusive",       \
		##__VA_ARGS__)

/**
 * @brief Assume that 2 memory buffers have the same contents
 *
 * This macro calls the final memory comparison assumption macro.
 * Using double expansion allows providing some arguments by macros that
 * would expand to more than one values (ANSI-C99 defines that all the macro
 * arguments have to be expanded before macro call).
 *
 * @param ... Arguments, see @ref zassume_mem_equal__
 *            for real arguments accepted.
 */
#define zassume_mem_equal(...) zassume_mem_equal__(__VA_ARGS__)

/**
 * @brief Internal assume that 2 memory buffers have the same contents
 *
 * If the assumption fails, the test will be marked as "skipped".
 *
 * @note This is internal macro, to be used as a second expansion.
 *       See @ref zassume_mem_equal.
 *
 * @param buf Buffer to compare
 * @param exp Buffer with expected contents
 * @param size Size of buffers
 * @param ... Optional message and variables to print if the assumption fails
 */
#define zassume_mem_equal__(buf, exp, size, ...)                                                   \
	zassume(memcmp(buf, exp, size) == 0, #buf " not equal to " #exp, ##__VA_ARGS__)

/**
 * @brief Assumes that 2 strings have the same contents
 *
 * @param s1 The first string
 * @param s2 The second string
 * @param ... Optional message and variables to print if the expectation fails
 */
#define zassume_str_equal(s1, s2, ...)                                                             \
	zassume(strcmp(s1, s2) == 0, #s1 " not equal to " #s2, ##__VA_ARGS__)

/**
 * @}
 */

/**
 * @defgroup ztest_expect Ztest expectation macros
 * @ingroup ztest
 *
 * This module provides expectations when using Ztest.
 *
 * @{
 */

/**
 * @brief Expect that @a cond is true, otherwise mark test as failed but continue its execution.
 *
 * @param cond Condition to check
 * @param ... Optional message and variables to print if the expectation fails
 */
#define zexpect_true(cond, ...) zexpect(cond, #cond " is false", ##__VA_ARGS__)

/**
 * @brief Expect that @a cond is false, otherwise mark test as failed but continue its execution.
 *
 * @param cond Condition to check
 * @param ... Optional message and variables to print if the expectation fails
 */
#define zexpect_false(cond, ...) zexpect(!(cond), #cond " is true", ##__VA_ARGS__)

/**
 * @brief Expect that @a cond is 0 (success), otherwise mark test as failed but continue its
 * execution.
 *
 * @param cond Condition to check
 * @param ... Optional message and variables to print if the expectation fails
 */
#define zexpect_ok(cond, ...) zexpect(!(cond), #cond " is non-zero", ##__VA_ARGS__)

/**
 * @brief Expect that @a cond is not 0 (failure), otherwise mark test as failed but continue its
 * execution.
 *
 * @param cond Condition to check
 * @param ... Optional message and variables to print if the expectation fails
 */
#define zexpect_not_ok(cond, ...) zexpect(!!(cond), #cond " is zero", ##__VA_ARGS__)

/**
 * @brief Expect that @a ptr is NULL, otherwise mark test as failed but continue its execution.
 *
 * @param ptr Pointer to compare
 * @param ... Optional message and variables to print if the expectation fails
 */
#define zexpect_is_null(ptr, ...) zexpect((ptr) == NULL, #ptr " is not NULL", ##__VA_ARGS__)

/**
 * @brief Expect that @a ptr is not NULL, otherwise mark test as failed but continue its execution.
 *
 * @param ptr Pointer to compare
 * @param ... Optional message and variables to print if the expectation fails
 */
#define zexpect_not_null(ptr, ...) zexpect((ptr) != NULL, #ptr " is NULL", ##__VA_ARGS__)

/**
 * @brief Expect that @a a equals @a b, otherwise mark test as failed but continue its execution.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param ... Optional message and variables to print if the expectation fails
 */
#define zexpect_equal(a, b, ...) zexpect((a) == (b), #a " not equal to " #b, ##__VA_ARGS__)

/**
 * @brief Expect that @a a does not equal @a b, otherwise mark test as failed but continue its
 * execution.
 *
 * @a a and @a b won't be converted and will be compared directly.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param ... Optional message and variables to print if the expectation fails
 */
#define zexpect_not_equal(a, b, ...) zexpect((a) != (b), #a " equal to " #b, ##__VA_ARGS__)

/**
 * @brief Expect that @a a equals @a b, otherwise mark test as failed but continue its execution.
 *
 * @a a and @a b will be converted to `void *` before comparing.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param ... Optional message and variables to print if the expectation fails
 */
#define zexpect_equal_ptr(a, b, ...)                                                               \
	zexpect((void *)(a) == (void *)(b), #a " not equal to " #b, ##__VA_ARGS__)

/**
 * @brief Expect that @a a is within @a b with delta @a d, otherwise mark test as failed but
 * continue its execution.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param delta Difference between a and b
 * @param ... Optional message and variables to print if the expectation fails
 */
#define zexpect_within(a, b, delta, ...)                                                           \
	zexpect(((a) >= ((b) - (delta))) && ((a) <= ((b) + (delta))),                              \
		#a " not within " #b " +/- " #delta, ##__VA_ARGS__)

/**
 * @brief Expect that @a a is greater than or equal to @a l and less
 *        than or equal to @a u, otherwise mark test as failed but continue its execution.
 *
 * @param a Value to compare
 * @param lower Lower limit
 * @param upper Upper limit
 * @param ... Optional message and variables to print if the expectation fails
 */
#define zexpect_between_inclusive(a, lower, upper, ...)                                            \
	zexpect(((a) >= (lower)) && ((a) <= (upper)),                                              \
		#a " not between " #lower " and " #upper " inclusive", ##__VA_ARGS__)

/**
 * @brief Expect that 2 memory buffers have the same contents, otherwise mark test as failed but
 * continue its execution.
 *
 * @param buf Buffer to compare
 * @param exp Buffer with expected contents
 * @param size Size of buffers
 * @param ... Optional message and variables to print if the expectation fails
 */
#define zexpect_mem_equal(buf, exp, size, ...)                                                     \
	zexpect(memcmp(buf, exp, size) == 0, #buf " not equal to " #exp, ##__VA_ARGS__)

/**
 * @brief Expect that 2 strings have the same contents, otherwise mark test as failed but
 * continue its execution.
 *
 * @param s1 The first string
 * @param s2 The second string
 * @param ... Optional message and variables to print if the expectation fails
 */
#define zexpect_str_equal(s1, s2, ...)                                                             \
	zexpect(strcmp(s1, s2) == 0, #s1 " not equal to " #s2, ##__VA_ARGS__)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTSUITE_ZTEST_ASSERT_H_ */
