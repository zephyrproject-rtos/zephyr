/* stdint.h */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_stdint_h__
#define __INC_stdint_h__

#ifdef __cplusplus
extern "C" {
#endif

#define INT8_MAX    0x7F
#define INT16_MAX   0x7FFF
#define INT32_MAX   0x7FFFFFFF
#define INT64_MAX   0x7FFFFFFFFFFFFFFFll

#define INT8_MIN    (-INT8_MAX - 1)
#define INT16_MIN   (-INT16_MAX - 1)
#define INT32_MIN   (-INT32_MAX - 1)
#define INT64_MIN   (-INT64_MAX - 1ll)

#define UINT8_MAX   0xFF
#define UINT16_MAX  0xFFFF
#define UINT32_MAX  0xFFFFFFFFu
#define UINT64_MAX  0xFFFFFFFFFFFFFFFFull

#define INTPTR_MIN  INT32_MIN
#define INTPTR_MAX  INT32_MAX
#define UINTPTR_MAX UINT32_MAX

#define PTRDIFF_MIN INT32_MIN
#define PTRDIFF_MAX INT32_MAX

#define SIZE_MAX    UINT32_MAX

typedef __INT8_TYPE__       int8_t;
typedef __INT16_TYPE__      int16_t;
typedef __INT32_TYPE__      int32_t;
typedef __INT64_TYPE__      int64_t;

/* Assume int to be the fastest type for all types except 64bit ones */

typedef __INT_FAST8_TYPE__	int_fast8_t;
typedef __INT_FAST16_TYPE__	int_fast16_t;
typedef __INT_FAST32_TYPE__	int_fast32_t;
typedef __INT_FAST64_TYPE__	int_fast64_t;

typedef __INT_LEAST8_TYPE__	int_least8_t;
typedef __INT_LEAST16_TYPE__	int_least16_t;
typedef __INT_LEAST32_TYPE__	int_least32_t;
typedef __INT_LEAST64_TYPE__	int_least64_t;

typedef __UINT8_TYPE__      uint8_t;
typedef __UINT16_TYPE__     uint16_t;
typedef __UINT32_TYPE__     uint32_t;
typedef __UINT64_TYPE__     uint64_t;

typedef __UINT_FAST8_TYPE__	uint_fast8_t;
typedef __UINT_FAST16_TYPE__	uint_fast16_t;
typedef __UINT_FAST32_TYPE__	uint_fast32_t;
typedef __UINT_FAST64_TYPE__	uint_fast64_t;

typedef __UINT_LEAST8_TYPE__	uint_least8_t;
typedef __UINT_LEAST16_TYPE__	uint_least16_t;
typedef __UINT_LEAST32_TYPE__	uint_least32_t;
typedef __UINT_LEAST64_TYPE__	uint_least64_t;

typedef __INTPTR_TYPE__         intptr_t;
typedef __UINTPTR_TYPE__        uintptr_t;

#ifdef __cplusplus
}
#endif

#endif /* __INC_stdint_h__ */
