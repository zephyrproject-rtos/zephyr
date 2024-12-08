/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This file is only meant to be included by kernel_structs.h */

#ifndef ZEPHYR_ARCH_POSIX_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_POSIX_INCLUDE_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>

#include <zephyr/platform/hooks.h>

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

static inline void arch_kernel_init(void)
{
#ifdef CONFIG_SOC_PER_CORE_INIT_HOOK
	soc_per_core_init_hook();
#endif /* CONFIG_SOC_PER_CORE_INIT_HOOK */
}

static ALWAYS_INLINE void
arch_thread_return_value_set(struct k_thread *thread, unsigned int value)
{
	thread->callee_saved.retval = value;
}

#ifdef __cplusplus
}
#endif

static inline bool arch_is_in_isr(void)
{
	return _kernel.cpus[0].nested != 0U;
}

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_POSIX_INCLUDE_KERNEL_ARCH_FUNC_H_ */
