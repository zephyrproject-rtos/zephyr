/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/m68k/exception.h>
#include <zephyr/fatal.h>
#include <zephyr/sw_isr_table.h>
#include <kswap.h>
#include <vector.h>

void z_m68k_exc_handle_sync(struct arch_esf *esf)
{
	const unsigned int vector = esf->vector;

	if (vector == M68K_VECTOR_RUNTIME_EXCEPT) {
		z_m68k_fatal_error(esf->d0, esf);
	} else {
		z_m68k_fatal_error(K_ERR_CPU_EXCEPTION, esf);
	}
}

void z_irq_spurious(const void *unused)
{
	ARG_UNUSED(unused);
	z_m68k_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

/*
 * CPU vectors index the first 256 _sw_isr_table entries directly.
 * Cascaded controller entries follow the CPU vector range.
 */
void z_m68k_exc_handle_async(struct arch_esf *esf)
{
	const unsigned int vector = esf->vector;
	const struct _isr_table_entry *entry = &_sw_isr_table[vector];

	if ((vector == M68K_VECTOR_UNINITIALIZED_INTERRUPT) ||
	    (vector == M68K_VECTOR_SPURIOUS) ||
	    (vector == M68K_VECTOR_NMI) || (entry->isr == z_irq_spurious)) {
		z_m68k_fatal_error(K_ERR_SPURIOUS_IRQ, esf);
		return;
	}

	entry->isr(entry->arg);
}

void z_m68k_isr_enter(void)
{
	_current_cpu->nested++;

	if (IS_ENABLED(CONFIG_TRACING_ISR)) {
		sys_trace_isr_enter();
	}
}

/*
 * Only the outermost interrupt may reschedule. Capture _current before
 * z_sched_next_handle() can replace it.
 */
void *z_m68k_isr_exit(struct k_thread **old_thread)
{
	if (IS_ENABLED(CONFIG_TRACING_ISR)) {
		sys_trace_isr_exit();
	}

	_current_cpu->nested--;

	if (_current_cpu->nested == 0) {
		struct k_thread *curr = _current;

		z_check_stack_sentinel();

		*old_thread = curr;
		return z_sched_next_handle(curr);
	}

	return NULL;
}
