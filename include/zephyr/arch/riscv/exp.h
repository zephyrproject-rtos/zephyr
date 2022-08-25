/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISCV public exception handling
 *
 * RISCV-specific kernel exception handling interface.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_EXP_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_EXP_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <zephyr/toolchain.h>

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
#include <soc_context.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The name of the structure which contains soc-specific state, if
 * any, as well as the soc_esf_t typedef below, are part of the RISC-V
 * arch API.
 *
 * The contents of the struct are provided by a SOC-specific
 * definition in soc_context.h.
 */
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
struct soc_esf {
	SOC_ESF_MEMBERS;
};
#endif

#if !defined(RV_FP_TYPE) && defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
#define RV_FP_TYPE uint64_t
#else
#define RV_FP_TYPE uint32_t
#endif
#endif

struct __esf {
	unsigned long ra;		/* return address */

	unsigned long t0;		/* Caller-saved temporary register */
	unsigned long t1;		/* Caller-saved temporary register */
	unsigned long t2;		/* Caller-saved temporary register */
#if !defined(CONFIG_RISCV_ISA_RV32E)
	unsigned long t3;		/* Caller-saved temporary register */
	unsigned long t4;		/* Caller-saved temporary register */
	unsigned long t5;		/* Caller-saved temporary register */
	unsigned long t6;		/* Caller-saved temporary register */
#endif /* !CONFIG_RISCV_ISA_RV32E */

	unsigned long a0;		/* function argument/return value */
	unsigned long a1;		/* function argument */
	unsigned long a2;		/* function argument */
	unsigned long a3;		/* function argument */
	unsigned long a4;		/* function argument */
	unsigned long a5;		/* function argument */
#if !defined(CONFIG_RISCV_ISA_RV32E)
	unsigned long a6;		/* function argument */
	unsigned long a7;		/* function argument */
#endif /* !CONFIG_RISCV_ISA_RV32E */

	unsigned long mepc;		/* machine exception program counter */
	unsigned long mstatus;	/* machine status register */

	unsigned long s0;		/* callee-saved s0 */

#ifdef CONFIG_USERSPACE
	unsigned long sp;		/* preserved (user or kernel) stack pointer */
#endif

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	RV_FP_TYPE ft0;		/* Caller-saved temporary floating register */
	RV_FP_TYPE ft1;		/* Caller-saved temporary floating register */
	RV_FP_TYPE ft2;		/* Caller-saved temporary floating register */
	RV_FP_TYPE ft3;		/* Caller-saved temporary floating register */
	RV_FP_TYPE ft4;		/* Caller-saved temporary floating register */
	RV_FP_TYPE ft5;		/* Caller-saved temporary floating register */
	RV_FP_TYPE ft6;		/* Caller-saved temporary floating register */
	RV_FP_TYPE ft7;		/* Caller-saved temporary floating register */
	RV_FP_TYPE ft8;		/* Caller-saved temporary floating register */
	RV_FP_TYPE ft9;		/* Caller-saved temporary floating register */
	RV_FP_TYPE ft10;	/* Caller-saved temporary floating register */
	RV_FP_TYPE ft11;	/* Caller-saved temporary floating register */
	RV_FP_TYPE fa0;		/* function argument/return value */
	RV_FP_TYPE fa1;		/* function argument/return value */
	RV_FP_TYPE fa2;		/* function argument */
	RV_FP_TYPE fa3;		/* function argument */
	RV_FP_TYPE fa4;		/* function argument */
	RV_FP_TYPE fa5;		/* function argument */
	RV_FP_TYPE fa6;		/* function argument */
	RV_FP_TYPE fa7;		/* function argument */
#endif

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	struct soc_esf soc_context;
#endif
} __aligned(16);

typedef struct __esf z_arch_esf_t;
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
typedef struct soc_esf soc_esf_t;
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_EXP_H_ */
