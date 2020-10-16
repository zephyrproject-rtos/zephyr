/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions
 *
 * This file contains private kernel function/macro definitions and various
 * other definitions for the SPARC processor architecture.
 */

#ifndef ZEPHYR_ARCH_SPARC_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_SPARC_INCLUDE_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
static ALWAYS_INLINE void arch_kernel_init(void)
{
}

void z_sparc_arch_switch(void *switch_to, void **switched_from);

static inline void arch_switch(void *switch_to, void **switched_from)
{
	z_sparc_arch_switch(switch_to, switched_from);
}

FUNC_NORETURN void z_sparc_fatal_error(unsigned int reason,
				       const z_arch_esf_t *esf);

static inline bool arch_is_in_isr(void)
{
	return _current_cpu->nested != 0U;
}

#ifdef CONFIG_IRQ_OFFLOAD
void z_irq_do_offload(void);
#endif

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_SPARC_INCLUDE_KERNEL_ARCH_FUNC_H_ */
