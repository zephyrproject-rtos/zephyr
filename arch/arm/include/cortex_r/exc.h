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

#ifdef _ASMLANGUAGE

/* nothing */

#else

#include <irq_offload.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_IRQ_OFFLOAD
extern volatile irq_offload_routine_t offload_routine;
#endif

/* Check the CPSR mode bits to see if we are in IRQ or FIQ mode */
static ALWAYS_INLINE bool z_arch_is_in_isr(void)
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
static ALWAYS_INLINE void z_arm_exc_setup(void)
{
}

/**
 * @brief Clear Fault exceptions
 *
 * Clear out exceptions for Mem, Bus, Usage and Hard Faults
 *
 * @return N/A
 */
static ALWAYS_INLINE void z_arm_clear_faults(void)
{
}

extern void z_arm_cortex_r_svc(void);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* _ARM_CORTEXRM_ISR__H_ */
