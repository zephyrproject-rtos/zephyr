/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/pm/pm.h>
#include <kernel_arch_func.h>
#include <kernel_arch_swap.h>

/* dsPIC33A interrtup exit routine. Will check if a context
 * switch is required. If so, z_dspic_do_swap() will be called
 * to affect the context switch
 */
static inline __attribute__((always_inline)) void z_dspic_exc_exit(void)
{
#ifdef CONFIG_PREEMPT_ENABLED
	if ((_current_cpu->nested == 0) && (_kernel.ready_q.cache != _current) &&
	    !k_is_pre_kernel()) {
		z_dspic_do_swap();
	}

#endif /* CONFIG_PREEMPT_ENABLED */
#ifdef CONFIG_STACK_SENTINEL
	z_check_stack_sentinel();
#endif /* CONFIG_STACK_SENTINEL */
	return;
}

void __attribute__((interrupt)) _isr_wrapper(void)
{
#ifdef CONFIG_TRACING_ISR
	sys_trace_isr_enter();
#endif /* CONFIG_TRACING_ISR */

	_current_cpu->nested++;
	int32_t irq_number = INTTREGbits.VECNUM - 9;
	const struct _isr_table_entry *entry = &_sw_isr_table[irq_number];
	(entry->isr)(entry->arg);
	_current_cpu->nested--;

#ifdef CONFIG_TRACING_ISR
	sys_trace_isr_exit();
#endif /* CONFIG_TRACING_ISR */

	z_dspic_exc_exit();
}
