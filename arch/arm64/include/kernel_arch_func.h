/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com<
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions (ARM64)
 *
 * This file contains private kernel function definitions and various
 * other definitions for the ARM Cortex-A processor architecture family.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute symbols"
 * in the offsets.o module.
 */

#ifndef ZEPHYR_ARCH_ARM64_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_ARM64_INCLUDE_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

static ALWAYS_INLINE void arch_kernel_init(void)
{
}

static inline void arch_switch(void *switch_to, void **switched_from)
{
	extern void z_arm64_context_switch(struct k_thread *new,
					   struct k_thread *old);
	struct k_thread *new = switch_to;
	struct k_thread *old = CONTAINER_OF(switched_from, struct k_thread,
					    switch_handle);

	z_arm64_context_switch(new, old);
}

extern void z_arm64_fatal_error(unsigned int reason, z_arch_esf_t *esf);
extern void z_arm64_set_ttbr0(uint64_t ttbr0);
extern void z_arm64_mem_cfg_ipi(void);

#ifdef CONFIG_FPU_SHARING
void z_arm64_flush_local_fpu(void);
void z_arm64_flush_fpu_ipi(unsigned int cpu);
#endif

#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
void z_arm64_safe_exception_stack_init(void);
#endif

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM64_INCLUDE_KERNEL_ARCH_FUNC_H_ */
