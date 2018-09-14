/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Stack helpers for Cortex-M CPUs
 *
 * Stack helper functions.
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_M_STACK_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_M_STACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

/* nothing */

#else

#include "arch/arm/cortex_m/cmsis.h"

extern K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);

/**
 *
 * @brief Setup interrupt stack
 *
 * On Cortex-M, the interrupt stack is registered in the MSP (main stack
 * pointer) register, and switched to automatically when taking an exception.
 *
 * @return N/A
 */
static ALWAYS_INLINE void _InterruptStackSetup(void)
{
#ifdef CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT
	u32_t msp = (u32_t)(K_THREAD_STACK_BUFFER(_interrupt_stack) +
			    CONFIG_ISR_STACK_SIZE - MPU_GUARD_ALIGN_AND_SIZE);
#else
	u32_t msp = (u32_t)(K_THREAD_STACK_BUFFER(_interrupt_stack) +
			    CONFIG_ISR_STACK_SIZE);
#endif

	__set_MSP(msp);
#if defined(CONFIG_BUILTIN_STACK_GUARD)
#if defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
	__set_MSPLIM((u32_t)_interrupt_stack);
#else
#error "Built-in MSP limit checks not supported by HW"
#endif
#endif /* CONFIG_BUILTIN_STACK_GUARD */
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_M_STACK_H_ */
