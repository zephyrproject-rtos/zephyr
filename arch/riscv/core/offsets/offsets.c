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

#include <kernel.h>
#include <kernel_arch_data.h>
#include <gen_offset.h>
#include <kernel_offsets.h>

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
#include <soc_context.h>
#endif
#ifdef CONFIG_RISCV_SOC_OFFSETS
#include <soc_offsets.h>
#endif

/* thread_arch_t member offsets */
GEN_OFFSET_SYM(_thread_arch_t, swap_return_value);
#if defined(CONFIG_USERSPACE)
GEN_OFFSET_SYM(_thread_arch_t, priv_stack_start);
GEN_OFFSET_SYM(_thread_arch_t, user_sp);
#endif

/* struct coop member offsets */
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
GEN_OFFSET_SYM(_callee_saved_t, s9);
GEN_OFFSET_SYM(_callee_saved_t, s10);
GEN_OFFSET_SYM(_callee_saved_t, s11);

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
GEN_OFFSET_SYM(_callee_saved_t, fcsr);
GEN_OFFSET_SYM(_callee_saved_t, fs0);
GEN_OFFSET_SYM(_callee_saved_t, fs1);
GEN_OFFSET_SYM(_callee_saved_t, fs2);
GEN_OFFSET_SYM(_callee_saved_t, fs3);
GEN_OFFSET_SYM(_callee_saved_t, fs4);
GEN_OFFSET_SYM(_callee_saved_t, fs5);
GEN_OFFSET_SYM(_callee_saved_t, fs6);
GEN_OFFSET_SYM(_callee_saved_t, fs7);
GEN_OFFSET_SYM(_callee_saved_t, fs8);
GEN_OFFSET_SYM(_callee_saved_t, fs9);
GEN_OFFSET_SYM(_callee_saved_t, fs10);
GEN_OFFSET_SYM(_callee_saved_t, fs11);
#endif

/* esf member offsets */
GEN_OFFSET_SYM(z_arch_esf_t, ra);
GEN_OFFSET_SYM(z_arch_esf_t, gp);
GEN_OFFSET_SYM(z_arch_esf_t, tp);
GEN_OFFSET_SYM(z_arch_esf_t, t0);
GEN_OFFSET_SYM(z_arch_esf_t, t1);
GEN_OFFSET_SYM(z_arch_esf_t, t2);
GEN_OFFSET_SYM(z_arch_esf_t, t3);
GEN_OFFSET_SYM(z_arch_esf_t, t4);
GEN_OFFSET_SYM(z_arch_esf_t, t5);
GEN_OFFSET_SYM(z_arch_esf_t, t6);
GEN_OFFSET_SYM(z_arch_esf_t, a0);
GEN_OFFSET_SYM(z_arch_esf_t, a1);
GEN_OFFSET_SYM(z_arch_esf_t, a2);
GEN_OFFSET_SYM(z_arch_esf_t, a3);
GEN_OFFSET_SYM(z_arch_esf_t, a4);
GEN_OFFSET_SYM(z_arch_esf_t, a5);
GEN_OFFSET_SYM(z_arch_esf_t, a6);
GEN_OFFSET_SYM(z_arch_esf_t, a7);

GEN_OFFSET_SYM(z_arch_esf_t, mepc);
GEN_OFFSET_SYM(z_arch_esf_t, mstatus);

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
GEN_OFFSET_SYM(z_arch_esf_t, fp_state);
GEN_OFFSET_SYM(z_arch_esf_t, ft0);
GEN_OFFSET_SYM(z_arch_esf_t, ft1);
GEN_OFFSET_SYM(z_arch_esf_t, ft2);
GEN_OFFSET_SYM(z_arch_esf_t, ft3);
GEN_OFFSET_SYM(z_arch_esf_t, ft4);
GEN_OFFSET_SYM(z_arch_esf_t, ft5);
GEN_OFFSET_SYM(z_arch_esf_t, ft6);
GEN_OFFSET_SYM(z_arch_esf_t, ft7);
GEN_OFFSET_SYM(z_arch_esf_t, ft8);
GEN_OFFSET_SYM(z_arch_esf_t, ft9);
GEN_OFFSET_SYM(z_arch_esf_t, ft10);
GEN_OFFSET_SYM(z_arch_esf_t, ft11);
GEN_OFFSET_SYM(z_arch_esf_t, fa0);
GEN_OFFSET_SYM(z_arch_esf_t, fa1);
GEN_OFFSET_SYM(z_arch_esf_t, fa2);
GEN_OFFSET_SYM(z_arch_esf_t, fa3);
GEN_OFFSET_SYM(z_arch_esf_t, fa4);
GEN_OFFSET_SYM(z_arch_esf_t, fa5);
GEN_OFFSET_SYM(z_arch_esf_t, fa6);
GEN_OFFSET_SYM(z_arch_esf_t, fa7);
#endif

#if defined(CONFIG_RISCV_SOC_CONTEXT_SAVE)
GEN_OFFSET_SYM(z_arch_esf_t, soc_context);
#endif
#if defined(CONFIG_RISCV_SOC_OFFSETS)
GEN_SOC_OFFSET_SYMS();
#endif

/*
 * RISC-V requires the stack to be 16-bytes aligned, hence SP needs to grow or
 * shrink by a size, which follows the RISC-V stack alignment requirements
 * Hence, ensure that __z_arch_esf_t_SIZEOF and _K_THREAD_NO_FLOAT_SIZEOF sizes
 * are aligned accordingly.
 */
GEN_ABSOLUTE_SYM(__z_arch_esf_t_SIZEOF, STACK_ROUND_UP(sizeof(z_arch_esf_t)));

/*
 * size of the struct k_thread structure sans save area for floating
 * point regs
 */
GEN_ABSOLUTE_SYM(_K_THREAD_NO_FLOAT_SIZEOF,
		 STACK_ROUND_UP(sizeof(struct k_thread)));

GEN_ABS_SYM_END
