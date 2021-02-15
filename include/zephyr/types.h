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

#ifdef CONFIG_DEPRECATED_ZEPHYR_INT_TYPES

typedef signed char         s8_t;
typedef signed short        s16_t;
typedef signed int          s32_t;
typedef signed long long    s64_t;

typedef unsigned char       u8_t;
typedef unsigned short      u16_t;
typedef unsigned int        u32_t;
typedef unsigned long long  u64_t;

#endif

/* 32 bits on ILP32 builds, 64 bits on LP64 builts */
typedef unsigned long       ulong_t;

/*
 * A type with strong alignment requiremnts, similar to C11 max_align_t. It can
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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_TYPES_H_ */
