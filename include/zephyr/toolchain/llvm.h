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
#ifndef CONFIG_ENFORCE_ZEPHYR_STDINT

#define __INT8_C(x)	x
#define __UINT8_C(x)	x ## U
#define __INT16_C(x)	x
#define __UINT16_C(x)	x ## U
#define __INT32_C(x)	x
#define __UINT32_C(x)	x ## U
#define __INT64_C(x)	x
#define __UINT64_C(x)	x ## ULL

#endif /* !CONFIG_ENFORCE_ZEPHYR_STDINT */

#define __INTMAX_C(x)	x
#define __UINTMAX_C(x)	x ## ULL

#endif /* CONFIG_MINIMAL_LIBC */

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_LLVM_H_ */
