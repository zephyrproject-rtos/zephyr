/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various ARM kernel structures.
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

#ifndef _ARM_OFFSETS_INC_
#define _ARM_OFFSETS_INC_

#include <zephyr/kernel.h>
#include <kernel_arch_data.h>
#include <kernel_offsets.h>

GEN_OFFSET_SYM(_thread_arch_t, basepri);
GEN_OFFSET_SYM(_thread_arch_t, swap_return_value);

#if defined(CONFIG_CPU_AARCH32_CORTEX_A) || defined(CONFIG_CPU_AARCH32_CORTEX_R)
GEN_OFFSET_SYM(_thread_arch_t, exception_depth);
GEN_OFFSET_SYM(_cpu_arch_t, exc_depth);
#endif

#if defined(CONFIG_ARM_STORE_EXC_RETURN) || defined(CONFIG_USERSPACE)
GEN_OFFSET_SYM(_thread_arch_t, mode);
#endif
#if defined(CONFIG_ARM_STORE_EXC_RETURN)
GEN_OFFSET_SYM(_thread_arch_t, mode_exc_return);
#endif
#if defined(CONFIG_USERSPACE)
GEN_OFFSET_SYM(_thread_arch_t, priv_stack_start);
#if defined(CONFIG_CPU_AARCH32_CORTEX_R)
GEN_OFFSET_SYM(_thread_arch_t, priv_stack_end);
GEN_OFFSET_SYM(_thread_arch_t, sp_usr);
#endif
#endif

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
GEN_OFFSET_SYM(_thread_arch_t, preempt_float);
#endif

GEN_OFFSET_SYM(_basic_sf_t, pc);
GEN_OFFSET_SYM(_basic_sf_t, xpsr);

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
GEN_OFFSET_SYM(_fpu_sf_t, fpscr);

GEN_ABSOLUTE_SYM(___fpu_t_SIZEOF, sizeof(_fpu_sf_t));
#endif

GEN_ABSOLUTE_SYM(___esf_t_SIZEOF, sizeof(_esf_t));

/* size of the entire preempt registers structure */

GEN_ABSOLUTE_SYM(___callee_saved_t_SIZEOF, sizeof(struct _callee_saved));

#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
GEN_ABSOLUTE_SYM(___extra_esf_info_t_SIZEOF, sizeof(struct __extra_esf_info));
#endif

#if defined(CONFIG_THREAD_STACK_INFO)
GEN_OFFSET_SYM(_thread_stack_info_t, start);
#endif

/*
 * CPU context for S2RAM
 */
#if defined(CONFIG_PM_S2RAM)
GEN_OFFSET_SYM(_cpu_context_t, msp);
GEN_OFFSET_SYM(_cpu_context_t, psp);
GEN_OFFSET_SYM(_cpu_context_t, primask);
GEN_OFFSET_SYM(_cpu_context_t, control);

#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
/* Registers present only on ARMv7-M and ARMv8-M Mainline */
GEN_OFFSET_SYM(_cpu_context_t, faultmask);
GEN_OFFSET_SYM(_cpu_context_t, basepri);
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */

#if defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
/* Registers present only on certain ARMv8-M implementations */
GEN_OFFSET_SYM(_cpu_context_t, msplim);
GEN_OFFSET_SYM(_cpu_context_t, psplim);
#endif /* CONFIG_CPU_CORTEX_M_HAS_SPLIM */
#endif /* CONFIG_PM_S2RAM */

#endif /* _ARM_OFFSETS_INC_ */
