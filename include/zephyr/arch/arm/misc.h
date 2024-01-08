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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
extern uint32_t sys_clock_cycle_get_32(void);

static inline uint32_t arch_k_cycle_get_32(void)
{
	return sys_clock_cycle_get_32();
}

extern uint64_t sys_clock_cycle_get_64(void);

static inline uint64_t arch_k_cycle_get_64(void)
{
	return sys_clock_cycle_get_64();
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

#if defined(CONFIG_USERSPACE)
extern bool z_arm_thread_is_in_user_mode(void);
#endif

#if defined(CONFIG_ARM_ON_ENTER_CPU_IDLE_HOOK)
/* Prototype of a hook that can be enabled to be called every time the CPU is
 * made idle (the calls will be done from k_cpu_idle() and k_cpu_atomic_idle()).
 * If this hook returns false, the CPU is prevented from entering the actual
 * sleep (the WFE/WFI instruction is skipped).
 */
bool z_arm_on_enter_cpu_idle(void);
#endif

#if defined(CONFIG_ARM_ON_ENTER_CPU_IDLE_PREPARE_HOOK)
/* Prototype of a hook that can be enabled to be called every time the CPU is
 * made idle (the calls will be done from k_cpu_idle() and k_cpu_atomic_idle()).
 * The function is called before interrupts are disabled and can prepare to
 * upcoming call to z_arm_on_enter_cpu_idle.
 */
void z_arm_on_enter_cpu_idle_prepare(void);
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_MISC_H_ */
