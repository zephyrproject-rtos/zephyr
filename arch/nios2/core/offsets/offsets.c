/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Nios II kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various Nios II kernel
 * structures.
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


#include <zephyr/kernel.h>
#include <kernel_arch_data.h>
#include <gen_offset.h>
#include <kernel_offsets.h>

/* struct coop member offsets */
GEN_OFFSET_SYM(_callee_saved_t, r16);
GEN_OFFSET_SYM(_callee_saved_t, r17);
GEN_OFFSET_SYM(_callee_saved_t, r18);
GEN_OFFSET_SYM(_callee_saved_t, r19);
GEN_OFFSET_SYM(_callee_saved_t, r20);
GEN_OFFSET_SYM(_callee_saved_t, r21);
GEN_OFFSET_SYM(_callee_saved_t, r22);
GEN_OFFSET_SYM(_callee_saved_t, r23);
GEN_OFFSET_SYM(_callee_saved_t, r28);
GEN_OFFSET_SYM(_callee_saved_t, ra);
GEN_OFFSET_SYM(_callee_saved_t, sp);
GEN_OFFSET_SYM(_callee_saved_t, key);
GEN_OFFSET_SYM(_callee_saved_t, retval);

GEN_OFFSET_SYM(z_arch_esf_t, ra);
GEN_OFFSET_SYM(z_arch_esf_t, r1);
GEN_OFFSET_SYM(z_arch_esf_t, r2);
GEN_OFFSET_SYM(z_arch_esf_t, r3);
GEN_OFFSET_SYM(z_arch_esf_t, r4);
GEN_OFFSET_SYM(z_arch_esf_t, r5);
GEN_OFFSET_SYM(z_arch_esf_t, r6);
GEN_OFFSET_SYM(z_arch_esf_t, r7);
GEN_OFFSET_SYM(z_arch_esf_t, r8);
GEN_OFFSET_SYM(z_arch_esf_t, r9);
GEN_OFFSET_SYM(z_arch_esf_t, r10);
GEN_OFFSET_SYM(z_arch_esf_t, r11);
GEN_OFFSET_SYM(z_arch_esf_t, r12);
GEN_OFFSET_SYM(z_arch_esf_t, r13);
GEN_OFFSET_SYM(z_arch_esf_t, r14);
GEN_OFFSET_SYM(z_arch_esf_t, r15);
GEN_OFFSET_SYM(z_arch_esf_t, estatus);
GEN_OFFSET_SYM(z_arch_esf_t, instr);
GEN_ABSOLUTE_SYM(__z_arch_esf_t_SIZEOF, sizeof(z_arch_esf_t));

/*
 * size of the struct k_thread structure sans save area for floating
 * point regs
 */
GEN_ABSOLUTE_SYM(_K_THREAD_NO_FLOAT_SIZEOF, sizeof(struct k_thread));

GEN_ABS_SYM_END
