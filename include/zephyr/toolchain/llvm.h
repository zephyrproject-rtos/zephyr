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

#if __clang_major__ >= 10
#define __fallthrough __attribute__((fallthrough))
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

/*
 * Provide these definitions only when minimal libc is used.
 * Avoid collision with defines from include/zephyr/toolchain/zephyr_stdint.h
 */
#ifdef CONFIG_MINIMAL_LIBC

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

#endif /* CONFIG_MINIMAL_LIBC */

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_LLVM_H_ */
