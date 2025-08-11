/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_LLVM_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_LLVM_H_

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_H_
#error Please do not include toolchain-specific headers directly, use <zephyr/toolchain.h> instead
#endif

#define __no_optimization __attribute__((optnone))

#ifndef __fallthrough
#if __clang_major__ >= 10
#define __fallthrough __attribute__((fallthrough))
#endif
#endif

#define TOOLCHAIN_CLANG_VERSION \
	((__clang_major__ * 10000) + (__clang_minor__ * 100) + \
	  __clang_patchlevel__)

#define TOOLCHAIN_HAS_PRAGMA_DIAG 1

#if TOOLCHAIN_CLANG_VERSION >= 30800
#define TOOLCHAIN_HAS_C_GENERIC 1
#define TOOLCHAIN_HAS_C_AUTO_TYPE 1
#endif

#include <zephyr/toolchain/gcc.h>

/* clear out common version. The build assert assert from gcc.h is defined to be empty */
#undef BUILD_ASSERT

#if defined(__cplusplus) && (__cplusplus >= 201103L)

/* C++11 has static_assert built in */
#define BUILD_ASSERT(EXPR, MSG...) static_assert(EXPR, "" MSG)

#elif !defined(__cplusplus) && ((__STDC_VERSION__) >= 201100)

/* C11 has static_assert built in */
#define BUILD_ASSERT(EXPR, MSG...) _Static_assert((EXPR), "" MSG)

#else

/* Rely on that the C-library provides a static assertion function */
#define BUILD_ASSERT(EXPR, MSG...) _Static_assert((EXPR), "" MSG)

#endif

#define TOOLCHAIN_WARNING_SIZEOF_ARRAY_DECAY            "-Wsizeof-array-decay"
#define TOOLCHAIN_WARNING_UNNEEDED_INTERNAL_DECLARATION "-Wunneeded-internal-declaration"

#define TOOLCHAIN_DISABLE_CLANG_WARNING(warning) _TOOLCHAIN_DISABLE_WARNING(clang, warning)
#define TOOLCHAIN_ENABLE_CLANG_WARNING(warning)  _TOOLCHAIN_ENABLE_WARNING(clang, warning)

/*
 * Provide these definitions only when minimal libc is used.
 * Avoid collision with defines from include/zephyr/toolchain/zephyr_stdint.h
 */
#ifdef CONFIG_MINIMAL_LIBC

/*
 * Predefined __INTN_C/__UINTN_C macros are provided by clang starting in version 20.1.
 * Avoid redefining these macros if a sufficiently modern clang is being used.
 */
#if TOOLCHAIN_CLANG_VERSION < 200100

#define __int_c(v, suffix) v ## suffix
#define int_c(v, suffix) __int_c(v, suffix)
#define uint_c(v, suffix) __int_c(v ## U, suffix)

#ifndef CONFIG_ENFORCE_ZEPHYR_STDINT

#ifdef __INT64_TYPE__
#undef __int_least64_c_suffix__
#undef __int_least32_c_suffix__
#undef __int_least16_c_suffix__
#undef __int_least8_c_suffix__
#ifdef __INT64_C_SUFFIX__
#define __int_least64_c_suffix__ __INT64_C_SUFFIX__
#define __int_least32_c_suffix__ __INT64_C_SUFFIX__
#define __int_least16_c_suffix__ __INT64_C_SUFFIX__
#define __int_least8_c_suffix__ __INT64_C_SUFFIX__
#endif /* __INT64_C_SUFFIX__ */
#endif /* __INT64_TYPE__ */

#ifdef __INT_LEAST64_TYPE__
#ifdef __int_least64_c_suffix__
#define __INT64_C(x)	int_c(x, __int_least64_c_suffix__)
#define __UINT64_C(x)	uint_c(x, __int_least64_c_suffix__)
#else
#define __INT64_C(x)	x
#define __UINT64_C(x)	x ## U
#endif /* __int_least64_c_suffix__ */
#endif /* __INT_LEAST64_TYPE__ */

#ifdef __INT32_TYPE__
#undef __int_least32_c_suffix__
#undef __int_least16_c_suffix__
#undef __int_least8_c_suffix__
#ifdef __INT32_C_SUFFIX__
#define __int_least32_c_suffix__ __INT32_C_SUFFIX__
#define __int_least16_c_suffix__ __INT32_C_SUFFIX__
#define __int_least8_c_suffix__ __INT32_C_SUFFIX__
#endif /* __INT32_C_SUFFIX__ */
#endif /* __INT32_TYPE__ */

#ifdef __INT_LEAST32_TYPE__
#ifdef __int_least32_c_suffix__
#define __INT32_C(x)	int_c(x, __int_least32_c_suffix__)
#define __UINT32_C(x)	uint_c(x, __int_least32_c_suffix__)
#else
#define __INT32_C(x)	x
#define __UINT32_C(x)	x ## U
#endif /* __int_least32_c_suffix__ */
#endif /* __INT_LEAST32_TYPE__ */

#endif /* !CONFIG_ENFORCE_ZEPHYR_STDINT */

#ifdef __INT16_TYPE__
#undef __int_least16_c_suffix__
#undef __int_least8_c_suffix__
#ifdef __INT16_C_SUFFIX__
#define __int_least16_c_suffix__ __INT16_C_SUFFIX__
#define __int_least8_c_suffix__ __INT16_C_SUFFIX__
#endif /* __INT16_C_SUFFIX__ */
#endif /* __INT16_TYPE__ */

#ifdef __INT_LEAST16_TYPE__
#ifdef __int_least16_c_suffix__
#define __INT16_C(x)	int_c(x, __int_least16_c_suffix__)
#define __UINT16_C(x)	uint_c(x, __int_least16_c_suffix__)
#else
#define __INT16_C(x)	x
#define __UINT16_C(x)	x ## U
#endif /* __int_least16_c_suffix__ */
#endif /* __INT_LEAST16_TYPE__ */

#ifdef __INT8_TYPE__
#undef __int_least8_c_suffix__
#ifdef __INT8_C_SUFFIX__
#define __int_least8_c_suffix__ __INT8_C_SUFFIX__
#endif /* __INT8_C_SUFFIX__ */
#endif /* __INT8_TYPE__ */

#ifdef __INT_LEAST8_TYPE__
#ifdef __int_least8_c_suffix__
#define __INT8_C(x)	int_c(x, __int_least8_c_suffix__)
#define __UINT8_C(x)	uint_c(x, __int_least8_c_suffix__)
#else
#define __INT8_C(x)	x
#define __UINT8_C(x)	x ## U
#endif /* __int_least8_c_suffix__ */
#endif /* __INT_LEAST8_TYPE__ */

#define __INTMAX_C(x)	int_c(x, __INTMAX_C_SUFFIX__)
#define __UINTMAX_C(x)	int_c(x, __UINTMAX_C_SUFFIX__)

#endif /* TOOLCHAIN_CLANG_VERSION < 200100 */

#endif /* CONFIG_MINIMAL_LIBC */

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_LLVM_H_ */
