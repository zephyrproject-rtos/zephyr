/*
 * Copyright (c) 2017 Imagination Technologies Ltd.
 */

/**
 * @file
 * @brief Private kernel definitions
 *
 * This file contains private kernel function/macro definitions and various
 * other definitions for the MIPS processor architecture.
 */

#ifndef _kernel_arch_func__h_
#define _kernel_arch_func__h_

#ifdef __cplusplus
extern "C" {
#endif

#include <soc.h>

#ifndef _ASMLANGUAGE
void k_cpu_idle(void);
void k_cpu_atomic_idle(unsigned int key);

static ALWAYS_INLINE void
_arch_switch_to_main_thread(struct k_thread *main_thread,
			k_thread_stack_t *main_stack,
			size_t main_stack_size, k_thread_entry_t _main)
{
	struct k_thread dummy_thread_memory;
	struct k_thread *dummy_thread = &dummy_thread_memory;

	_current = dummy_thread;

	dummy_thread->base.user_options = K_ESSENTIAL;
	dummy_thread->base.thread_state = _THREAD_DUMMY;

	mips_bissr(SR_IE);

	_Swap(irq_lock());
}

static ALWAYS_INLINE void kernel_arch_init(void)
{
	_kernel.irq_stack =
		K_THREAD_STACK_BUFFER(_interrupt_stack) + CONFIG_ISR_STACK_SIZE;
}

static ALWAYS_INLINE void
_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	thread->arch.swap_return_value = value;
}

static inline void _IntLibInit(void)
{
#if defined(CONFIG_MIPS_SOC_INTERRUPT_INIT)
	soc_interrupt_init();
#endif
}

FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int reason,
					const NANO_ESF *esf);

#define _is_in_isr() (_kernel.nested != 0)

#ifdef CONFIG_IRQ_OFFLOAD
int _irq_do_offload(void);
#endif

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _kernel_arch_func__h_ */
