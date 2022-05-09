/* asm_inline.h - ARC inline assembler and macros for public functions */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_ASM_INLINE_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_ASM_INLINE_H_

/*
 * The file must not be included directly
 * Include kernel.h instead
 */

#if defined(__GNUC__)
#include <zephyr/arch/arc/v2/asm_inline_gcc.h>
#else
#erro "you need to provide an asm_inline.h for your compiler"
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_ASM_INLINE_H_ */
