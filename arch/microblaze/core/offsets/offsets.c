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

GEN_OFFSET_SYM(z_arch_esf_t, r31);
GEN_OFFSET_SYM(z_arch_esf_t, r30);
GEN_OFFSET_SYM(z_arch_esf_t, r29);
GEN_OFFSET_SYM(z_arch_esf_t, r28);
GEN_OFFSET_SYM(z_arch_esf_t, r27);
GEN_OFFSET_SYM(z_arch_esf_t, r26);
GEN_OFFSET_SYM(z_arch_esf_t, r25);
GEN_OFFSET_SYM(z_arch_esf_t, r24);
GEN_OFFSET_SYM(z_arch_esf_t, r23);
GEN_OFFSET_SYM(z_arch_esf_t, r22);
GEN_OFFSET_SYM(z_arch_esf_t, r21);
GEN_OFFSET_SYM(z_arch_esf_t, r20);
GEN_OFFSET_SYM(z_arch_esf_t, r19);
GEN_OFFSET_SYM(z_arch_esf_t, r18);
GEN_OFFSET_SYM(z_arch_esf_t, r17);
GEN_OFFSET_SYM(z_arch_esf_t, r16);
GEN_OFFSET_SYM(z_arch_esf_t, r15);
GEN_OFFSET_SYM(z_arch_esf_t, r14);
GEN_OFFSET_SYM(z_arch_esf_t, r13);
GEN_OFFSET_SYM(z_arch_esf_t, r12);
GEN_OFFSET_SYM(z_arch_esf_t, r11);
GEN_OFFSET_SYM(z_arch_esf_t, r10);
GEN_OFFSET_SYM(z_arch_esf_t, r11);
GEN_OFFSET_SYM(z_arch_esf_t, r10);
GEN_OFFSET_SYM(z_arch_esf_t, r9);
GEN_OFFSET_SYM(z_arch_esf_t, r8);
GEN_OFFSET_SYM(z_arch_esf_t, r7);
GEN_OFFSET_SYM(z_arch_esf_t, r6);
GEN_OFFSET_SYM(z_arch_esf_t, r5);
GEN_OFFSET_SYM(z_arch_esf_t, r4);
GEN_OFFSET_SYM(z_arch_esf_t, r3);
GEN_OFFSET_SYM(z_arch_esf_t, r2);
GEN_OFFSET_SYM(z_arch_esf_t, msr);
#if defined(CONFIG_USE_HARDWARE_FLOAT_INSTR)
GEN_OFFSET_SYM(z_arch_esf_t, fsr);
#endif

GEN_ABSOLUTE_SYM(__z_arch_esf_t_SIZEOF, STACK_ROUND_UP(sizeof(z_arch_esf_t)));

GEN_ABS_SYM_END
