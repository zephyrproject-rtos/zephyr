/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-M public error handling
 *
 * ARM-specific kernel error handling interface. Included by arm/arch.h.
 */

#ifndef _ARCH_ARM_CORTEXM_ERROR_H_
#define _ARCH_ARM_CORTEXM_ERROR_H_

#include <arch/arm/cortex_m/exc.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
extern void _NanoFatalErrorHandler(unsigned int reason, const NANO_ESF *esf);
extern void _SysFatalErrorHandler(unsigned int reason, const NANO_ESF *esf);
#endif

#define _NANO_ERR_HW_EXCEPTION (0)      /* MPU/Bus/Usage fault */
#define _NANO_ERR_INVALID_TASK_EXIT (1) /* Invalid task exit */
#define _NANO_ERR_STACK_CHK_FAIL (2)    /* Stack corruption detected */
#define _NANO_ERR_ALLOCATION_FAIL (3)   /* Kernel Allocation Failure */
#define _NANO_ERR_KERNEL_OOPS (4)       /* Kernel oops (fatal to thread) */
#define _NANO_ERR_KERNEL_PANIC (5)	/* Kernel panic (fatal to system) */

#define _SVC_CALL_IRQ_OFFLOAD		1
#define _SVC_CALL_RUNTIME_EXCEPT	2

#if defined(CONFIG_ARMV6_M)
/* ARMv6 will hard-fault if SVC is called with interrupts locked. Just
 * force them unlocked, the thread is in an undefined state anyway
 */
#define _ARCH_EXCEPT(reason_p) do { \
	__asm__ volatile ( \
		"cpsie i\n\t" \
		"mov r0, %[reason]\n\t" \
		"svc %[id]\n\t" \
		: \
		: [reason] "i" (reason_p), [id] "i" (_SVC_CALL_RUNTIME_EXCEPT) \
		: "memory"); \
	CODE_UNREACHABLE; \
} while (0)
#elif defined(CONFIG_ARMV7_M)
#define _ARCH_EXCEPT(reason_p) do { \
	__asm__ volatile ( \
		"mov r0, %[reason]\n\t" \
		"svc %[id]\n\t" \
		: \
		: [reason] "i" (reason_p), [id] "i" (_SVC_CALL_RUNTIME_EXCEPT) \
		: "memory"); \
	CODE_UNREACHABLE; \
} while (0)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */

#ifdef __cplusplus
}
#endif

#endif /* _ARCH_ARM_CORTEXM_ERROR_H_ */
