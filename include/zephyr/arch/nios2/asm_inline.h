/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_NIOS2_ASM_INLINE_H_
#define ZEPHYR_INCLUDE_ARCH_NIOS2_ASM_INLINE_H_

/*
 * The file must not be included directly
 * Include kernel.h instead
 */

#if defined(__GNUC__)
#include <arch/nios2/asm_inline_gcc.h>
#else
#include <arch/nios2/asm_inline_other.h>
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_NIOS2_ASM_INLINE_H_ */
