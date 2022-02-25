/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_LLVM_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_LLVM_H_


#define __no_optimization __attribute__((optnone))

#if __clang_major__ >= 10
#define __fallthrough __attribute__((fallthrough))
#endif

#define CLANG_VERSION \
	((__clang_major__ * 10000) + (__clang_minor__ * 100) + \
	  __clang_patchlevel__)

#include <toolchain/gcc.h>


#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_LLVM_H_ */
