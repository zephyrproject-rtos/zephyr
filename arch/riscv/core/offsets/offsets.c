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

#include <gen_offset.h>
#include <kernel_structs.h>
#include <kernel_offsets.h>

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
#include <soc_context.h>
#endif
#ifdef CONFIG_RISCV_SOC_OFFSETS
#include <soc_offsets.h>
#endif

/* thread_arch_t member offsets */
GEN_OFFSET_SYM(_thread_arch_t, swap_return_value);

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
