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

GEN_OFFSET_SYM(z_arch_esf_t, ra);
GEN_OFFSET_SYM(z_arch_esf_t, gp);
GEN_OFFSET_SYM(z_arch_esf_t, t0);
GEN_OFFSET_SYM(z_arch_esf_t, t1);
GEN_OFFSET_SYM(z_arch_esf_t, t2);
GEN_OFFSET_SYM(z_arch_esf_t, t3);
GEN_OFFSET_SYM(z_arch_esf_t, t4);
GEN_OFFSET_SYM(z_arch_esf_t, t5);
GEN_OFFSET_SYM(z_arch_esf_t, t6);
GEN_OFFSET_SYM(z_arch_esf_t, t7);
GEN_OFFSET_SYM(z_arch_esf_t, t8);
GEN_OFFSET_SYM(z_arch_esf_t, t9);
GEN_OFFSET_SYM(z_arch_esf_t, a0);
GEN_OFFSET_SYM(z_arch_esf_t, a1);
GEN_OFFSET_SYM(z_arch_esf_t, a2);
GEN_OFFSET_SYM(z_arch_esf_t, a3);
GEN_OFFSET_SYM(z_arch_esf_t, v0);
GEN_OFFSET_SYM(z_arch_esf_t, v1);
GEN_OFFSET_SYM(z_arch_esf_t, at);
GEN_OFFSET_SYM(z_arch_esf_t, epc);
GEN_OFFSET_SYM(z_arch_esf_t, badvaddr);
GEN_OFFSET_SYM(z_arch_esf_t, hi);
GEN_OFFSET_SYM(z_arch_esf_t, lo);
GEN_OFFSET_SYM(z_arch_esf_t, status);
GEN_OFFSET_SYM(z_arch_esf_t, cause);

GEN_ABSOLUTE_SYM(__z_arch_esf_t_SIZEOF, STACK_ROUND_UP(sizeof(z_arch_esf_t)));

GEN_ABS_SYM_END
