/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM AArch32 public exception handling
 *
 * ARM AArch32-specific kernel exception handling interface. Included by
 * arm/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_EXCEPTION_H_

#if defined(CONFIG_CPU_CORTEX_M)
#include <zephyr/arch/arm/cortex_m/exception.h>
#elif defined(CONFIG_CPU_AARCH32_CORTEX_A) || defined(CONFIG_CPU_AARCH32_CORTEX_R)
#include <zephyr/arch/arm/cortex_a_r/exception.h>
#else
#error Unknown ARM architecture
#endif /* CONFIG_CPU_CORTEX_M */

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

struct arch_sig_ctx {
	unsigned long sp; /* user stack pointer */
	unsigned long lr; /* user link register */
	unsigned long r0; /* syscall return value */
	unsigned long pc; /* syscall return address */
	/*
	 * Callee-saved registers of the interrupted context. Not part of the privileged
	 * syscall stack frame: the signal trampoline chain clobbers them per the ABI, and
	 * k_sig_return() resumes the interrupted context without unwinding the trampoline's
	 * frames, so they are captured at redirection and restored on the k_sig_return()
	 * exit path.
	 */
	unsigned long s[8]; /* r4..r11 */
};

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_EXCEPTION_H_ */
