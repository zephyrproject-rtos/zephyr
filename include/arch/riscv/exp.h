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
#include <toolchain.h>

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

struct __esf {
	ulong_t ra;		/* return address */
	ulong_t gp;		/* global pointer */
	ulong_t tp;		/* thread pointer */

	ulong_t t0;		/* Caller-saved temporary register */
	ulong_t t1;		/* Caller-saved temporary register */
	ulong_t t2;		/* Caller-saved temporary register */
	ulong_t t3;		/* Caller-saved temporary register */
	ulong_t t4;		/* Caller-saved temporary register */
	ulong_t t5;		/* Caller-saved temporary register */
	ulong_t t6;		/* Caller-saved temporary register */

	ulong_t a0;		/* function argument/return value */
	ulong_t a1;		/* function argument */
	ulong_t a2;		/* function argument */
	ulong_t a3;		/* function argument */
	ulong_t a4;		/* function argument */
	ulong_t a5;		/* function argument */
	ulong_t a6;		/* function argument */
	ulong_t a7;		/* function argument */

	ulong_t mepc;		/* machine exception program counter */
	ulong_t mstatus;	/* machine status register */

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)

	ulong_t ft0;		/* Caller-saved temporary floating register */
	ulong_t ft1;		/* Caller-saved temporary floating register */
	ulong_t ft2;		/* Caller-saved temporary floating register */
	ulong_t ft3;		/* Caller-saved temporary floating register */
	ulong_t ft4;		/* Caller-saved temporary floating register */
	ulong_t ft5;		/* Caller-saved temporary floating register */
	ulong_t ft6;		/* Caller-saved temporary floating register */
	ulong_t ft7;		/* Caller-saved temporary floating register */
	ulong_t ft8;		/* Caller-saved temporary floating register */
	ulong_t ft9;		/* Caller-saved temporary floating register */
	ulong_t ft10;		/* Caller-saved temporary floating register */
	ulong_t ft11;		/* Caller-saved temporary floating register */

	ulong_t fa0;		/* function argument/return value */
	ulong_t fa1;		/* function argument/return value */
	ulong_t fa2;		/* function argument */
	ulong_t fa3;		/* function argument */
	ulong_t fa4;		/* function argument */
	ulong_t fa5;		/* function argument */
	ulong_t fa6;		/* function argument */
	ulong_t fa7;		/* function argument */

#endif

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	struct soc_esf soc_context;
#endif
};

typedef struct __esf z_arch_esf_t;
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
typedef struct soc_esf soc_esf_t;
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_EXP_H_ */
