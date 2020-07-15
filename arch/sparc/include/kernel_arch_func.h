/*
 * Copyright (c) 2018 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions (SPARC)
 *
 */

/* this file is only meant to be included by kernel_structs.h */

#ifndef ZEPHYR_ARCH_SPARC_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_SPARC_INCLUDE_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

extern K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);

/**
 * @brief Perform architecture-specific initialization
 *
 * This routine performs architecture-specific initialization of the
 * kernel.  Trivial stuff is done inline; more complex initialization
 * is done via function calls.
 *
 * @return N/A
 */
static ALWAYS_INLINE void arch_kernel_init(void)
{

}

/* SPARC currently supports USE_SWITCH and USE_SWITCH only */
void arch_switch(void *switch_to, void **switched_from);

static inline bool arch_is_in_isr(void)
{
	return _kernel.cpus[0].nested != 0U;
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_SPARC_INCLUDE_KERNEL_ARCH_FUNC_H_ */
