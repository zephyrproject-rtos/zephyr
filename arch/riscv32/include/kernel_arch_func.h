/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions
 *
 * This file contains private kernel function/macro definitions and various
 * other definitions for the RISCV32 processor architecture.
 */

#ifndef ZEPHYR_ARCH_RISCV32_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_RISCV32_INCLUDE_KERNEL_ARCH_FUNC_H_

#include <soc.h>
#include <kernel_structs.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
void k_cpu_idle(void);
void k_cpu_atomic_idle(unsigned int key);
_cpu_t *_arch_curr_cpu(void);

static ALWAYS_INLINE void kernel_arch_init(void)
{
#ifdef CONFIG_SMP
	_kernel.cpus[0].irq_stack =
		K_THREAD_STACK_BUFFER(_interrupt_stack) + CONFIG_ISR_STACK_SIZE;

	/* Init pointer to _kernel.cpus[0] in mscratch for hart 0 */
	u32_t cput = (u32_t) &(_kernel.cpus[0]);
	__asm__ volatile("csrw mscratch, %[cput]" :: [cput] "r" (cput));
#else
	_kernel.irq_stack =
		K_THREAD_STACK_BUFFER(_interrupt_stack) + CONFIG_ISR_STACK_SIZE;
#endif
}

#ifndef CONFIG_USE_SWITCH
/* If we're using new-style _arch_switch for context switching, this function
 * is no longer architecture-specific */
static ALWAYS_INLINE void
_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	thread->arch.swap_return_value = value;
}
# else
/* Point _arch_switch at our architecture-specific switch function */
extern void _riscv_switch(void *switch_to, void **switch_from);
#define _arch_switch _riscv_switch
#endif /* CONFIG_USE_SWITCH */

static inline void _IntLibInit(void)
{
#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
	soc_interrupt_init();
#endif
}

FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int reason,
					  const NANO_ESF *esf);

#ifdef CONFIG_SMP
#define _is_in_isr() (_arch_curr_cpu()->nested != 0)
#else
#define _is_in_isr() (_kernel.nested != 0)
#endif

#ifdef CONFIG_IRQ_OFFLOAD
int _irq_do_offload(void);
#endif

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_RISCV32_INCLUDE_KERNEL_ARCH_FUNC_H_ */
