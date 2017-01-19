/* asm_inline.h - ARC inline assembler and macros for public functions */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ASM_INLINE_H__
#define __ASM_INLINE_H__

/*
 * The file must not be included directly
 * Include kernel.h instead
 */

#if defined(__GNUC__)
#include <arch/arc/v2/asm_inline_gcc.h>
#else
#erro "you need to provide an asm_inline.h for your compiler"
#endif

#endif /* __ASM_INLINE_H__ */
