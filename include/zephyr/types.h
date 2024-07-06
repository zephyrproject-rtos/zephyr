/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_TYPES_H_
#define ZEPHYR_INCLUDE_ZEPHYR_TYPES_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A type with strong alignment requirements, similar to C11 max_align_t. It can
 * be used to force alignment of data structures allocated on the stack or as
 * return * type for heap allocators.
 */
typedef union {
	long long       thelonglong;
	long double     thelongdouble;
	uintmax_t       theuintmax_t;
	size_t          thesize_t;
	uintptr_t       theuintptr_t;
	void            *thepvoid;
	void            (*thepfunc)(void);
} z_max_align_t;

/*
 * Offset type
 *
 * k_off_t, much like the POSIX off_t, is a signed integer type used to represent file sizes and
 * offsets.
 *
 * k_off_t effectively limits the size of files that can be handled by the system, it is
 * therefore not tied to the word-size of the architecture.
 *
 * In order to overcome historical limitations of the off_t type, Zephyr may be configured to
 * always define k_off_t as 64-bits.
 */
#ifdef CONFIG_OFFSET_64BIT
typedef int64_t k_off_t;
#else
typedef long k_off_t;
#endif

/*
 * Signed size type
 *
 * k_ssize_t, much like the POSIX ssize_t, is a signed integer type that effectively limits the
 * size of I/O operations on files while still supporting negative return values.
 *
 * k_ssize_t is usually tied to the word-size of the architecture.
 */
#ifdef __SIZE_TYPE__
#define unsigned signed /* parasoft-suppress MISRAC2012-RULE_20_4-a MISRAC2012-RULE_20_4-b */
typedef __SIZE_TYPE__ k_ssize_t;
#undef unsigned
#else
#ifdef CONFIG_64BIT
typedef long k_ssize_t;
#else
typedef int k_ssize_t;
#endif
#endif

#ifdef __cplusplus
/* Zephyr requires an int main(void) signature with C linkage for the application main if present */
extern int main(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_TYPES_H_ */
