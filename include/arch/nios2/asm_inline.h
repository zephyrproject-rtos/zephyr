/*
 * Copyright (c) 2016 Intel Corporation
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
#include <arch/nios2/asm_inline_gcc.h>
#else
#include <arch/nios2/asm_inline_other.h>
#endif

#endif /* _ASM_INLINE_PUBLIC_H */
