/* stdint.h */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDINT_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDINT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define INT8_MAX    __INT8_MAX__
#define INT16_MAX   __INT16_MAX__
#define INT32_MAX   __INT32_MAX__
#define INT64_MAX   __INT64_MAX__
#define INTMAX_MAX  __INT64_MAX__

#define INT8_MIN    (-INT8_MAX - 1)
#define INT16_MIN   (-INT16_MAX - 1)
#define INT32_MIN   (-INT32_MAX - 1)
#define INT64_MIN   (-INT64_MAX - 1LL)

#define UINT8_MAX   __UINT8_MAX__
#define UINT16_MAX  __UINT16_MAX__
#define UINT32_MAX  __UINT32_MAX__
#define UINT64_MAX  __UINT64_MAX__
#define UINTMAX_MAX __UINT64_MAX__

#define INTPTR_MAX  __INTPTR_MAX__
#define INTPTR_MIN  (-INTPTR_MAX - 1)
#define UINTPTR_MAX __UINTPTR_MAX__

#define PTRDIFF_MAX __PTRDIFF_MAX__
#define PTRDIFF_MIN (-PTRDIFF_MAX - 1)

#define SIZE_MAX    __SIZE_MAX__

typedef __INT8_TYPE__		int8_t;
typedef __INT16_TYPE__		int16_t;
typedef __INT32_TYPE__		int32_t;
typedef __INT64_TYPE__		int64_t;
typedef __INT64_TYPE__		intmax_t;

typedef __INT_FAST8_TYPE__	int_fast8_t;
typedef __INT_FAST16_TYPE__	int_fast16_t;
typedef __INT_FAST32_TYPE__	int_fast32_t;
typedef __INT_FAST64_TYPE__	int_fast64_t;

typedef __INT_LEAST8_TYPE__	int_least8_t;
typedef __INT_LEAST16_TYPE__	int_least16_t;
typedef __INT_LEAST32_TYPE__	int_least32_t;
typedef __INT_LEAST64_TYPE__	int_least64_t;

typedef __UINT8_TYPE__		uint8_t;
typedef __UINT16_TYPE__		uint16_t;
typedef __UINT32_TYPE__		uint32_t;
typedef __UINT64_TYPE__		uint64_t;
typedef __UINT64_TYPE__		uintmax_t;

typedef __UINT_FAST8_TYPE__	uint_fast8_t;
typedef __UINT_FAST16_TYPE__	uint_fast16_t;
typedef __UINT_FAST32_TYPE__	uint_fast32_t;
typedef __UINT_FAST64_TYPE__	uint_fast64_t;

typedef __UINT_LEAST8_TYPE__	uint_least8_t;
typedef __UINT_LEAST16_TYPE__	uint_least16_t;
typedef __UINT_LEAST32_TYPE__	uint_least32_t;
typedef __UINT_LEAST64_TYPE__	uint_least64_t;

typedef __INTPTR_TYPE__		intptr_t;
typedef __UINTPTR_TYPE__	uintptr_t;

#ifdef __GNUC__
/* These macros must produce constant integer expressions, which can't
 * be done in the preprocessor (casts aren't allowed).  Defer to the
 * GCC internal functions where they're available.
 */
#define INT8_C(_v) __INT8_C(_v)
#define INT16_C(_v) __INT16_C(_v)
#define INT32_C(_v) __INT32_C(_v)
#define INT64_C(_v) __INT64_C(_v)
#define INTMAX_C(_v) __INTMAX_C(_v)

#define UINT8_C(_v) __UINT8_C(_v)
#define UINT16_C(_v) __UINT16_C(_v)
#define UINT32_C(_v) __UINT32_C(_v)
#define UINT64_C(_v) __UINT64_C(_v)
#define UINTMAX_C(_v) __UINTMAX_C(_v)
#endif /* __GNUC__ */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDINT_H_ */
