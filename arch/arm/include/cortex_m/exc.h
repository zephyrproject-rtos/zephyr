/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Exception/interrupt context helpers for Cortex-M CPUs
 *
 * Exception/interrupt context helpers.
 */

#ifndef _ARM_CORTEXM_ISR__H_
#define _ARM_CORTEXM_ISR__H_

#include <arch/cpu.h>
#include <asm_inline.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

/* nothing */

#else

#include <arch/arm/cortex_m/cmsis.h>

/**
 *
 * @brief Find out if running in an ISR context
	 *
 * The current executing vector is found in the IPSR register. We consider the
 * IRQs (exception 16 and up), and the SVC, PendSV, and SYSTICK exceptions,
 * to be interrupts. Taking a fault within an exception is also considered in
 * interrupt context.
 *
 * @return 1 if in ISR, 0 if not.
 */
static ALWAYS_INLINE int _IsInIsr(void)
{
	uint32_t vector = _IpsrGet();

	/*
	 * IRQs + PendSV (14) + SVC (11) + SYSTICK (15) are interrupts.
	 * Vectors 12 and 13 are reserved, we'll never be in there
	 * On ARMv6-M there is no nested execution bit, so we check exception 3,
	 * hard fault, to a detect a nested exception.
	 */
#if defined(CONFIG_ARMV6_M)
	return (vector > 10) || (vector == 3);
#elif defined(CONFIG_ARMV7_M)
	return (vector > 10) || (vector && _ScbIsNestedExc());
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
}

#define _EXC_SVC_PRIO 0
#define _EXC_FAULT_PRIO 0
/**
 * @brief Setup system exceptions
 *
 * Set exception priorities to conform with the BASEPRI locking mechanism.
 * Set PendSV priority to lowest possible.
 *
 * Enable fault exceptions.
 *
 * @return N/A
 */
static ALWAYS_INLINE void _ExcSetup(void)
{
	NVIC_SetPriority(PendSV_IRQn, 0xff);

#ifdef CONFIG_CPU_CORTEX_M_HAS_BASEPRI
	NVIC_SetPriority(SVCall_IRQn, _EXC_SVC_PRIO);
#endif

#ifdef CONFIG_CPU_CORTEX_M_HAS_PROGRAMMABLE_FAULT_PRIOS
	NVIC_SetPriority(MemoryManagement_IRQn, _EXC_FAULT_PRIO);
	NVIC_SetPriority(BusFault_IRQn, _EXC_FAULT_PRIO);
	NVIC_SetPriority(UsageFault_IRQn, _EXC_FAULT_PRIO);

	_ScbUsageFaultEnable();
	_ScbBusFaultEnable();
	_ScbMemFaultEnable();
#endif
}

/**
 * @brief Clear Fault exceptions
 *
 * Clear out exceptions for Mem, Bus, Usage and Hard Faults
 *
 * @return N/A
 */
static ALWAYS_INLINE void _ClearFaults(void)
{
#if defined(CONFIG_ARMV6_M)
#elif defined(CONFIG_ARMV7_M)
	/* Reset all faults */
	_ScbMemFaultAllFaultsReset();
	_ScbBusFaultAllFaultsReset();
	_ScbUsageFaultAllFaultsReset();

	_ScbHardFaultAllFaultsReset();
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif


#endif /* _ARM_CORTEXM_ISR__H_ */
