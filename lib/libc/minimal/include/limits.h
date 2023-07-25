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

#if __CHAR_BIT__ == 8
#define UCHAR_MAX	0xFFU
#else
#error "unexpected __CHAR_BIT__ value"
#endif

#define SCHAR_MAX	__SCHAR_MAX__
#define SCHAR_MIN	(-SCHAR_MAX - 1)

#ifdef __CHAR_UNSIGNED__
	/* 'char' is an unsigned type */
	#define CHAR_MAX  UCHAR_MAX
	#define CHAR_MIN  0
#else
	/* 'char' is a signed type */
	#define CHAR_MAX  SCHAR_MAX
	#define CHAR_MIN  SCHAR_MIN
#endif

#define CHAR_BIT	__CHAR_BIT__
#define LONG_BIT	(__SIZEOF_LONG__ * CHAR_BIT)
#define WORD_BIT	(__SIZEOF_POINTER__ * CHAR_BIT)

#define INT_MAX		__INT_MAX__
#define SHRT_MAX	__SHRT_MAX__
#define LONG_MAX	__LONG_MAX__
#define LLONG_MAX	__LONG_LONG_MAX__

#define INT_MIN		(-INT_MAX - 1)
#define SHRT_MIN	(-SHRT_MAX - 1)
#define LONG_MIN	(-LONG_MAX - 1L)
#define LLONG_MIN	(-LLONG_MAX - 1LL)

#if __SIZE_MAX__ == __UINT32_MAX__
#define SSIZE_MAX	__INT32_MAX__
#elif __SIZE_MAX__ == __UINT64_MAX__
#define SSIZE_MAX	__INT64_MAX__
#else
#error "unexpected __SIZE_MAX__ value"
#endif

#if __SIZEOF_SHORT__ == 2
#define USHRT_MAX	0xFFFFU
#else
#error "unexpected __SIZEOF_SHORT__ value"
#endif

#if __SIZEOF_INT__ == 4
#define UINT_MAX	0xFFFFFFFFU
#else
#error "unexpected __SIZEOF_INT__ value"
#endif

#if __SIZEOF_LONG__ == 4
#define ULONG_MAX	0xFFFFFFFFUL
#elif __SIZEOF_LONG__ == 8
#define ULONG_MAX	0xFFFFFFFFFFFFFFFFUL
#else
#error "unexpected __SIZEOF_LONG__ value"
#endif

#if __SIZEOF_LONG_LONG__ == 8
#define ULLONG_MAX	0xFFFFFFFFFFFFFFFFULL
#else
#error "unexpected __SIZEOF_LONG_LONG__ value"
#endif

#define PATH_MAX    256

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_LIMITS_H_ */
