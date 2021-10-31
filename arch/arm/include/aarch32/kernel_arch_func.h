/*
 * Copyright (c) 2013-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions (ARM)
 *
 * This file contains private kernel function definitions and various
 * other definitions for the 32-bit ARM Cortex-A/R/M processor architecture
 * family.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute symbols"
 * in the offsets.o module.
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
extern void z_arm_fault_init(void);
extern void z_arm_cpu_idle_init(void);
#ifdef CONFIG_ARM_MPU
extern void z_arm_configure_static_mpu_regions(void);
extern void z_arm_configure_dynamic_mpu_regions(struct k_thread *thread);
extern int z_arm_mpu_init(void);
#endif /* CONFIG_ARM_MPU */
#ifdef CONFIG_ARM_AARCH32_MMU
extern int z_arm_mmu_init(void);
#endif /* CONFIG_ARM_AARCH32_MMU */

static ALWAYS_INLINE void arch_kernel_init(void)
{
	z_arm_interrupt_stack_setup();
	z_arm_exc_setup();
	z_arm_fault_init();
	z_arm_cpu_idle_init();
	z_arm_clear_faults();
#if defined(CONFIG_ARM_MPU)
	z_arm_mpu_init();
	/* Configure static memory map. This will program MPU regions,
	 * to set up access permissions for fixed memory sections, such
	 * as Application Memory or No-Cacheable SRAM area.
	 *
	 * This function is invoked once, upon system initialization.
	 */
	z_arm_configure_static_mpu_regions();
#endif /* CONFIG_ARM_MPU */
#if defined(CONFIG_ARM_AARCH32_MMU)
	z_arm_mmu_init();
#endif /* CONFIG_ARM_AARCH32_MMU */
}

static ALWAYS_INLINE void
arch_thread_return_value_set(struct k_thread *thread, unsigned int value)
{
	thread->arch.swap_return_value = value;
}

#if !defined(CONFIG_MULTITHREADING) && defined(CONFIG_CPU_CORTEX_M)
extern FUNC_NORETURN void z_arm_switch_to_main_no_multithreading(
	k_thread_entry_t main_func,
	void *p1, void *p2, void *p3);

#define ARCH_SWITCH_TO_MAIN_NO_MULTITHREADING \
	z_arm_switch_to_main_no_multithreading

#endif /* !CONFIG_MULTITHREADING && CONFIG_CPU_CORTEX_M */

extern FUNC_NORETURN void z_arm_userspace_enter(k_thread_entry_t user_entry,
					       void *p1, void *p2, void *p3,
					       uint32_t stack_end,
					       uint32_t stack_start);

extern void z_arm_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_KERNEL_ARCH_FUNC_H_ */
