/* Intel x86 inline assembler functions and macros for public functions */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ASM_INLINE_PUBLIC_H
#define _ASM_INLINE_PUBLIC_H

/*
 * The file must not be included directly
 * Include kernel.h instead
 */

#if defined(__GNUC__)
#include <arch/x86/asm_inline_gcc.h>
#else
#include <arch/x86/asm_inline_other.h>
#endif

#endif /* _ASM_INLINE_PUBLIC_H */
