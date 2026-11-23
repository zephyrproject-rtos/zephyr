/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RH850_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_RH850_INCLUDE_KERNEL_ARCH_FUNC_H_

#ifndef _ASMLANGUAGE
#include <kernel_arch_data.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void z_rh850_arch_switch_thread(void *switch_to, void **switched_from);

static ALWAYS_INLINE void arch_kernel_init(void)
{
	/* check if: further device initialization functions must be called here */
}

static inline bool arch_is_in_isr(void)
{
	return (arch_curr_cpu()->nested != 0U);
}

static inline void arch_switch(void *switch_to, void **switched_from)
{
	z_rh850_arch_switch_thread(switch_to, switched_from);
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_RH850_INCLUDE_KERNEL_ARCH_FUNC_H_ */
