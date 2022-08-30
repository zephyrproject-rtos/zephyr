/*
 * Copyright (c) 2021 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * based on include/arch/riscv/exp.h
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_MIPS_EXP_H_
#define ZEPHYR_INCLUDE_ARCH_MIPS_EXP_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

struct __esf {
	ulong_t ra;		/* return address */
	ulong_t gp;		/* global pointer */

	ulong_t t0;		/* Caller-saved temporary register */
	ulong_t t1;		/* Caller-saved temporary register */
	ulong_t t2;		/* Caller-saved temporary register */
	ulong_t t3;		/* Caller-saved temporary register */
	ulong_t t4;		/* Caller-saved temporary register */
	ulong_t t5;		/* Caller-saved temporary register */
	ulong_t t6;		/* Caller-saved temporary register */
	ulong_t t7;		/* Caller-saved temporary register */
	ulong_t t8;		/* Caller-saved temporary register */
	ulong_t t9;		/* Caller-saved temporary register */

	ulong_t a0;		/* function argument */
	ulong_t a1;		/* function argument */
	ulong_t a2;		/* function argument */
	ulong_t a3;		/* function argument */

	ulong_t v0;		/* return value */
	ulong_t v1;		/* return value */

	ulong_t at;		/* assembly temporary */

	ulong_t epc;
	ulong_t badvaddr;
	ulong_t hi;
	ulong_t lo;
	ulong_t status;
	ulong_t cause;
};

typedef struct __esf z_arch_esf_t;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_MIPS_EXP_H_ */
