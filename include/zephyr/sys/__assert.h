/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS___ASSERT_H_
#define ZEPHYR_INCLUDE_SYS___ASSERT_H_

#include <zephyr/sys/zassert.h>

#if CONFIG_ASSERT_MODULE_DEFAULT_LEVEL > ZASSERT_LEVEL_OFF


/* @cond INTERNAL_HIDDEN */

#define _ZASSERT_M(_module, _test, ...) \
	_ZASSERT_LEVEL(_ZASSERT_MODULE_LEVEL(_module), (_test), ##__VA_ARGS__)
/**
 * @brief Legacy assertion with an optional message.
 *
 * Compatibility shim for the historical __ASSERT() macro. It forwards to the
 * private ZASSERT_M() macro using the DEFAULT module, whose level is
 * CONFIG_ASSERT_MODULE_DEFAULT_LEVEL. New code should use ZASSERT() instead.
 *
 * @param test Condition to check. A fatal error is raised if it is false.
 * @param ...  Optional message format string followed by its arguments.
 */
#define __ASSERT(test, ...)   _ZASSERT_M(DEFAULT, test, ##__VA_ARGS__)

/**
 * @brief Legacy assertion with no message.
 *
 * Equivalent to __ASSERT() without a user message; uses the DEFAULT module.
 *
 * @param test Condition to check. A fatal error is raised if it is false.
 */
#define __ASSERT_NO_MSG(test) _ZASSERT_M(DEFAULT, test)

/**
 * @brief Print a formatted assertion message when verbose.
 *
 * Emits @p fmt through zassert_print() only when the DEFAULT module level is at
 * least ZASSERT_LEVEL_VERBOSE; otherwise it expands to nothing.
 *
 * @param fmt Message format string.
 * @param ... Arguments for @p fmt.
 */
#define __ASSERT_PRINT(fmt, ...)                                       \
	do {                                                           \
		if (CONFIG_ASSERT_MODULE_DEFAULT_LEVEL >= ZASSERT_LEVEL_VERBOSE) { \
			zassert_print(fmt, ##__VA_ARGS__);             \
		}                                                      \
	} while (false)

/**
 * @brief Print an indented, newline-terminated supplemental info line.
 *
 * Convenience wrapper around __ASSERT_PRINT() used to append extra context
 * below an assertion failure.
 *
 * @param fmt Message format string.
 * @param ... Arguments for @p fmt.
 */
#define __ASSERT_MSG_INFO(fmt, ...) __ASSERT_PRINT("\t" fmt "\n", ##__VA_ARGS__)

/**
 * @brief Print the location of a failing assertion.
 *
 * Emits an "ASSERTION FAIL [cond] @ file:line" line for @p test using
 * __ASSERT_PRINT().
 *
 * @param test Failing condition, stringified into the output.
 */
#define __ASSERT_LOC(test) \
	__ASSERT_PRINT("ASSERTION FAIL [%s] @ %s:%d\n", Z_STRINGIFY(test), __FILE__, __LINE__)

/**
 * @brief Take the terminal action for a failed assertion.
 *
 * Invokes zassert_post_action() with the current source location.
 */
#define __ASSERT_POST_ACTION() zassert_post_action(__FILE__, __LINE__)

/**
 * @brief Evaluate an expression and assert, depending on the assert level.
 *
 * When assertions are enabled, evaluates @p expr2 and then asserts @p test.
 * When assertions are disabled, this macro instead evaluates @p expr1, so the
 * side effect required in production builds is preserved without the check.
 *
 * @param expr1 Expression evaluated when assertions are disabled.
 * @param expr2 Expression evaluated when assertions are enabled.
 * @param test  Condition to check. A fatal error is raised if it is false.
 * @param fmt   Message format string.
 * @param ...   Arguments for @p fmt.
 */
#define __ASSERT_EVAL(expr1, expr2, test, fmt, ...) \
	do {                                        \
		expr2;                              \
		__ASSERT(test, fmt, ##__VA_ARGS__); \
	} while (false)

#else /* CONFIG_ASSERT_MODULE_DEFAULT_LEVEL > ZASSERT_LEVEL_OFF */

#define __ASSERT(test, ...)   ((void)0)
#define __ASSERT_NO_MSG(test) ((void)0)
#define __ASSERT_PRINT(fmt, ...) ((void)0)
#define __ASSERT_MSG_INFO(fmt, ...) ((void)0)
#define __ASSERT_LOC(test) ((void)0)
#define __ASSERT_POST_ACTION() ((void)0)
#define __ASSERT_EVAL(expr1, expr2, test, fmt, ...) expr1

#endif /* CONFIG_ASSERT_MODULE_DEFAULT_LEVEL >= 1 */
/* @endcond */
#endif /* ZEPHYR_INCLUDE_SYS___ASSERT_H_ */
