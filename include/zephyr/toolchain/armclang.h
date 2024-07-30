/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_ARMCLANG_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_ARMCLANG_H_

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_H_
#error Please do not include toolchain-specific headers directly, use <zephyr/toolchain.h> instead
#endif

#include <zephyr/toolchain/llvm.h>

/*
 * To reuse as much as possible from the llvm.h header we only redefine the
 * __GENERIC_SECTION and Z_GENERIC_SECTION macros here to include the `used` keyword.
 */
#undef __GENERIC_SECTION
#undef Z_GENERIC_SECTION

#define __GENERIC_SECTION(segment) __attribute__((section(STRINGIFY(segment)), used))
#define Z_GENERIC_SECTION(segment) __GENERIC_SECTION(segment)

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_ARMCLANG_H_ */
