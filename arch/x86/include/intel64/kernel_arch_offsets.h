/*
 * Copyright (c) 2023 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_OFFSETS_H_
#define ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_OFFSETS_H_

/*
 * The following set of macros are used by assembly code to access
 * the x86_tss64_t structure.
 */

#define __x86_tss64_t_ist1_OFFSET  0x24
#define __x86_tss64_t_ist2_OFFSET  0x2c
#define __x86_tss64_t_ist6_OFFSET  0x4c
#define __x86_tss64_t_ist7_OFFSET  0x54
#define __x86_tss64_t_cpu_OFFSET   0x68

#ifdef CONFIG_USERSPACE
#define __x86_tss64_t_psp_OFFSET   0x70
#define __x86_tss64_t_usp_OFFSET   0x78
#define __X86_TSS64_SIZEOF         0x80
#else
#define __X86_TSS64_SIZEOF         0x70
#endif

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_OFFSETS_H_ */
