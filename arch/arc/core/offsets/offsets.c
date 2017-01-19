/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various ARCv2 kernel structures.
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

#include <gen_offset.h>
#include <kernel_structs.h>
#include <kernel_offsets.h>

GEN_OFFSET_SYM(_thread_arch_t, intlock_key);
GEN_OFFSET_SYM(_thread_arch_t, relinquish_cause);
GEN_OFFSET_SYM(_thread_arch_t, return_value);
#ifdef CONFIG_ARC_STACK_CHECKING
GEN_OFFSET_SYM(_thread_arch_t, stack_top);
#endif

/* ARCv2-specific IRQ stack frame structure member offsets */
GEN_OFFSET_SYM(_isf_t, r0);
GEN_OFFSET_SYM(_isf_t, r1);
GEN_OFFSET_SYM(_isf_t, r2);
GEN_OFFSET_SYM(_isf_t, r3);
GEN_OFFSET_SYM(_isf_t, r4);
GEN_OFFSET_SYM(_isf_t, r5);
GEN_OFFSET_SYM(_isf_t, r6);
GEN_OFFSET_SYM(_isf_t, r7);
GEN_OFFSET_SYM(_isf_t, r8);
GEN_OFFSET_SYM(_isf_t, r9);
GEN_OFFSET_SYM(_isf_t, r10);
GEN_OFFSET_SYM(_isf_t, r11);
GEN_OFFSET_SYM(_isf_t, r12);
GEN_OFFSET_SYM(_isf_t, r13);
GEN_OFFSET_SYM(_isf_t, blink);
GEN_OFFSET_SYM(_isf_t, lp_end);
GEN_OFFSET_SYM(_isf_t, lp_start);
GEN_OFFSET_SYM(_isf_t, lp_count);
GEN_OFFSET_SYM(_isf_t, pc);
GEN_OFFSET_SYM(_isf_t, status32);
GEN_ABSOLUTE_SYM(___isf_t_SIZEOF, sizeof(_isf_t));

GEN_OFFSET_SYM(_callee_saved_t, sp);
GEN_ABSOLUTE_SYM(___callee_saved_t_SIZEOF, sizeof(_callee_saved_t));

GEN_OFFSET_SYM(_callee_saved_stack_t, r13);
GEN_OFFSET_SYM(_callee_saved_stack_t, r14);
GEN_OFFSET_SYM(_callee_saved_stack_t, r15);
GEN_OFFSET_SYM(_callee_saved_stack_t, r16);
GEN_OFFSET_SYM(_callee_saved_stack_t, r17);
GEN_OFFSET_SYM(_callee_saved_stack_t, r18);
GEN_OFFSET_SYM(_callee_saved_stack_t, r19);
GEN_OFFSET_SYM(_callee_saved_stack_t, r20);
GEN_OFFSET_SYM(_callee_saved_stack_t, r21);
GEN_OFFSET_SYM(_callee_saved_stack_t, r22);
GEN_OFFSET_SYM(_callee_saved_stack_t, r23);
GEN_OFFSET_SYM(_callee_saved_stack_t, r24);
GEN_OFFSET_SYM(_callee_saved_stack_t, r25);
GEN_OFFSET_SYM(_callee_saved_stack_t, r26);
GEN_OFFSET_SYM(_callee_saved_stack_t, fp);
GEN_OFFSET_SYM(_callee_saved_stack_t, r30);
GEN_ABSOLUTE_SYM(___callee_saved_stack_t_SIZEOF, sizeof(_callee_saved_stack_t));

GEN_ABSOLUTE_SYM(_K_THREAD_NO_FLOAT_SIZEOF, sizeof(struct k_thread));

GEN_ABS_SYM_END
