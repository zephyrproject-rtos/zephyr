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

#include <kernel.h>
#include <kernel_arch_data.h>
#include <kernel_offsets.h>

GEN_OFFSET_SYM(_thread_arch_t, basepri);
GEN_OFFSET_SYM(_thread_arch_t, swap_return_value);

#if defined(CONFIG_USERSPACE) || defined(CONFIG_FPU_SHARING)
GEN_OFFSET_SYM(_thread_arch_t, mode);
#endif
#if defined(CONFIG_ARM_STORE_EXC_RETURN)
GEN_OFFSET_SYM(_thread_arch_t, mode_exc_return);
#endif
#if defined(CONFIG_USERSPACE)
GEN_OFFSET_SYM(_thread_arch_t, priv_stack_start);
#endif

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
GEN_OFFSET_SYM(_thread_arch_t, preempt_float);
#endif

GEN_OFFSET_SYM(_basic_sf_t, a1);
GEN_OFFSET_SYM(_basic_sf_t, a2);
GEN_OFFSET_SYM(_basic_sf_t, a3);
GEN_OFFSET_SYM(_basic_sf_t, a4);
GEN_OFFSET_SYM(_basic_sf_t, ip);
GEN_OFFSET_SYM(_basic_sf_t, lr);
GEN_OFFSET_SYM(_basic_sf_t, pc);
GEN_OFFSET_SYM(_basic_sf_t, xpsr);
GEN_ABSOLUTE_SYM(___basic_sf_t_SIZEOF, sizeof(_basic_sf_t));

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
GEN_OFFSET_SYM(_esf_t, s);
GEN_OFFSET_SYM(_esf_t, fpscr);
#endif

GEN_ABSOLUTE_SYM(___esf_t_SIZEOF, sizeof(_esf_t));

GEN_OFFSET_SYM(_callee_saved_t, v1);
GEN_OFFSET_SYM(_callee_saved_t, v2);
GEN_OFFSET_SYM(_callee_saved_t, v3);
GEN_OFFSET_SYM(_callee_saved_t, v4);
GEN_OFFSET_SYM(_callee_saved_t, v5);
GEN_OFFSET_SYM(_callee_saved_t, v6);
GEN_OFFSET_SYM(_callee_saved_t, v7);
GEN_OFFSET_SYM(_callee_saved_t, v8);
GEN_OFFSET_SYM(_callee_saved_t, psp);

/* size of the entire preempt registers structure */

GEN_ABSOLUTE_SYM(___callee_saved_t_SIZEOF, sizeof(struct _callee_saved));

#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
GEN_ABSOLUTE_SYM(___extra_esf_info_t_SIZEOF, sizeof(struct __extra_esf_info));
#endif

#if defined(CONFIG_THREAD_STACK_INFO)
GEN_OFFSET_SYM(_thread_stack_info_t, start);

GEN_ABSOLUTE_SYM(___thread_stack_info_t_SIZEOF,
	 sizeof(struct _thread_stack_info));
#endif

/*
 * size of the struct k_thread structure sans save area for floating
 * point registers.
 */

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
GEN_ABSOLUTE_SYM(_K_THREAD_NO_FLOAT_SIZEOF, sizeof(struct k_thread) -
					    sizeof(struct _preempt_float));
#else
GEN_ABSOLUTE_SYM(_K_THREAD_NO_FLOAT_SIZEOF, sizeof(struct k_thread));
#endif

#endif /* _ARM_OFFSETS_INC_ */
