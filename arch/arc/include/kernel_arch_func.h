/*
 * Copyright (c) 2014-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions
 *
 * This file contains private kernel structures definitions and various
 * other definitions for the ARCv2 processor architecture.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute
 * symbols" in the offsets.o module.
 */

#ifndef ZEPHYR_ARCH_ARC_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_ARC_INCLUDE_KERNEL_ARCH_FUNC_H_

#if !defined(_ASMLANGUAGE)

#ifdef CONFIG_CPU_ARCV2
#include <v2/cache.h>
#include <v2/irq.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE _cpu_t *z_arch_curr_cpu(void)
{
#ifdef CONFIG_SMP
	u32_t core;

	core = z_arc_v2_core_id();

	return &_kernel.cpus[core];
#else
	return &_kernel.cpus[0];
#endif
}

static ALWAYS_INLINE void z_arch_kernel_init(void)
{
	z_irq_setup();
	_current_cpu->irq_stack =
		Z_THREAD_STACK_BUFFER(_interrupt_stack) + CONFIG_ISR_STACK_SIZE;
}


/**
 *
 * @brief Indicates the interrupt number of the highest priority
 * active interrupt
 *
 * @return IRQ number
 */
static ALWAYS_INLINE int Z_INTERRUPT_CAUSE(void)
{
	u32_t irq_num = z_arc_v2_aux_reg_read(_ARC_V2_ICAUSE);

	return irq_num;
}

static inline bool z_arch_is_in_isr(void)
{
	return z_arc_v2_irq_unit_is_in_isr();
}

extern void z_thread_entry_wrapper(void);
extern void z_user_thread_entry_wrapper(void);

extern void z_arc_userspace_enter(k_thread_entry_t user_entry, void *p1,
		 void *p2, void *p3, u32_t stack, u32_t size);


extern void z_arch_switch(void *switch_to, void **switched_from);
extern void z_arc_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

extern void z_arch_sched_ipi(void);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_ARC_INCLUDE_KERNEL_ARCH_FUNC_H_ */
