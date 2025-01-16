/*
 * Copyright (c) 2021 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * based on include/arch/riscv/exception.h
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_MIPS_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_MIPS_EXCEPTION_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

struct arch_esf {
	unsigned long ra;		/* return address */
	unsigned long gp;		/* global pointer */

	unsigned long t0;		/* Caller-saved temporary register */
	unsigned long t1;		/* Caller-saved temporary register */
	unsigned long t2;		/* Caller-saved temporary register */
	unsigned long t3;		/* Caller-saved temporary register */
	unsigned long t4;		/* Caller-saved temporary register */
	unsigned long t5;		/* Caller-saved temporary register */
	unsigned long t6;		/* Caller-saved temporary register */
	unsigned long t7;		/* Caller-saved temporary register */
	unsigned long t8;		/* Caller-saved temporary register */
	unsigned long t9;		/* Caller-saved temporary register */

	unsigned long a0;		/* function argument */
	unsigned long a1;		/* function argument */
	unsigned long a2;		/* function argument */
	unsigned long a3;		/* function argument */

	unsigned long v0;		/* return value */
	unsigned long v1;		/* return value */

	unsigned long at;		/* assembly temporary */

	unsigned long epc;
	unsigned long badvaddr;
	unsigned long hi;
	unsigned long lo;
	unsigned long status;
	unsigned long cause;
};

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_MIPS_EXCEPTION_H_ */
