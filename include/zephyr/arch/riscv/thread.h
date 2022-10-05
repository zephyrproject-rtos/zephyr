/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Per-arch thread definition
 *
 * This file contains definitions for
 *
 *  struct _thread_arch
 *  struct _callee_saved
 *
 * necessary to instantiate instances of struct k_thread.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#if !defined(RV_FP_TYPE) && defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
#define RV_FP_TYPE uint64_t
#else
#define RV_FP_TYPE uint32_t
#endif
#endif

/*
 * The following structure defines the list of registers that need to be
 * saved/restored when a context switch occurs.
 */
struct _callee_saved {
	unsigned long sp;	/* Stack pointer, (x2 register) */
	unsigned long ra;	/* return address */

	unsigned long s0;	/* saved register/frame pointer */
	unsigned long s1;	/* saved register */
#if !defined(CONFIG_RISCV_ISA_RV32E)
	unsigned long s2;	/* saved register */
	unsigned long s3;	/* saved register */
	unsigned long s4;	/* saved register */
	unsigned long s5;	/* saved register */
	unsigned long s6;	/* saved register */
	unsigned long s7;	/* saved register */
	unsigned long s8;	/* saved register */
	unsigned long s9;	/* saved register */
	unsigned long s10;	/* saved register */
	unsigned long s11;	/* saved register */
#endif

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	uint32_t fcsr;		/* Control and status register */
	RV_FP_TYPE fs0;		/* saved floating-point register */
	RV_FP_TYPE fs1;		/* saved floating-point register */
	RV_FP_TYPE fs2;		/* saved floating-point register */
	RV_FP_TYPE fs3;		/* saved floating-point register */
	RV_FP_TYPE fs4;		/* saved floating-point register */
	RV_FP_TYPE fs5;		/* saved floating-point register */
	RV_FP_TYPE fs6;		/* saved floating-point register */
	RV_FP_TYPE fs7;		/* saved floating-point register */
	RV_FP_TYPE fs8;		/* saved floating-point register */
	RV_FP_TYPE fs9;		/* saved floating-point register */
	RV_FP_TYPE fs10;	/* saved floating-point register */
	RV_FP_TYPE fs11;	/* saved floating-point register */
#endif
};
typedef struct _callee_saved _callee_saved_t;

#define PMP_M_MODE_SLOTS 8	/* 8 is plenty enough for m-mode */

struct _thread_arch {
#ifdef CONFIG_USERSPACE
	unsigned long priv_stack_start;
	unsigned long u_mode_pmpaddr_regs[CONFIG_PMP_SLOTS];
	unsigned long u_mode_pmpcfg_regs[CONFIG_PMP_SLOTS / sizeof(unsigned long)];
	unsigned int u_mode_pmp_domain_offset;
	unsigned int u_mode_pmp_end_index;
	unsigned int u_mode_pmp_update_nr;
#endif
#ifdef CONFIG_PMP_STACK_GUARD
	unsigned int m_mode_pmp_end_index;
	unsigned long m_mode_pmpaddr_regs[PMP_M_MODE_SLOTS];
	unsigned long m_mode_pmpcfg_regs[PMP_M_MODE_SLOTS / sizeof(unsigned long)];
#endif
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_THREAD_H_ */
