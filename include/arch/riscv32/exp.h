/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISCV32 public exception handling
 *
 * RISCV32-specific kernel exception handling interface.
 */

#ifndef _ARCH_RISCV32_EXP_H_
#define _ARCH_RISCV32_EXP_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <toolchain.h>

struct __esf {
	u32_t ra;       /* return address */
	u32_t gp;       /* global pointer */
	u32_t tp;       /* thread pointer */

	u32_t t0;       /* Caller-saved temporary register */
	u32_t t1;       /* Caller-saved temporary register */
	u32_t t2;       /* Caller-saved temporary register */
	u32_t t3;       /* Caller-saved temporary register */
	u32_t t4;       /* Caller-saved temporary register */
	u32_t t5;       /* Caller-saved temporary register */
	u32_t t6;       /* Caller-saved temporary register */

	u32_t a0;       /* function argument/return value */
	u32_t a1;       /* function argument */
	u32_t a2;       /* function argument */
	u32_t a3;       /* function argument */
	u32_t a4;       /* function argument */
	u32_t a5;       /* function argument */
	u32_t a6;       /* function argument */
	u32_t a7;       /* function argument */

	u32_t mepc;      /* machine exception program counter */
	u32_t mstatus;   /* machine status register */

#if defined(CONFIG_SOC_RISCV32_PULPINO)
	/* pulpino hardware loop registers */
	u32_t lpstart0;
	u32_t lpend0;
	u32_t lpcount0;
	u32_t lpstart1;
	u32_t lpend1;
	u32_t lpcount1;
#endif
};

typedef struct __esf NANO_ESF;
extern const NANO_ESF _default_esf;

extern FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int reason,
						 const NANO_ESF *esf);
extern void _SysFatalErrorHandler(unsigned int reason,
				  const NANO_ESF *esf);

#endif /* _ASMLANGUAGE */

#define _NANO_ERR_CPU_EXCEPTION (0)      /* Any unhandled exception */
#define _NANO_ERR_STACK_CHK_FAIL (2)     /* Stack corruption detected */
#define _NANO_ERR_ALLOCATION_FAIL (3)    /* Kernel Allocation Failure */
#define _NANO_ERR_SPURIOUS_INT (4)	 /* Spurious interrupt */
#define _NANO_ERR_KERNEL_OOPS (5)       /* Kernel oops (fatal to thread) */
#define _NANO_ERR_KERNEL_PANIC (6)	/* Kernel panic (fatal to system) */

#ifdef __cplusplus
}
#endif

#endif /* _ARCH_RISCV32_EXP_H_ */
