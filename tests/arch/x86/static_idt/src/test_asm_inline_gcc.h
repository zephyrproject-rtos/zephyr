/* Intel x86 GCC specific test inline assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEST_ASM_INLINE_GCC_H
#define _TEST_ASM_INLINE_GCC_H

#if !defined(__GNUC__) || !defined(CONFIG_X86)
#error test_asm_inline_gcc.h goes only with x86 GCC
#endif

#define _trigger_isr_handler() __asm__ volatile("int %0" : : "i" (TEST_SOFT_INT) : "memory")
#define _trigger_spur_handler() __asm__ volatile("int %0" : : "i" (TEST_SPUR_INT) : "memory")

#endif /* _TEST_ASM_INLINE_GCC_H */
