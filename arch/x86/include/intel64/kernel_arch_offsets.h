/*
 * Copyright (c) 2023 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_OFFSETS_H_
#define ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_OFFSETS_H_

#include <zephyr/arch/x86/alignment.h>

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

/*
 * The following set of macros are used by assembly code to access
 * the _thread_arch_t structure.
 */

#define _thread_arch_flags_OFFSET    0x00
#define _thread_arch_rax_OFFSET      0x08
#define _thread_arch_rcx_OFFSET      0x10
#define _thread_arch_rdx_OFFSET      0x18
#define _thread_arch_rsi_OFFSET      0x20
#define _thread_arch_rdi_OFFSET      0x28
#define _thread_arch_r8_OFFSET       0x30
#define _thread_arch_r9_OFFSET       0x38
#define _thread_arch_r10_OFFSET      0x40
#define _thread_arch_r11_OFFSET      0x48
#define _thread_arch_sse_OFFSET      ((0x50 + X86_FXSAVE_ALIGN - 1) & \
				      ~(X86_FXSAVE_ALIGN - 1))

#ifdef CONFIG_USERSPACE
#define _thread_arch_psp_OFFSET      (_thread_arch_sse_OFFSET + 0x200)
#define _thread_arch_ss_OFFSET       (_thread_arch_psp_OFFSET + 0x08)
#define _thread_arch_cs_OFFSET       (_thread_arch_ss_OFFSET + 0x08)
#ifndef CONFIG_X86_COMMON_PAGE_TABLE
#define _thread_arch_ptables_OFFSET  (_thread_arch_cs_OFFSET + 0x08)
#endif
#endif /* CONFIG_USERSPACE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_OFFSETS_H_ */
