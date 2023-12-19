/* ARM AArch32 inline assembler functions and macros for public functions */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_ASM_INLINE_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_ASM_INLINE_H_

/*
 * The file must not be included directly
 * Include kernel.h instead
 */

#if defined(__GNUC__)
#include <zephyr/arch/arm/asm_inline_gcc.h>
#else
#include <arch/arm/asm_inline_other.h>
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_ASM_INLINE_H_ */
