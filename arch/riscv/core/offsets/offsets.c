/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISCV32 kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various RISCV32 kernel
 * structures.
 */

#include <zephyr/kernel.h>
#include <kernel_arch_data.h>
#include <gen_offset.h>
#include <kernel_offsets.h>

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
#include <soc_context.h>
#endif
#ifdef CONFIG_RISCV_SOC_OFFSETS
#include <soc_offsets.h>
#endif

/* struct _callee_saved member offsets */
GEN_OFFSET_SYM(_callee_saved_t, sp);
GEN_OFFSET_SYM(_callee_saved_t, ra);
GEN_OFFSET_SYM(_callee_saved_t, s0);
GEN_OFFSET_SYM(_callee_saved_t, s1);
#if !defined(CONFIG_RISCV_ISA_RV32E)
GEN_OFFSET_SYM(_callee_saved_t, s2);
GEN_OFFSET_SYM(_callee_saved_t, s3);
GEN_OFFSET_SYM(_callee_saved_t, s4);
GEN_OFFSET_SYM(_callee_saved_t, s5);
GEN_OFFSET_SYM(_callee_saved_t, s6);
GEN_OFFSET_SYM(_callee_saved_t, s7);
GEN_OFFSET_SYM(_callee_saved_t, s8);
GEN_OFFSET_SYM(_callee_saved_t, s9);
GEN_OFFSET_SYM(_callee_saved_t, s10);
GEN_OFFSET_SYM(_callee_saved_t, s11);
#endif /* !CONFIG_RISCV_ISA_RV32E */

#if defined(CONFIG_FPU_SHARING)

GEN_OFFSET_SYM(z_riscv_fp_context_t, fa0);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fa1);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fa2);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fa3);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fa4);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fa5);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fa6);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fa7);

GEN_OFFSET_SYM(z_riscv_fp_context_t, ft0);
GEN_OFFSET_SYM(z_riscv_fp_context_t, ft1);
GEN_OFFSET_SYM(z_riscv_fp_context_t, ft2);
GEN_OFFSET_SYM(z_riscv_fp_context_t, ft3);
GEN_OFFSET_SYM(z_riscv_fp_context_t, ft4);
GEN_OFFSET_SYM(z_riscv_fp_context_t, ft5);
GEN_OFFSET_SYM(z_riscv_fp_context_t, ft6);
GEN_OFFSET_SYM(z_riscv_fp_context_t, ft7);
GEN_OFFSET_SYM(z_riscv_fp_context_t, ft8);
GEN_OFFSET_SYM(z_riscv_fp_context_t, ft9);
GEN_OFFSET_SYM(z_riscv_fp_context_t, ft10);
GEN_OFFSET_SYM(z_riscv_fp_context_t, ft11);

GEN_OFFSET_SYM(z_riscv_fp_context_t, fs0);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fs1);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fs2);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fs3);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fs4);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fs5);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fs6);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fs7);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fs8);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fs9);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fs10);
GEN_OFFSET_SYM(z_riscv_fp_context_t, fs11);

GEN_OFFSET_SYM(z_riscv_fp_context_t, fcsr);

GEN_OFFSET_SYM(_thread_arch_t, exception_depth);

#endif /* CONFIG_FPU_SHARING */

/* esf member offsets */
GEN_OFFSET_SYM(z_arch_esf_t, ra);
GEN_OFFSET_SYM(z_arch_esf_t, t0);
GEN_OFFSET_SYM(z_arch_esf_t, t1);
GEN_OFFSET_SYM(z_arch_esf_t, t2);
GEN_OFFSET_SYM(z_arch_esf_t, a0);
GEN_OFFSET_SYM(z_arch_esf_t, a1);
GEN_OFFSET_SYM(z_arch_esf_t, a2);
GEN_OFFSET_SYM(z_arch_esf_t, a3);
GEN_OFFSET_SYM(z_arch_esf_t, a4);
GEN_OFFSET_SYM(z_arch_esf_t, a5);

#if !defined(CONFIG_RISCV_ISA_RV32E)
GEN_OFFSET_SYM(z_arch_esf_t, t3);
GEN_OFFSET_SYM(z_arch_esf_t, t4);
GEN_OFFSET_SYM(z_arch_esf_t, t5);
GEN_OFFSET_SYM(z_arch_esf_t, t6);
GEN_OFFSET_SYM(z_arch_esf_t, a6);
GEN_OFFSET_SYM(z_arch_esf_t, a7);
#endif /* !CONFIG_RISCV_ISA_RV32E */

GEN_OFFSET_SYM(z_arch_esf_t, mepc);
GEN_OFFSET_SYM(z_arch_esf_t, mstatus);

GEN_OFFSET_SYM(z_arch_esf_t, s0);

#ifdef CONFIG_USERSPACE
GEN_OFFSET_SYM(z_arch_esf_t, sp);
#endif

#if defined(CONFIG_RISCV_SOC_CONTEXT_SAVE)
GEN_OFFSET_SYM(z_arch_esf_t, soc_context);
#endif
#if defined(CONFIG_RISCV_SOC_OFFSETS)
GEN_SOC_OFFSET_SYMS();
#endif

GEN_ABSOLUTE_SYM(__z_arch_esf_t_SIZEOF, sizeof(z_arch_esf_t));

#ifdef CONFIG_USERSPACE
GEN_OFFSET_SYM(_cpu_arch_t, user_exc_sp);
GEN_OFFSET_SYM(_cpu_arch_t, user_exc_tmp0);
GEN_OFFSET_SYM(_cpu_arch_t, user_exc_tmp1);
#endif

GEN_ABS_SYM_END
