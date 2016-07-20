/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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
 * @brief Debug aid
 *
 *
 * The __ASSERT() macro can be used inside kernel code.
 *
 * Assertions are enabled by setting the __ASSERT_ON symbol to a non-zero value.
 * There are two ways to do this:
 *   a) Use the ASSERT and ASSERT_LEVEL kconfig options
 *   b) Add "CFLAGS += -D__ASSERT_ON=<level>" at the end of a project's Makefile
 * The Makefile method takes precedence over the kconfig option if both are
 * used.
 *
 * Specifying an assertion level of 1 causes the compiler to issue warnings that
 * the kernel contains debug-type __ASSERT() statements; this reminder is issued
 * since assertion code is not normally present in a final product. Specifying
 * assertion level 2 suppresses these warnings.
 *
 * The __ASSERT_EVAL() macro can also be used inside kernel code.
 *
 * It makes use of the __ASSERT() macro, but has some extra flexibility.  It
 * allows the developer to specify different actions depending whether the
 * __ASSERT() macro is enabled or not.  This can be particularly useful to
 * prevent the compiler from generating comments (errors, warnings or remarks)
 * about variables that are only used with __ASSERT() being assigned a value,
 * but otherwise unused when the __ASSERT() macro is disabled.
 *
 * Consider the following example:
 *
 * int  x;
 *
 * x = foo ();
 * __ASSERT (x != 0, "foo() returned zero!");
 *
 * If __ASSERT() is disabled, then 'x' is assigned a value, but never used.
 * This type of situation can be resolved using the __ASSERT_EVAL() macro.
 *
 * __ASSERT_EVAL ((void) foo(),
 *		  int x = foo(),
 *                x != 0,
 *                "foo() returned zero!");
 *
 * The first parameter tells __ASSERT_EVAL() what to do if __ASSERT() is
 * disabled.  The second parameter tells __ASSERT_EVAL() what to do if
 * __ASSERT() is enabled.  The third and fourth parameters are the parameters
 * it passes to __ASSERT().
 *
 * The __ASSERT_NO_MSG() macro can be used to perform an assertion that reports
 * the failed test and its location, but lacks additional debugging information
 * provided to assist the user in diagnosing the problem; its use is
 * discouraged.
 */

#ifndef ___ASSERT__H_
#define ___ASSERT__H_

#ifdef CONFIG_ASSERT
#ifndef __ASSERT_ON
#define __ASSERT_ON CONFIG_ASSERT_LEVEL
#endif
#endif

#ifdef __ASSERT_ON
#if (__ASSERT_ON < 0) || (__ASSERT_ON > 2)
#error "Invalid __ASSERT() level: must be between 0 and 2"
#endif

#if __ASSERT_ON
#include <misc/printk.h>
#define __ASSERT(test, fmt, ...)                                   \
	do {                                                       \
		if (!(test)) {                                     \
			printk("ASSERTION FAIL [%s] @ %s:%d:\n\t", \
			       _STRINGIFY(test),                   \
			       __FILE__,                           \
			       __LINE__);                          \
			printk(fmt, ##__VA_ARGS__);                \
			for (;;)                                   \
				; /* spin thread */                \
		}                                                  \
	} while ((0))

#define __ASSERT_EVAL(expr1, expr2, test, fmt, ...)                \
	do {                                                       \
		expr2;                                             \
		__ASSERT(test, fmt, ##__VA_ARGS__);                \
	} while (0)

#if (__ASSERT_ON == 1)
#warning "__ASSERT() statements are ENABLED"
#endif
#else
#define __ASSERT(test, fmt, ...) \
	do {/* nothing */        \
	} while ((0))
#define __ASSERT_EVAL(expr1, expr2, test, fmt, ...) expr1
#endif
#else
#define __ASSERT(test, fmt, ...) \
	do {/* nothing */        \
	} while ((0))
#define __ASSERT_EVAL(expr1, expr2, test, fmt, ...) expr1
#endif

#define __ASSERT_NO_MSG(test) __ASSERT(test, "")

#endif /* ___ASSERT__H_ */
