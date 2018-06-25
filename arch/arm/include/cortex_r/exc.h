/*
 * Copyright (c) 2018 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Exception/interrupt context helpers for Cortex-R CPUs
 *
 * Exception/interrupt context helpers.
 */

#ifndef _ARM_CORTEXR_ISR__H_
#define _ARM_CORTEXR_ISR__H_

#include <arch/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

/* nothing */

#else

#include <irq_offload.h>

#ifdef CONFIG_IRQ_OFFLOAD
extern volatile irq_offload_routine_t offload_routine;
#endif

/**
 *
 * @brief Find out if running in an ISR context
 *
 * Check the CPSR mode bits to see if we are in IRQ or FIQ mode
 *
 * @return 1 if in ISR, 0 if not.
 */
static ALWAYS_INLINE bool z_IsInIsr(void)
{
	unsigned int status;

	__asm__ volatile(
			" mrs %0, cpsr"
			: "=r" (status) : : "memory", "cc");
	status &= MODE_MASK;

	return (status == MODE_FIQ) || (status == MODE_IRQ);
}

/**
 * @brief Setup system exceptions
 *
 * Enable fault exceptions.
 *
 * @return N/A
 */
static ALWAYS_INLINE void z_ExcSetup(void)
{
}

/**
 * @brief Clear Fault exceptions
 *
 * Clear out exceptions for Mem, Bus, Usage and Hard Faults
 *
 * @return N/A
 */
static ALWAYS_INLINE void z_clearfaults(void)
{
}

extern void cortex_r_svc(void);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif


#endif /* _ARM_CORTEXRM_ISR__H_ */
