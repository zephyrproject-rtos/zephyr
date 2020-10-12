/*
 * Copyright (c) 2018 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Exception/interrupt context helpers for Cortex-A and Cortex-R CPUs
 *
 * Exception/interrupt context helpers.
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_A_R_EXC_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_A_R_EXC_H_

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
static ALWAYS_INLINE bool arch_is_in_isr(void)
{
	return (_kernel.cpus[0].nested != 0U);
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

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_A_R_EXC_H_ */
