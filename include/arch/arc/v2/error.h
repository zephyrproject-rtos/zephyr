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

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_ERROR_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_ERROR_H_

#include <arch/arc/syscall.h>
#include <arch/arc/v2/exc.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
#include <toolchain/gcc.h>
extern void _NanoFatalErrorHandler(unsigned int, const NANO_ESF*);
extern void _SysFatalErrorHandler(unsigned int cause, const NANO_ESF *esf);
#endif

#define _NANO_ERR_HW_EXCEPTION (0)      /* MPU/Bus/Usage fault */
#define _NANO_ERR_STACK_CHK_FAIL (2)    /* Stack corruption detected */
#define _NANO_ERR_ALLOCATION_FAIL (3)   /* Kernel Allocation Failure */
#define _NANO_ERR_KERNEL_OOPS (4)       /* Kernel oops (fatal to thread) */
#define _NANO_ERR_KERNEL_PANIC (5)	/* Kernel panic (fatal to system) */


/*
 * the exception caused by kernel will be handled in interrupt context
 * when the processor is already in interrupt context, no need to raise
 * a new exception; when the processor is in thread context, the exception
 * will be raised
 */
#define _ARCH_EXCEPT(reason_p)	do { \
	if (_arc_v2_irq_unit_is_in_isr()) { \
		printk("@ %s:%d:\n", __FILE__,  __LINE__); \
		_NanoFatalErrorHandler(reason_p, 0); \
	} else {\
		__asm__ volatile ( \
		"mov r0, %[reason]\n\t" \
		"trap_s %[id]\n\t" \
		: \
		: [reason] "i" (reason_p), \
		[id] "i" (_TRAP_S_CALL_RUNTIME_EXCEPT) \
		: "memory"); \
		CODE_UNREACHABLE; \
	} \
	} while (0)

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_ERROR_H_ */
