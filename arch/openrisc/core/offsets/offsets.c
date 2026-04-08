/*
 * Copyright (c) 2025 NVIDIA Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OpenRISC kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various OpenRISC kernel structures.
 */

#include <gen_offset.h>
#include <kernel_arch_data.h>
#include <kernel_offsets.h>

GEN_OFFSET_SYM(_callee_saved_t, r1);
GEN_OFFSET_SYM(_callee_saved_t, r2);
GEN_OFFSET_SYM(_callee_saved_t, r9);
GEN_OFFSET_SYM(_callee_saved_t, r10);
GEN_OFFSET_SYM(_callee_saved_t, r14);
GEN_OFFSET_SYM(_callee_saved_t, r16);
GEN_OFFSET_SYM(_callee_saved_t, r18);
GEN_OFFSET_SYM(_callee_saved_t, r20);
GEN_OFFSET_SYM(_callee_saved_t, r22);
GEN_OFFSET_SYM(_callee_saved_t, r24);
GEN_OFFSET_SYM(_callee_saved_t, r26);
GEN_OFFSET_SYM(_callee_saved_t, r28);
GEN_OFFSET_SYM(_callee_saved_t, r30);

GEN_ABSOLUTE_SYM(_callee_saved_t_SIZEOF, sizeof(_callee_saved_t));

GEN_OFFSET_STRUCT(arch_esf, r3);
GEN_OFFSET_STRUCT(arch_esf, r4);
GEN_OFFSET_STRUCT(arch_esf, r5);
GEN_OFFSET_STRUCT(arch_esf, r6);
GEN_OFFSET_STRUCT(arch_esf, r7);
GEN_OFFSET_STRUCT(arch_esf, r8);
GEN_OFFSET_STRUCT(arch_esf, r11);
GEN_OFFSET_STRUCT(arch_esf, r12);
GEN_OFFSET_STRUCT(arch_esf, r13);
GEN_OFFSET_STRUCT(arch_esf, r15);
GEN_OFFSET_STRUCT(arch_esf, r17);
GEN_OFFSET_STRUCT(arch_esf, r19);
GEN_OFFSET_STRUCT(arch_esf, r21);
GEN_OFFSET_STRUCT(arch_esf, r23);
GEN_OFFSET_STRUCT(arch_esf, r25);
GEN_OFFSET_STRUCT(arch_esf, r27);
GEN_OFFSET_STRUCT(arch_esf, r29);
GEN_OFFSET_STRUCT(arch_esf, r31);
GEN_OFFSET_STRUCT(arch_esf, mac_lo);
GEN_OFFSET_STRUCT(arch_esf, mac_hi);
GEN_OFFSET_STRUCT(arch_esf, epcr);
GEN_OFFSET_STRUCT(arch_esf, esr);

GEN_ABSOLUTE_SYM(__struct_arch_esf_SIZEOF, sizeof(struct arch_esf));

GEN_ABS_SYM_END
