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

/*
 * The following set of macros are used by assembly code to access
 * the _thread_arch_t structure.
 */

#define _thread_arch_flags_OFFSET       0x00

#ifdef CONFIG_USERSPACE
#define _thread_arch_psp_OFFSET         0x04

#ifndef CONFIG_X86_COMMON_PAGE_TABLE
#define _thread_arch_ptables_OFFSET     0x08
#define _thread_arch_userspace_SIZE     0x08
#else
#define _thread_arch_userspace_SIZE     0x04
#endif  /* CONFIG_X86_COMMON_PAGE_TABLE */

#else
#define _thread_arch_userspace_SIZE     0x00
#endif  /* CONFIG_USERSPACE */

#ifdef CONFIG_LAZY_FPU_SHARING
#define _thread_arch_excNestCount_OFFSET   (0x04 + _thread_arch_userspace_SIZE)
#define _thread_arch_fpu_sharing_SIZE   0x04
#else
#define _thread_arch_fpu_sharing_SIZE   0x00
#endif  /* CONFIG_LAZY_FPU_SHARING */

#define _thread_arch_preempFloatReg_OFFSET  ((0x04 +                          \
					      _thread_arch_userspace_SIZE +   \
					      _thread_arch_fpu_sharing_SIZE + \
					      FP_REG_SET_ALIGN - 1) &         \
					     ~(FP_REG_SET_ALIGN - 1))

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_OFFSETS_H_ */
