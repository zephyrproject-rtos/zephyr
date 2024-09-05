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

#include <zephyr/arch/exception.h>
#include <zephyr/kernel.h>
#include <kernel_arch_data.h>
#include <gen_offset.h>

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
#include <soc_context.h>
#endif
#ifdef CONFIG_RISCV_SOC_OFFSETS
#include <soc_offsets.h>
#endif

#include <kernel_offsets.h>

/* struct arch_csf member offsets */
GEN_OFFSET_STRUCT(arch_csf, sp);
GEN_OFFSET_STRUCT(arch_csf, ra);
GEN_OFFSET_STRUCT(arch_csf, s0);
GEN_OFFSET_STRUCT(arch_csf, s1);
#if !defined(CONFIG_RISCV_ISA_RV32E)
GEN_OFFSET_STRUCT(arch_csf, s2);
GEN_OFFSET_STRUCT(arch_csf, s3);
GEN_OFFSET_STRUCT(arch_csf, s4);
GEN_OFFSET_STRUCT(arch_csf, s5);
GEN_OFFSET_STRUCT(arch_csf, s6);
GEN_OFFSET_STRUCT(arch_csf, s7);
GEN_OFFSET_STRUCT(arch_csf, s8);
GEN_OFFSET_STRUCT(arch_csf, s9);
GEN_OFFSET_STRUCT(arch_csf, s10);
GEN_OFFSET_STRUCT(arch_csf, s11);
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
GEN_OFFSET_STRUCT(arch_esf, ra);
GEN_OFFSET_STRUCT(arch_esf, t0);
GEN_OFFSET_STRUCT(arch_esf, t1);
GEN_OFFSET_STRUCT(arch_esf, t2);
GEN_OFFSET_STRUCT(arch_esf, a0);
GEN_OFFSET_STRUCT(arch_esf, a1);
GEN_OFFSET_STRUCT(arch_esf, a2);
GEN_OFFSET_STRUCT(arch_esf, a3);
GEN_OFFSET_STRUCT(arch_esf, a4);
GEN_OFFSET_STRUCT(arch_esf, a5);

#if !defined(CONFIG_RISCV_ISA_RV32E)
GEN_OFFSET_STRUCT(arch_esf, t3);
GEN_OFFSET_STRUCT(arch_esf, t4);
GEN_OFFSET_STRUCT(arch_esf, t5);
GEN_OFFSET_STRUCT(arch_esf, t6);
GEN_OFFSET_STRUCT(arch_esf, a6);
GEN_OFFSET_STRUCT(arch_esf, a7);
#endif /* !CONFIG_RISCV_ISA_RV32E */

GEN_OFFSET_STRUCT(arch_esf, mepc);
GEN_OFFSET_STRUCT(arch_esf, mstatus);

GEN_OFFSET_STRUCT(arch_esf, s0);

#ifdef CONFIG_USERSPACE
GEN_OFFSET_STRUCT(arch_esf, sp);
#endif

#if defined(CONFIG_RISCV_SOC_CONTEXT_SAVE)
GEN_OFFSET_STRUCT(arch_esf, soc_context);
#endif
#if defined(CONFIG_RISCV_SOC_OFFSETS)
GEN_SOC_OFFSET_SYMS();
#endif

GEN_ABSOLUTE_SYM(__struct_arch_esf_SIZEOF, sizeof(struct arch_esf));

#ifdef CONFIG_EXCEPTION_DEBUG
GEN_ABSOLUTE_SYM(_struct_arch_csf_SIZEOF, ROUND_UP(sizeof(struct arch_csf), ARCH_STACK_PTR_ALIGN));
#endif /* CONFIG_EXCEPTION_DEBUG */

#ifdef CONFIG_USERSPACE
GEN_OFFSET_SYM(_cpu_arch_t, user_exc_sp);
GEN_OFFSET_SYM(_cpu_arch_t, user_exc_tmp0);
GEN_OFFSET_SYM(_cpu_arch_t, user_exc_tmp1);
#endif

GEN_ABS_SYM_END
