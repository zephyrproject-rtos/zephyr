/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_RISCV_PRIVILEGE_ASM_INLINE_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_RISCV_PRIVILEGE_ASM_INLINE_H_

/*
 * The file must not be included directly
 * Include arch/cpu.h instead
 */

#if defined(__GNUC__)
#include <arch/riscv/riscv-privilege/asm_inline_gcc.h>
#else
#error "Supports only GNU C compiler"
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_RISCV_PRIVILEGE_ASM_INLINE_H_ */
