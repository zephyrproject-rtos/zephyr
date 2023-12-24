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

#ifdef __cplusplus
/* Zephyr requires an int main(void) signature with C linkage for the application main if present */
extern int main(void);
#endif

#ifdef __cplusplus
}
#endif

/*
 * Below are truly Zephyr-specific types that should never collide with
 * any application/library that wants zephyr/types.h.
 */

/* sparse defines __CHECKER__; see doc/develop/sca/sparse.rst */
#ifdef __CHECKER__
#define __bitwise	__attribute__((bitwise))
#else
#define __bitwise
#endif

typedef uint16_t __bitwise __le16;
typedef uint16_t __bitwise __be16;
typedef uint32_t __bitwise __le32;
typedef uint32_t __bitwise __be32;
typedef uint64_t __bitwise __le64;
typedef uint64_t __bitwise __be64;

#endif /* ZEPHYR_INCLUDE_ZEPHYR_TYPES_H_ */
