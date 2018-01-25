/*
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MIPS public exception handling
 *
 * MIPS-specific kernel exception handling interface.
 */

#ifndef _ARCH_MIPS_EXP_H_
#define _ARCH_MIPS_EXP_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <toolchain.h>

typedef struct gpctx NANO_ESF;
extern const NANO_ESF _default_esf;

extern FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int reason,
						 const NANO_ESF *esf);
extern void _SysFatalErrorHandler(unsigned int reason,
				  const NANO_ESF *esf);

#endif /* _ASMLANGUAGE */

#define _NANO_ERR_CPU_EXCEPTION (0)      /* Any unhandled exception */
#define _NANO_ERR_INVALID_TASK_EXIT (1)  /* Invalid task exit */
#define _NANO_ERR_STACK_CHK_FAIL (2)     /* Stack corruption detected */
#define _NANO_ERR_ALLOCATION_FAIL (3)    /* Kernel Allocation Failure */
#define _NANO_ERR_SPURIOUS_INT (4)	 /* Spurious interrupt */
#define _NANO_ERR_KERNEL_OOPS (5)       /* Kernel oops (fatal to thread) */
#define _NANO_ERR_KERNEL_PANIC (6)	/* Kernel panic (fatal to system) */

#ifdef __cplusplus
}
#endif

#endif /* _ARCH_MIPS_EXP_H_ */
