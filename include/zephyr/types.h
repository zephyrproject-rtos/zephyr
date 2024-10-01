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
 * Thread local variables are declared with different keywords depending on
 * which C/C++ standard that is used. C++11 and C23 uses "thread_local" whilst
 * C11 uses "_Thread_local". Previously the GNU "__thread" keyword was used
 * which is the same in both gcc and g++.
 */
#ifndef Z_THREAD_LOCAL
#if defined(__cplusplus) && (__cplusplus) >= 201103L /* C++11 */
#define Z_THREAD_LOCAL thread_local
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__) >= 202311L /* C23 */
#define Z_THREAD_LOCAL thread_local
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__) >= 201112L /* C11 */
#define Z_THREAD_LOCAL _Thread_local
#else /* Default back to old behavior which used the GNU keyword. */
#define Z_THREAD_LOCAL __thread
#endif
#endif /* Z_THREAD_LOCAL */

#ifdef __cplusplus
/* Zephyr requires an int main(void) signature with C linkage for the application main if present */
extern int main(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_TYPES_H_ */
