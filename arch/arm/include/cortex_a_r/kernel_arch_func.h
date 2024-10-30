/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Private kernel definitions (ARM)
 *
 * This file contains private kernel function definitions and various
 * other definitions for the 32-bit ARM Cortex-A/R processor architecture
 * family.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute symbols"
 * in the offsets.o module.
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_R_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_R_KERNEL_ARCH_FUNC_H_

#include <zephyr/platform/hooks.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

static ALWAYS_INLINE void arch_kernel_init(void)
{
#ifdef CONFIG_SOC_PER_CORE_INIT_HOOK
	soc_per_core_init_hook();
#endif /* CONFIG_SOC_PER_CORE_INIT_HOOK */
}

#ifndef CONFIG_USE_SWITCH

static ALWAYS_INLINE void
arch_thread_return_value_set(struct k_thread *thread, unsigned int value)
{
	thread->arch.swap_return_value = value;
}

#else

static ALWAYS_INLINE void arch_switch(void *switch_to, void **switched_from)
{
	extern void z_arm_context_switch(struct k_thread *new,
					struct k_thread *old);

	struct k_thread *new = switch_to;
	struct k_thread *old = CONTAINER_OF(switched_from, struct k_thread,
					    switch_handle);

	z_arm_context_switch(new, old);
}

#endif

extern FUNC_NORETURN void z_arm_userspace_enter(k_thread_entry_t user_entry,
					       void *p1, void *p2, void *p3,
					       uint32_t stack_end,
					       uint32_t stack_start);

extern void z_arm_fatal_error(unsigned int reason, const struct arch_esf *esf);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_R_KERNEL_ARCH_FUNC_H_ */
