/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright 2023, 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM AArch32 public error handling
 *
 * ARM AArch32-specific kernel error handling interface. Included by
 * arm/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_ERROR_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_ERROR_H_

#include <zephyr/arch/arm/syscall.h>
#include <zephyr/arch/exception.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_CPU_CORTEX_M)
/* ARMv6 will hard-fault if SVC is called with interrupts locked. Just
 * force them unlocked, the thread is in an undefined state anyway
 *
 * On ARMv7m we won't get a HardFault, but if interrupts were locked the
 * thread will continue executing after the exception and forbid PendSV to
 * schedule a new thread until they are unlocked which is not what we want.
 * Force them unlocked as well.
 */
#define ARCH_EXCEPT(reason_p) \
do {\
	arch_irq_unlock(0); \
	__asm__ volatile( \
		"mov r0, %[_reason]\n" \
		"svc %[id]\n" \
		IF_ENABLED(CONFIG_ARM_BTI, ("bti\n")) \
		:: [_reason] "r" (reason_p), [id] "i" (_SVC_CALL_RUNTIME_EXCEPT) \
		: "r0", "memory"); \
} while (false)
#elif defined(CONFIG_ARMV7_R) || defined(CONFIG_AARCH32_ARMV8_R) \
	|| defined(CONFIG_ARMV7_A)
/*
 * In order to support using svc for an exception while running in an
 * isr, stack $lr_svc before calling svc.  While exiting the isr,
 * z_check_stack_sentinel is called.  $lr_svc contains the return address.
 * If the sentinel is wrong, it calls svc to cause an oops.  This svc
 * call will overwrite $lr_svc, losing the return address from the
 * z_check_stack_sentinel call if it is not stacked before the svc.
 */
#define ARCH_EXCEPT(reason_p) \
do { \
	register uint32_t r0 __asm__("r0") = reason_p; \
	__asm__ volatile ( \
		"push {lr}\n\t" \
		"cpsie i\n\t" \
		"svc %[id]\n\t" \
		IF_ENABLED(CONFIG_ARM_BTI, ("bti\n\t")) \
		"pop {lr}\n\t" \
		: \
		: "r" (r0), [id] "i" (_SVC_CALL_RUNTIME_EXCEPT) \
		: "memory"); \
} while (false)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_ERROR_H_ */
