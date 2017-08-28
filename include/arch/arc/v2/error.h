/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 public error handling
 *
 * ARC-specific kernel error handling interface. Included by arc/arch.h.
 */

#ifndef _ARCH_ARC_V2_ERROR_H_
#define _ARCH_ARC_V2_ERROR_H_

#include <arch/arc/v2/exc.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
#include <toolchain/gcc.h>
extern FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int,
						 const NANO_ESF*);
extern void _SysFatalErrorHandler(unsigned int cause, const NANO_ESF *esf);
#endif

#define _NANO_ERR_HW_EXCEPTION (0)      /* MPU/Bus/Usage fault */
#define _NANO_ERR_STACK_CHK_FAIL (2)    /* Stack corruption detected */
#define _NANO_ERR_ALLOCATION_FAIL (3)   /* Kernel Allocation Failure */
#define _NANO_ERR_KERNEL_OOPS (4)       /* Kernel oops (fatal to thread) */
#define _NANO_ERR_KERNEL_PANIC (5)	/* Kernel panic (fatal to system) */

#ifdef __cplusplus
}
#endif


#endif /* _ARCH_ARC_V2_ERROR_H_ */
