/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions
 *
 * This file contains private kernel function/macro definitions and various
 * other definitions for the OpenRISC processor architecture.
 */

#ifndef ZEPHYR_ARCH_OPENRISC_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_OPENRISC_INCLUDE_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>

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

static ALWAYS_INLINE void
arch_switch(void *switch_to, void **switched_from)
{
	extern void z_openrisc_switch(struct k_thread *new, struct k_thread *old);
	struct k_thread *new = switch_to;
	struct k_thread *old = CONTAINER_OF(switched_from, struct k_thread,
					    switch_handle);
	z_openrisc_switch(new, old);
}

FUNC_NORETURN void z_openrisc_fatal_error(unsigned int reason,
					  const struct arch_esf *esf);

static inline bool arch_is_in_isr(void)
{
	return _current_cpu->nested != 0U;
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_OPENRISC_INCLUDE_KERNEL_ARCH_FUNC_H_ */
