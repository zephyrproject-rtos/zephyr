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

#include <ztest.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *ztest_relative_filename(const char *file);
void ztest_test_fail(void);
void ztest_test_skip(void);
#if CONFIG_ZTEST_ASSERT_VERBOSE == 0

static inline bool z_zassert_(bool cond, const char *file, int line)
{
	if (cond == false) {
		PRINT("\n    Assertion failed at %s:%d\n", ztest_relative_filename(file), line);
		ztest_test_fail();
		return false;
	}

	return true;
}

#define z_zassert(cond, default_msg, file, line, func, msg, ...) z_zassert_(cond, file, line)

static inline bool z_zassume_(bool cond, const char *file, int line)
{
	if (cond == false) {
		PRINT("\n    Assumption failed at %s:%d\n", ztest_relative_filename(file), line);
		ztest_test_skip();
		return false;
	}

	return true;
}

#define z_zassume(cond, default_msg, file, line, func, msg, ...) z_zassume_(cond, file, line)

#else /* CONFIG_ZTEST_ASSERT_VERBOSE != 0 */

static inline bool z_zassert(bool cond, const char *default_msg, const char *file, int line,
			     const char *func, const char *msg, ...)
{
	if (cond == false) {
		va_list vargs;

		va_start(vargs, msg);
		PRINT("\n    Assertion failed at %s:%d: %s: %s\n", ztest_relative_filename(file),
		      line, func, default_msg);
		vprintk(msg, vargs);
		printk("\n");
		va_end(vargs);
		ztest_test_fail();
		return false;
	}
#if CONFIG_ZTEST_ASSERT_VERBOSE == 2
	else {
		PRINT("\n   Assertion succeeded at %s:%d (%s)\n", ztest_relative_filename(file),
		      line, func);
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
		PRINT("\n    Assumption failed at %s:%d: %s: %s\n", ztest_relative_filename(file),
		      line, func, default_msg);
		vprintk(msg, vargs);
		printk("\n");
		va_end(vargs);
		ztest_test_skip();
		return false;
	}
#if CONFIG_ZTEST_ASSERT_VERBOSE == 2
	else {
		PRINT("\n   Assumption succeeded at %s:%d (%s)\n", ztest_relative_filename(file),
		      line, func);
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
 * @param msg Optional, can be NULL. Message to print if @a cond is false.
 * @param default_msg Message to print if @a cond is false
 */
#define zassert(cond, default_msg, msg, ...)                                                       \
	do {                                                                                       \
		bool _ret = z_zassert(cond, msg ? ("(" default_msg ")") : (default_msg), __FILE__, \
				      __LINE__, __func__, msg ? msg : "", ##__VA_ARGS__);          \
		if (!_ret) {                                                                       \
			/* If kernel but without multithreading return. */                         \
			COND_CODE_1(KERNEL, (COND_CODE_1(CONFIG_MULTITHREADING, (), (return;))),   \
				    ())                                                            \
		}                                                                                  \
	} while (0)

#define zassume(cond, default_msg, msg, ...)                                                       \
	do {                                                                                       \
		bool _ret = z_zassume(cond, msg ? ("(" default_msg ")") : (default_msg), __FILE__, \
				      __LINE__, __func__, msg ? msg : "", ##__VA_ARGS__);          \
		if (!_ret) {                                                                       \
			/* If kernel but without multithreading return. */                         \
			COND_CODE_1(KERNEL, (COND_CODE_1(CONFIG_MULTITHREADING, (), (return;))),   \
				    ())                                                            \
		}                                                                                  \
	} while (0)
/**
 * @brief Assert that this function call won't be reached
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_unreachable(msg, ...) zassert(0, "Reached unreachable code", msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a cond is true
 * @param cond Condition to check
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_true(cond, msg, ...) zassert(cond, #cond " is false", msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a cond is false
 * @param cond Condition to check
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_false(cond, msg, ...) zassert(!(cond), #cond " is true", msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a cond is 0 (success)
 * @param cond Condition to check
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_ok(cond, msg, ...) zassert(!(cond), #cond " is non-zero", msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a ptr is NULL
 * @param ptr Pointer to compare
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_is_null(ptr, msg, ...)                                                             \
	zassert((ptr) == NULL, #ptr " is not NULL", msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a ptr is not NULL
 * @param ptr Pointer to compare
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_not_null(ptr, msg, ...) zassert((ptr) != NULL, #ptr " is NULL", msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a a equals @a b
 *
 * @a a and @a b won't be converted and will be compared directly.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_equal(a, b, msg, ...)                                                              \
	zassert((a) == (b), #a " not equal to " #b, msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a a does not equal @a b
 *
 * @a a and @a b won't be converted and will be compared directly.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_not_equal(a, b, msg, ...)                                                          \
	zassert((a) != (b), #a " equal to " #b, msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a a equals @a b
 *
 * @a a and @a b will be converted to `void *` before comparing.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_equal_ptr(a, b, msg, ...)                                                          \
	zassert((void *)(a) == (void *)(b), #a " not equal to " #b, msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a a is within @a b with delta @a d
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param d Delta
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_within(a, b, d, msg, ...)                                                          \
	zassert(((a) >= ((b) - (d))) && ((a) <= ((b) + (d))), #a " not within " #b " +/- " #d,     \
		msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a a is greater than or equal to @a l and less
 *        than or equal to @a u
 *
 * @param a Value to compare
 * @param l Lower limit
 * @param u Upper limit
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_between_inclusive(a, l, u, msg, ...)                                               \
	zassert(((a) >= (l)) && ((a) <= (u)), #a " not between " #l " and " #u " inclusive", msg,  \
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
 * @param msg Optional message to print if the assertion fails
 */
#define zassert_mem_equal__(buf, exp, size, msg, ...)                                              \
	zassert(memcmp(buf, exp, size) == 0, #buf " not equal to " #exp, msg, ##__VA_ARGS__)

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
 * @param msg Optional message to print if the assumption fails
 */
#define zassume_true(cond, msg, ...) zassume(cond, #cond " is false", msg, ##__VA_ARGS__)

/**
 * @brief Assume that @a cond is false
 *
 * If the assumption fails, the test will be marked as "skipped".
 *
 * @param cond Condition to check
 * @param msg Optional message to print if the assumption fails
 */
#define zassume_false(cond, msg, ...) zassume(!(cond), #cond " is true", msg, ##__VA_ARGS__)

/**
 * @brief Assume that @a cond is 0 (success)
 *
 * If the assumption fails, the test will be marked as "skipped".
 *
 * @param cond Condition to check
 * @param msg Optional message to print if the assumption fails
 */
#define zassume_ok(cond, msg, ...) zassume(!(cond), #cond " is non-zero", msg, ##__VA_ARGS__)

/**
 * @brief Assume that @a ptr is NULL
 *
 * If the assumption fails, the test will be marked as "skipped".
 *
 * @param ptr Pointer to compare
 * @param msg Optional message to print if the assumption fails
 */
#define zassume_is_null(ptr, msg, ...)                                                             \
	zassume((ptr) == NULL, #ptr " is not NULL", msg, ##__VA_ARGS__)

/**
 * @brief Assume that @a ptr is not NULL
 *
 * If the assumption fails, the test will be marked as "skipped".
 *
 * @param ptr Pointer to compare
 * @param msg Optional message to print if the assumption fails
 */
#define zassume_not_null(ptr, msg, ...) zassume((ptr) != NULL, #ptr " is NULL", msg, ##__VA_ARGS__)

/**
 * @brief Assume that @a a equals @a b
 *
 * @a a and @a b won't be converted and will be compared directly. If the
 * assumption fails, the test will be marked as "skipped".
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assumption fails
 */
#define zassume_equal(a, b, msg, ...)                                                              \
	zassume((a) == (b), #a " not equal to " #b, msg, ##__VA_ARGS__)

/**
 * @brief Assume that @a a does not equal @a b
 *
 * @a a and @a b won't be converted and will be compared directly. If the
 * assumption fails, the test will be marked as "skipped".
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assumption fails
 */
#define zassume_not_equal(a, b, msg, ...)                                                          \
	zassume((a) != (b), #a " equal to " #b, msg, ##__VA_ARGS__)

/**
 * @brief Assume that @a a equals @a b
 *
 * @a a and @a b will be converted to `void *` before comparing. If the
 * assumption fails, the test will be marked as "skipped".
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assumption fails
 */
#define zassume_equal_ptr(a, b, msg, ...)                                                          \
	zassume((void *)(a) == (void *)(b), #a " not equal to " #b, msg, ##__VA_ARGS__)

/**
 * @brief Assume that @a a is within @a b with delta @a d
 *
 * If the assumption fails, the test will be marked as "skipped".
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param d Delta
 * @param msg Optional message to print if the assumption fails
 */
#define zassume_within(a, b, d, msg, ...)                                                          \
	zassume(((a) >= ((b) - (d))) && ((a) <= ((b) + (d))), #a " not within " #b " +/- " #d,     \
		msg, ##__VA_ARGS__)

/**
 * @brief Assume that @a a is greater than or equal to @a l and less
 *        than or equal to @a u
 *
 * If the assumption fails, the test will be marked as "skipped".
 *
 * @param a Value to compare
 * @param l Lower limit
 * @param u Upper limit
 * @param msg Optional message to print if the assumption fails
 */
#define zassume_between_inclusive(a, l, u, msg, ...)                                               \
	zassume(((a) >= (l)) && ((a) <= (u)), #a " not between " #l " and " #u " inclusive", msg,  \
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
 * @param msg Optional message to print if the assumption fails
 */
#define zassume_mem_equal__(buf, exp, size, msg, ...)                                              \
	zassume(memcmp(buf, exp, size) == 0, #buf " not equal to " #exp, msg, ##__VA_ARGS__)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTSUITE_ZTEST_ASSERT_H_ */
