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

GEN_OFFSET_STRUCT(arch_esf, ra);
GEN_OFFSET_STRUCT(arch_esf, r1);
GEN_OFFSET_STRUCT(arch_esf, r2);
GEN_OFFSET_STRUCT(arch_esf, r3);
GEN_OFFSET_STRUCT(arch_esf, r4);
GEN_OFFSET_STRUCT(arch_esf, r5);
GEN_OFFSET_STRUCT(arch_esf, r6);
GEN_OFFSET_STRUCT(arch_esf, r7);
GEN_OFFSET_STRUCT(arch_esf, r8);
GEN_OFFSET_STRUCT(arch_esf, r9);
GEN_OFFSET_STRUCT(arch_esf, r10);
GEN_OFFSET_STRUCT(arch_esf, r11);
GEN_OFFSET_STRUCT(arch_esf, r12);
GEN_OFFSET_STRUCT(arch_esf, r13);
GEN_OFFSET_STRUCT(arch_esf, r14);
GEN_OFFSET_STRUCT(arch_esf, r15);
GEN_OFFSET_STRUCT(arch_esf, estatus);
GEN_OFFSET_STRUCT(arch_esf, instr);
GEN_ABSOLUTE_SYM(__struct_arch_esf_SIZEOF, sizeof(struct arch_esf));

GEN_ABS_SYM_END
