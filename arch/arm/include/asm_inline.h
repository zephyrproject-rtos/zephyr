/* Inline assembler kernel functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ASM_INLINE_H
#define _ASM_INLINE_H

#if !defined(CONFIG_ARM) || !defined(CONFIG_CPU_CORTEX_M)
#error arch/arm/include/asm_inline.h is for ARM Cortex-M only
#endif

#if defined(__GNUC__)
#include <cortex_m/asm_inline_gcc.h>
#else
#include <cortex_m/asm_inline_other.h>
#endif /* __GNUC__ */

#endif /* _ASM_INLINE_H */
