/*
 * Copyright (c) 2019 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Basic macro definitions that gcc and clang provide on their own
 * but that xcc lacks. Only those that Zephyr requires are provided here.
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_XCC_MISSING_DEFS_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_XCC_MISSING_DEFS_H_

#if __CHAR_BIT__ == 8
#define __SCHAR_WIDTH__ 8
#else
#error "unexpected __CHAR_BIT__ value"
#endif

#if __SHRT_MAX__ == 32767
#define __SHRT_WIDTH__ 16
#define __SIZEOF_SHORT__ 2
#else
#error "unexpected __SHRT_WIDTH__ value"
#endif

#if __INT_MAX__ == 2147483647
#define __INT_WIDTH__ 32
#define __SIZEOF_INT__ 4
#else
#error "unexpected __INT_MAX__ value"
#endif

#if __LONG_MAX__ == 2147483647L
#define __LONG_WIDTH__ 32
#define __SIZEOF_LONG__ 4
#else
#error "unexpected __LONG_MAX__ value"
#endif

#if __LONG_LONG_MAX__ == 9223372036854775807LL
#define __LONG_LONG_WIDTH__ 64
#define __SIZEOF_LONG_LONG__ 8
#else
#error "unexpected __LONG_LONG_MAX__ value"
#endif

#if __INTMAX_MAX__ == 9223372036854775807LL
#define __INTMAX_WIDTH__ 64
#define __SIZEOF_INTMAX__ 8
#define __UINTMAX_MAX__ 0xffffffffffffffffULL
#define __UINTMAX_WIDTH__ 64
#define __SIZEOF_UINTMAX__ 8
#else
#error "unexpected __INTMAX_MAX__ value"
#endif

/*
 * No xcc provided definitions related to pointers, so let's just enforce
 * the Zephyr expected type.
 */

#define __INTPTR_MAX__ 0x7fffffffL
#define __INTPTR_TYPE__ long int
#define __INTPTR_WIDTH__ 32
#define __SIZEOF_POINTER__ 4

#define __PTRDIFF_MAX__ 0x7fffffffL
#define __PTRDIFF_WIDTH__ 32
#define __SIZEOF_PTRDIFF_T__ 4

#define __UINTPTR_MAX__ 0xffffffffLU
#define __UINTPTR_TYPE__ long unsigned int

/*
 * xcc already defines __SIZE_TYPE__ as "unsigned int" but there is no way
 * to safeguard that here with preprocessor equality.
 */

#define __SIZE_MAX__ 0xffffffffU
#define __SIZE_WIDTH__ 32
#define __SIZEOF_SIZE_T__ 4

/*
 * The following defines are inferred from the xcc provided defines
 * already tested above.
 */

#define __INT8_MAX__ 0x7f
#define __INT8_TYPE__ signed char

#define __INT16_MAX__ 0x7fff
#define __INT16_TYPE__ short int

#define __INT32_MAX__ 0x7fffffff
#define __INT32_TYPE__ int

#define __INT64_MAX__ 0x7fffffffffffffffLL
#define __INT64_TYPE__ long long int

#define __INT_FAST8_MAX__ 0x7f
#define __INT_FAST8_TYPE__ signed char
#define __INT_FAST8_WIDTH__ 8

#define __INT_FAST16_MAX__ 0x7fffffff
#define __INT_FAST16_TYPE__ int
#define __INT_FAST16_WIDTH__ 32

#define __INT_FAST32_MAX__ 0x7fffffff
#define __INT_FAST32_TYPE__ int
#define __INT_FAST32_WIDTH__ 32

#define __INT_FAST64_MAX__ 0x7fffffffffffffffLL
#define __INT_FAST64_TYPE__ long long int
#define __INT_FAST64_WIDTH__ 64

#define __INT_LEAST8_MAX__ 0x7f
#define __INT_LEAST8_TYPE__ signed char
#define __INT_LEAST8_WIDTH__ 8

#define __INT_LEAST16_MAX__ 0x7fff
#define __INT_LEAST16_TYPE__ short int
#define __INT_LEAST16_WIDTH__ 16

#define __INT_LEAST32_MAX__ 0x7fffffff
#define __INT_LEAST32_TYPE__ int
#define __INT_LEAST32_WIDTH__ 32

#define __INT_LEAST64_MAX__ 0x7fffffffffffffffLL
#define __INT_LEAST64_TYPE__ long long int
#define __INT_LEAST64_WIDTH__ 64

#define __UINT8_MAX__ 0xffU
#define __UINT8_TYPE__ unsigned char

#define __UINT16_MAX__ 0xffffU
#define __UINT16_TYPE__ short unsigned int

#define __UINT32_MAX__ 0xffffffffU
#define __UINT32_TYPE__ unsigned int

#define __UINT64_MAX__ 0xffffffffffffffffULL
#define __UINT64_TYPE__ long long unsigned int

#define __UINT_FAST8_MAX__ 0xffU
#define __UINT_FAST8_TYPE__ unsigned char

#define __UINT_FAST16_MAX__ 0xffffffffU
#define __UINT_FAST16_TYPE__ unsigned int

#define __UINT_FAST32_MAX__ 0xffffffffU
#define __UINT_FAST32_TYPE__ unsigned int

#define __UINT_FAST64_MAX__ 0xffffffffffffffffULL
#define __UINT_FAST64_TYPE__ long long unsigned int

#define __UINT_LEAST8_MAX__ 0xffU
#define __UINT_LEAST8_TYPE__ unsigned char

#define __UINT_LEAST16_MAX__ 0xffffU
#define __UINT_LEAST16_TYPE__ short unsigned int

#define __UINT_LEAST32_MAX__ 0xffffffffU
#define __UINT_LEAST32_TYPE__ unsigned int

#define __UINT_LEAST64_MAX__ 0xffffffffffffffffULL
#define __UINT_LEAST64_TYPE__ long long unsigned int

#endif
