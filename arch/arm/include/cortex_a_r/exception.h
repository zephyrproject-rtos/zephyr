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

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE bool arch_is_in_isr(void)
{
	uint32_t nested;
#ifdef CONFIG_SMP
	unsigned int key;

	key = arch_irq_lock();
#endif
	nested = arch_curr_cpu()->nested;
#ifdef CONFIG_SMP
	arch_irq_unlock(key);
#endif
	return nested != 0U;
}

static ALWAYS_INLINE bool arch_is_in_nested_exception(const struct arch_esf *esf)
{
	return (_current_cpu->arch.exc_depth > 1U) ? (true) : (false);
}

#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
/*
 * Stack space below the exception frame for the callee-saved registers
 * and the extra exception info, which sits right below the frame as part
 * of struct arch_esf.  Rounded up to keep the stack 8-byte aligned.
 */
#define Z_ARM_EXC_EXTRA_SIZE                                                                       \
	ROUND_UP(sizeof(struct _callee_saved) + sizeof(struct __extra_esf_info), 8)
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_R_EXCEPTION_H_ */
