/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_FUNC_H_

#include <kernel_structs.h>

#ifndef _ASMLANGUAGE

extern void z_x86_switch(void *switch_to, void **switched_from);

static inline void z_arch_switch(void *switch_to, void **switched_from)
{
	z_x86_switch(switch_to, switched_from);
}

/**
 * @brief Initialize scheduler IPI vector.
 *
 * Called in early BSP boot to set up scheduler IPI handling.
 */

extern void z_x86_ipi_setup(void);

static inline void z_arch_kernel_init(void)
{
	/* nothing */;
}

static inline struct _cpu *z_arch_curr_cpu(void)
{
	struct _cpu *cpu;

	__asm__ volatile("movq %%gs:(%c1), %0"
			 : "=r" (cpu)
			 : "i" (offsetof(x86_tss64_t, cpu)));

	return cpu;
}
#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_FUNC_H_ */
