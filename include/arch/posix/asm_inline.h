/* POSIX inline "assembler" functions and macros for public functions */

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
#include <arch/posix/asm_inline_gcc.h>
#else
#error "Only a compiler with GNU C extensions is supported for the POSIX arch"
#endif

#endif /* _ASM_INLINE_PUBLIC_H */
