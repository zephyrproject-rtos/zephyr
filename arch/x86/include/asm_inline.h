/* Inline assembler kernel functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_ASM_INLINE_H_
#define ZEPHYR_ARCH_X86_INCLUDE_ASM_INLINE_H_

#if !defined(CONFIG_X86)
#error The arch/x86/include/asm_inline.h is only for x86 architecture
#endif

#if defined(__GNUC__)
#include <asm_inline_gcc.h>
#else
#include <asm_inline_other.h>
#endif /* __GNUC__ */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_ASM_INLINE_H_ */
