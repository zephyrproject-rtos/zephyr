/*
 * Copyright (c) 2021 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * based on arch/riscv/core/offsets/offsets.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_arch_data.h>
#include <gen_offset.h>
#include <kernel_offsets.h>

GEN_OFFSET_SYM(_thread_arch_t, swap_return_value);

GEN_OFFSET_SYM(_callee_saved_t, sp);
GEN_OFFSET_SYM(_callee_saved_t, s0);
GEN_OFFSET_SYM(_callee_saved_t, s1);
GEN_OFFSET_SYM(_callee_saved_t, s2);
GEN_OFFSET_SYM(_callee_saved_t, s3);
GEN_OFFSET_SYM(_callee_saved_t, s4);
GEN_OFFSET_SYM(_callee_saved_t, s5);
GEN_OFFSET_SYM(_callee_saved_t, s6);
GEN_OFFSET_SYM(_callee_saved_t, s7);
GEN_OFFSET_SYM(_callee_saved_t, s8);

GEN_OFFSET_STRUCT(arch_esf, ra);
GEN_OFFSET_STRUCT(arch_esf, gp);
GEN_OFFSET_STRUCT(arch_esf, t0);
GEN_OFFSET_STRUCT(arch_esf, t1);
GEN_OFFSET_STRUCT(arch_esf, t2);
GEN_OFFSET_STRUCT(arch_esf, t3);
GEN_OFFSET_STRUCT(arch_esf, t4);
GEN_OFFSET_STRUCT(arch_esf, t5);
GEN_OFFSET_STRUCT(arch_esf, t6);
GEN_OFFSET_STRUCT(arch_esf, t7);
GEN_OFFSET_STRUCT(arch_esf, t8);
GEN_OFFSET_STRUCT(arch_esf, t9);
GEN_OFFSET_STRUCT(arch_esf, a0);
GEN_OFFSET_STRUCT(arch_esf, a1);
GEN_OFFSET_STRUCT(arch_esf, a2);
GEN_OFFSET_STRUCT(arch_esf, a3);
GEN_OFFSET_STRUCT(arch_esf, v0);
GEN_OFFSET_STRUCT(arch_esf, v1);
GEN_OFFSET_STRUCT(arch_esf, at);
GEN_OFFSET_STRUCT(arch_esf, epc);
GEN_OFFSET_STRUCT(arch_esf, badvaddr);
GEN_OFFSET_STRUCT(arch_esf, hi);
GEN_OFFSET_STRUCT(arch_esf, lo);
GEN_OFFSET_STRUCT(arch_esf, status);
GEN_OFFSET_STRUCT(arch_esf, cause);

GEN_ABSOLUTE_SYM(__struct_arch_esf_SIZEOF, STACK_ROUND_UP(sizeof(struct arch_esf)));

GEN_ABS_SYM_END
