/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS___ASSERT_H_
#define ZEPHYR_INCLUDE_SYS___ASSERT_H_

#include <stdbool.h>

#ifdef CONFIG_ASSERT
#ifndef __ASSERT_ON
#define __ASSERT_ON CONFIG_ASSERT_LEVEL
#endif
#endif

#ifdef CONFIG_FORCE_NO_ASSERT
#undef __ASSERT_ON
#define __ASSERT_ON 0
#endif

#ifdef CONFIG_ASSERT_NO_FILE_INFO
#define __ASSERT_FILE_INFO ""
#else  /* CONFIG_ASSERT_NO_FILE_INFO */
#define __ASSERT_FILE_INFO __FILE__
#endif /* CONFIG_ASSERT_NO_FILE_INFO */

#ifdef __ASSERT_ON
#if (__ASSERT_ON < 0) || (__ASSERT_ON > 2)
#error "Invalid __ASSERT() level: must be between 0 and 2"
#endif

#if __ASSERT_ON

#include <sys/printk.h>

#ifdef __cplusplus
extern "C" {
#endif

void assert_post_action(const char *file, unsigned int line);

#ifdef __cplusplus
}
#endif

#define __ASSERT_LOC(test)                               \
	printk("ASSERTION FAIL [%s] @ %s:%d\n",          \
	       Z_STRINGIFY(test),                        \
	       __ASSERT_FILE_INFO,                       \
	       __LINE__)                                 \

#define __ASSERT_NO_MSG(test)                                             \
	do {                                                              \
		if (!(test)) {                                            \
			__ASSERT_LOC(test);                               \
			assert_post_action(__ASSERT_FILE_INFO, __LINE__); \
		}                                                         \
	} while (false)

#define __ASSERT(test, fmt, ...)                                          \
	do {                                                              \
		if (!(test)) {                                            \
			__ASSERT_LOC(test);                               \
			printk("\t" fmt "\n", ##__VA_ARGS__);             \
			assert_post_action(__ASSERT_FILE_INFO, __LINE__); \
		}                                                         \
	} while (false)

#define __ASSERT_EVAL(expr1, expr2, test, fmt, ...)                \
	do {                                                       \
		expr2;                                             \
		__ASSERT(test, fmt, ##__VA_ARGS__);                \
	} while (false)

#if (__ASSERT_ON == 1)
#warning "__ASSERT() statements are ENABLED"
#endif
#else
#define __ASSERT(test, fmt, ...) { }
#define __ASSERT_EVAL(expr1, expr2, test, fmt, ...) expr1
#define __ASSERT_NO_MSG(test) { }
#endif
#else
#define __ASSERT(test, fmt, ...) { }
#define __ASSERT_EVAL(expr1, expr2, test, fmt, ...) expr1
#define __ASSERT_NO_MSG(test) { }
#endif

#endif /* ZEPHYR_INCLUDE_SYS___ASSERT_H_ */
