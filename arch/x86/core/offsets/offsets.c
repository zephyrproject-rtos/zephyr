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

#include <gen_offset.h> /* located in kernel/include */

/* list of headers that define whose structure offsets will be generated */

#include <kernel_structs.h>
#include <swapstk.h>
#include <mmustructs.h>

#include <kernel_offsets.h>

#ifdef CONFIG_DEBUG_INFO
GEN_OFFSET_SYM(_kernel_arch_t, isf);
#endif

#ifdef CONFIG_GDB_INFO
GEN_OFFSET_SYM(_thread_arch_t, esf);
#endif

#if (defined(CONFIG_FP_SHARING) || defined(CONFIG_GDB_INFO))
GEN_OFFSET_SYM(_thread_arch_t, excNestCount);
#endif

GEN_OFFSET_SYM(_thread_arch_t, coopFloatReg);
GEN_OFFSET_SYM(_thread_arch_t, preempFloatReg);

/* size of the struct tcs structure sans save area for floating point regs */

GEN_ABSOLUTE_SYM(_K_THREAD_NO_FLOAT_SIZEOF,
		 sizeof(struct k_thread) - sizeof(tCoopFloatReg) -
			 sizeof(tPreempFloatReg));

GEN_OFFSET_SYM(_callee_saved_t, esp);

GEN_OFFSET_SYM(tSwapStk, eax);
GEN_OFFSET_SYM(tSwapStk, ebp);
GEN_OFFSET_SYM(tSwapStk, ebx);
GEN_OFFSET_SYM(tSwapStk, esi);
GEN_OFFSET_SYM(tSwapStk, edi);
GEN_OFFSET_SYM(tSwapStk, retAddr);
GEN_OFFSET_SYM(tSwapStk, param);

/* size of the entire tSwapStk structure */

GEN_ABSOLUTE_SYM(__tSwapStk_SIZEOF, sizeof(tSwapStk));

/* NANO_ESF structure member offsets */

GEN_OFFSET_SYM(NANO_ESF, esp);
GEN_OFFSET_SYM(NANO_ESF, ebp);
GEN_OFFSET_SYM(NANO_ESF, ebx);
GEN_OFFSET_SYM(NANO_ESF, esi);
GEN_OFFSET_SYM(NANO_ESF, edi);
GEN_OFFSET_SYM(NANO_ESF, edx);
GEN_OFFSET_SYM(NANO_ESF, ecx);
GEN_OFFSET_SYM(NANO_ESF, eax);
GEN_OFFSET_SYM(NANO_ESF, errorCode);
GEN_OFFSET_SYM(NANO_ESF, eip);
GEN_OFFSET_SYM(NANO_ESF, cs);
GEN_OFFSET_SYM(NANO_ESF, eflags);

/* tTaskStateSegment structure member offsets */


/* size of the ISR_LIST structure. Used by linker scripts */

GEN_ABSOLUTE_SYM(__ISR_LIST_SIZEOF, sizeof(ISR_LIST));
GEN_ABSOLUTE_SYM(__MMU_REGION_SIZEOF, sizeof(struct mmu_region));


GEN_ABS_SYM_END
