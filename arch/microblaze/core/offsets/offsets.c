/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <gen_offset.h>
#include <kernel_arch_data.h>
#include <kernel_offsets.h>

GEN_OFFSET_SYM(_callee_saved_t, r1);
GEN_OFFSET_SYM(_callee_saved_t, key);
GEN_OFFSET_SYM(_callee_saved_t, retval);
GEN_OFFSET_SYM(_callee_saved_t, preempted);

GEN_OFFSET_STRUCT(arch_esf, r31);
GEN_OFFSET_STRUCT(arch_esf, r30);
GEN_OFFSET_STRUCT(arch_esf, r29);
GEN_OFFSET_STRUCT(arch_esf, r28);
GEN_OFFSET_STRUCT(arch_esf, r27);
GEN_OFFSET_STRUCT(arch_esf, r26);
GEN_OFFSET_STRUCT(arch_esf, r25);
GEN_OFFSET_STRUCT(arch_esf, r24);
GEN_OFFSET_STRUCT(arch_esf, r23);
GEN_OFFSET_STRUCT(arch_esf, r22);
GEN_OFFSET_STRUCT(arch_esf, r21);
GEN_OFFSET_STRUCT(arch_esf, r20);
GEN_OFFSET_STRUCT(arch_esf, r19);
GEN_OFFSET_STRUCT(arch_esf, r18);
GEN_OFFSET_STRUCT(arch_esf, r17);
GEN_OFFSET_STRUCT(arch_esf, r16);
GEN_OFFSET_STRUCT(arch_esf, r15);
GEN_OFFSET_STRUCT(arch_esf, r14);
GEN_OFFSET_STRUCT(arch_esf, r13);
GEN_OFFSET_STRUCT(arch_esf, r12);
GEN_OFFSET_STRUCT(arch_esf, r11);
GEN_OFFSET_STRUCT(arch_esf, r10);
GEN_OFFSET_STRUCT(arch_esf, r11);
GEN_OFFSET_STRUCT(arch_esf, r10);
GEN_OFFSET_STRUCT(arch_esf, r9);
GEN_OFFSET_STRUCT(arch_esf, r8);
GEN_OFFSET_STRUCT(arch_esf, r7);
GEN_OFFSET_STRUCT(arch_esf, r6);
GEN_OFFSET_STRUCT(arch_esf, r5);
GEN_OFFSET_STRUCT(arch_esf, r4);
GEN_OFFSET_STRUCT(arch_esf, r3);
GEN_OFFSET_STRUCT(arch_esf, r2);
GEN_OFFSET_STRUCT(arch_esf, msr);
#if defined(CONFIG_MICROBLAZE_USE_HARDWARE_FLOAT_INSTR)
GEN_OFFSET_STRUCT(arch_esf, fsr);
#endif

GEN_ABSOLUTE_SYM(__struct_arch_esf_SIZEOF, STACK_ROUND_UP(sizeof(struct arch_esf)));

GEN_ABS_SYM_END
