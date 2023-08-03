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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

static ALWAYS_INLINE void arch_kernel_init(void)
{
}

static ALWAYS_INLINE void
arch_thread_return_value_set(struct k_thread *thread, unsigned int value)
{
	thread->arch.swap_return_value = value;
}

extern FUNC_NORETURN void z_arm_userspace_enter(k_thread_entry_t user_entry,
					       void *p1, void *p2, void *p3,
					       uint32_t stack_end,
					       uint32_t stack_start);

extern void z_arm_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_R_KERNEL_ARCH_FUNC_H_ */
