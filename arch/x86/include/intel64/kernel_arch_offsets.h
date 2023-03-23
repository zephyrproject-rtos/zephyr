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

/*
 * The following set of macros are used by assembly code to access
 * the x86_cpuboot_t structure.
 */

#define __x86_cpuboot_t_tr_OFFSET         0x4
#define __x86_cpuboot_t_gs_base_OFFSET    0x8
#define __x86_cpuboot_t_sp_OFFSET         0x10
#define __x86_cpuboot_t_stack_size_OFFSET 0x18
#define __X86_CPUBOOT_SIZEOF              0x30

/*
 * The following set of macros are used by assembly code to access
 * the x86_boot_arg_t structure.
 */

#define __x86_boot_arg_t_boot_type_OFFSET 0x0
#define __x86_boot_arg_t_arg_OFFSET       0x8

/*
 * The following set of macros are used by assembly code to access
 * the _callee_saved_t  structure.
 */

#define _callee_saved_rsp_OFFSET     0x00
#define _callee_saved_rbx_OFFSET     0x08
#define _callee_saved_rbp_OFFSET     0x10
#define _callee_saved_r12_OFFSET     0x18
#define _callee_saved_r13_OFFSET     0x20
#define _callee_saved_r14_OFFSET     0x28
#define _callee_saved_r15_OFFSET     0x30
#define _callee_saved_rip_OFFSET     0x38
#define _callee_saved_rflags_OFFSET  0x40

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_OFFSETS_H_ */
