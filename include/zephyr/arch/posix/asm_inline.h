/* POSIX inline "assembler" functions and macros for public functions */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_POSIX_ASM_INLINE_H_
#define ZEPHYR_INCLUDE_ARCH_POSIX_ASM_INLINE_H_

/*
 * The file must not be included directly
 * Include kernel.h instead
 */

#if defined(__GNUC__)
#include <arch/posix/asm_inline_gcc.h>
#else
#error "Only a compiler with GNU C extensions is supported for the POSIX arch"
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_POSIX_ASM_INLINE_H_ */
