/* limits.h */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_LIMITS_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_LIMITS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define UCHAR_MAX  0xFF
#define SCHAR_MAX  0x7F
#define SCHAR_MIN  (-SCHAR_MAX - 1)

#ifdef __CHAR_UNSIGNED__
	/* 'char' is an unsigned type */
	#define CHAR_MAX  UCHAR_MAX
	#define CHAR_MIN  0
#else
	/* 'char' is a signed type */
	#define CHAR_MAX  SCHAR_MAX
	#define CHAR_MIN  SCHAR_MIN
#endif

#define CHAR_BIT    8
#define LONG_BIT    32
#define WORD_BIT    32

#define INT_MAX     0x7FFFFFFF
#define SHRT_MAX    0x7FFF
#define LONG_MAX    0x7FFFFFFFL
#define LLONG_MAX   0x7FFFFFFFFFFFFFFFLL

#define INT_MIN     (-INT_MAX - 1)
#define SHRT_MIN    (-SHRT_MAX - 1)
#define LONG_MIN    (-LONG_MAX - 1L)
#define LLONG_MIN   (-LLONG_MAX - 1LL)

#define SSIZE_MAX   INT_MAX

#define USHRT_MAX   0xFFFF
#define UINT_MAX    0xFFFFFFFFU
#define ULONG_MAX   0xFFFFFFFFUL
#define ULLONG_MAX  0xFFFFFFFFFFFFFFFFULL

#define PATH_MAX    256

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_LIMITS_H_ */
