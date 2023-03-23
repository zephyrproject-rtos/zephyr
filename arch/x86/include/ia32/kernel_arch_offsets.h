/*
 * Copyright (c) 2023 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_IA32_KERNEL_ARCH_OFFSETS_H_
#define ZEPHYR_ARCH_X86_INCLUDE_IA32_KERNEL_ARCH_OFFSETS_H_

/*
 * The following set of macros are used by assembly code to access
 * the x86_boot_arg_t structure.
 */

#define __x86_boot_arg_t_boot_type_OFFSET 0x0
#define __x86_boot_arg_t_arg_OFFSET       0x4

#ifdef CONFIG_GDBSTUB
#define __z_arch_esf_t_eflags_OFFSET    0x30
#else
#define __z_arch_esf_t_eflags_OFFSET    0x2c
#endif

/*
 * The following set of macros are used by assembly code to access
 * the _callee_saved_t structure.
 */

#define _callee_saved_esp_OFFSET        0x00

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_OFFSETS_H_ */
