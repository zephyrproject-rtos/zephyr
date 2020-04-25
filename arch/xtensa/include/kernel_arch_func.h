/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* this file is only meant to be included by kernel_structs.h */

#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_KERNEL_ARCH_FUNC_H_

#ifndef _ASMLANGUAGE
#include <kernel_internal.h>
#include <kernel_arch_data.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void FatalErrorHandler(void);
extern void ReservedInterruptHandler(unsigned int intNo);
extern void z_xtensa_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

/* Defined in xtensa_context.S */
extern void z_xt_coproc_init(void);

extern K_KERNEL_STACK_ARRAY_DEFINE(z_interrupt_stacks, CONFIG_MP_NUM_CPUS,
				   CONFIG_ISR_STACK_SIZE);

static ALWAYS_INLINE void arch_kernel_init(void)
{
	_cpu_t *cpu0 = &_kernel.cpus[0];

	cpu0->nested = 0;

	/* The asm2 scheme keeps the kernel pointer in MISC0 for easy
	 * access.  That saves 4 bytes of immediate value to store the
	 * address when compared to the legacy scheme.  But in SMP
	 * this record is a per-CPU thing and having it stored in a SR
	 * already is a big win.
	 */
	WSR(CONFIG_XTENSA_KERNEL_CPU_PTR_SR, cpu0);

#ifdef CONFIG_INIT_STACKS
	memset(Z_KERNEL_STACK_BUFFER(z_interrupt_stacks[0]), 0xAA,
	       K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[0]));
#endif
}

void xtensa_switch(void *switch_to, void **switched_from);

static inline void arch_switch(void *switch_to, void **switched_from)
{
	return xtensa_switch(switch_to, switched_from);
}

#ifdef __cplusplus
}
#endif

static inline bool arch_is_in_isr(void)
{
	return arch_curr_cpu()->nested != 0U;
}
#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_KERNEL_ARCH_FUNC_H_ */
