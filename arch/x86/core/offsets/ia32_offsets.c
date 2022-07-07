/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various IA-32 structures.
 *
 * All of the absolute symbols defined by this module will be present in the
 * final kernel ELF image (due to the linker's reference to the _OffsetAbsSyms
 * symbol).
 *
 * INTERNAL
 * It is NOT necessary to define the offset for every member of a structure.
 * Typically, only those members that are accessed by assembly language routines
 * are defined; however, it doesn't hurt to define all fields for the sake of
 * completeness.
 */

/* list of headers that define whose structure offsets will be generated */

#ifndef _X86_OFFSETS_INC_
#define _X86_OFFSETS_INC_

#include <zephyr/arch/x86/mmustructs.h>

#if defined(CONFIG_LAZY_FPU_SHARING)
GEN_OFFSET_SYM(_thread_arch_t, excNestCount);
#endif

#ifdef CONFIG_USERSPACE
GEN_OFFSET_SYM(_thread_arch_t, psp);
#ifndef CONFIG_X86_COMMON_PAGE_TABLE
GEN_OFFSET_SYM(_thread_arch_t, ptables);
#endif
#endif

GEN_OFFSET_SYM(_thread_arch_t, preempFloatReg);

/**
 * size of the struct k_thread structure sans save area for floating
 * point regs
 */

GEN_ABSOLUTE_SYM(_K_THREAD_NO_FLOAT_SIZEOF,
		 sizeof(struct k_thread) - sizeof(tPreempFloatReg));

GEN_OFFSET_SYM(_callee_saved_t, esp);

/* z_arch_esf_t structure member offsets */

GEN_OFFSET_SYM(z_arch_esf_t, esp);
GEN_OFFSET_SYM(z_arch_esf_t, ebp);
GEN_OFFSET_SYM(z_arch_esf_t, ebx);
GEN_OFFSET_SYM(z_arch_esf_t, esi);
GEN_OFFSET_SYM(z_arch_esf_t, edi);
GEN_OFFSET_SYM(z_arch_esf_t, edx);
GEN_OFFSET_SYM(z_arch_esf_t, ecx);
GEN_OFFSET_SYM(z_arch_esf_t, eax);
GEN_OFFSET_SYM(z_arch_esf_t, errorCode);
GEN_OFFSET_SYM(z_arch_esf_t, eip);
GEN_OFFSET_SYM(z_arch_esf_t, cs);
GEN_OFFSET_SYM(z_arch_esf_t, eflags);
#endif /* _X86_OFFSETS_INC_ */
