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

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_R_EXCEPTION_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_R_EXCEPTION_H_

#include <zephyr/arch/cpu.h>

#ifdef _ASMLANGUAGE

/* nothing */

#else

#include <zephyr/irq_offload.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_IRQ_OFFLOAD
extern volatile irq_offload_routine_t offload_routine;
#endif

/* Check the CPSR mode bits to see if we are in IRQ or FIQ mode */
static ALWAYS_INLINE bool arch_is_in_isr(void)
{
	return (arch_curr_cpu()->nested != 0U);
}

static ALWAYS_INLINE bool arch_is_in_nested_exception(const struct arch_esf *esf)
{
	return (arch_curr_cpu()->arch.exc_depth > 1U) ? (true) : (false);
}

/**
 * @brief No current implementation where core dump is not supported
 *
 * @param esf exception frame
 * @param exc_return EXC_RETURN value present in LR after exception entry.
 */
static ALWAYS_INLINE void z_arm_set_fault_sp(const struct arch_esf *esf, uint32_t exc_return)
{}

#if defined(CONFIG_USERSPACE)
/*
 * This function is used by privileged code to determine if the thread
 * associated with the stack frame is in user mode.
 */
static ALWAYS_INLINE bool z_arm_preempted_thread_in_user_mode(const struct arch_esf *esf)
{
	return ((esf->basic.xpsr & CPSR_M_Msk) == CPSR_M_USR);
}
#endif

#ifndef CONFIG_USE_SWITCH
extern void z_arm_cortex_r_svc(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_R_EXCEPTION_H_ */
