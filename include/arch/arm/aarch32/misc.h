/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM AArch32 public kernel miscellaneous
 *
 * ARM AArch32-specific kernel miscellaneous interface. Included by arm/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_MISC_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_MISC_H_

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t sys_clock_cycle_get_32(void);

static inline uint32_t arch_k_cycle_get_32(void)
{
	return sys_clock_cycle_get_32();
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

#if defined(CONFIG_USERSPACE)
#if defined(CONFIG_CPU_CORTEX_M)
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#elif defined(CONFIG_CPU_CORTEX_R)
#include <arch/arm/aarch32/cortex_a_r/cmsis.h>
#endif

static inline bool z_arm_thread_is_in_user_mode(void)
{
	uint32_t value;

#if defined(CONFIG_CPU_CORTEX_R)
	/*
	 * For Cortex-R, the mode (lower 5) bits will be 0x10 for user mode.
	 */
	value = __get_CPSR();
	return ((value & CPSR_M_Msk) == CPSR_M_USR);
#elif defined(CONFIG_CPU_CORTEX_M)
	/* return mode information */
	value = __get_CONTROL();
	return (value & CONTROL_nPRIV_Msk) != 0;
#else
#error Unknown ARM architecture
#endif
}
#endif

#ifdef __cplusplus
}
#endif

#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_MISC_H_ */
