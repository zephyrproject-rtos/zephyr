/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xtensa kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various Xtensa kernel
 * structures.
 *
 * All of the absolute symbols defined by this module will be present in the
 * final kernel or kernel ELF image (due to the linker's reference to
 * the _OffsetAbsSyms symbol).
 *
 * INTERNAL
 * It is NOT necessary to define the offset for every member of a structure.
 * Typically, only those members that are accessed by assembly language routines
 * are defined; however, it doesn't hurt to define all fields for the sake of
 * completeness.
 */

#include <gen_offset.h> /* located in kernel/arch/common/include */

/* list of headers that define whose structure offsets will be generated */

#include <kernel_structs.h>

#include <kernel_offsets.h>

/* Xtensa-specific k_thread structure member offsets */
GEN_OFFSET_SYM(_callee_saved_t, topOfStack);
GEN_OFFSET_SYM(_callee_saved_t, retval);

GEN_OFFSET_SYM(_thread_arch_t, preempCoprocReg);
#if XCHAL_CP_NUM > 0
GEN_OFFSET_SYM(tPreempCoprocReg, cpStack);
#endif

/* Xtensa-specific _thread_arch_t structure member offsets */
GEN_OFFSET_SYM(_thread_arch_t, flags);

/* Xtensa-specific ESF structure member offsets */
GEN_OFFSET_SYM(__esf_t, sp);
GEN_OFFSET_SYM(__esf_t, pc);

/* size of the entire __esf_t structure */
GEN_ABSOLUTE_SYM(____esf_t_SIZEOF, sizeof(__esf_t));

/* size of the struct k_thread structure without save area for coproc regs */
GEN_ABSOLUTE_SYM(_K_THREAD_NO_FLOAT_SIZEOF,
		 sizeof(struct k_thread) - sizeof(tCoopCoprocReg) -
			 sizeof(tPreempCoprocReg) + XT_CP_DESCR_SIZE);

GEN_ABS_SYM_END
