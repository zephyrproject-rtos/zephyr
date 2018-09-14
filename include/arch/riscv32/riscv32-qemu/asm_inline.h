/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV32_RISCV32_QEMU_ASM_INLINE_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV32_RISCV32_QEMU_ASM_INLINE_H_

/*
 * The file must not be included directly
 * Include arch/cpu.h instead
 */

#if defined(__GNUC__)
#include <arch/riscv32/riscv32-qemu/asm_inline_gcc.h>
#else
#error "Supports only GNU C compiler"
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV32_RISCV32_QEMU_ASM_INLINE_H_ */
