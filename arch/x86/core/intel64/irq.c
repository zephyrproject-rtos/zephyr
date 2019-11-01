/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <arch/cpu.h>
#include <kernel_arch_data.h>
#include <drivers/interrupt_controller/sysapic.h>
#include <drivers/interrupt_controller/loapic.h>
#include <irq.h>

unsigned char _irq_to_interrupt_vector[CONFIG_MAX_IRQ_LINES];

/*
 * The low-level interrupt code consults these arrays to dispatch IRQs, so
 * so be sure to keep locore.S up to date with any changes. Note the indices:
 * use (vector - IV_IRQS), since exception vectors do not appear here.
 *
 * Entries which are NULL in x86_irq_funcs[] correspond to unassigned vectors.
 * The locore IRQ handler should (read: doesn't currently) raise an exception
 * rather than attempt to dispatch to a NULL x86_irq_func[]. FIXME.
 */

#define NR_IRQ_VECTORS (IV_NR_VECTORS - IV_IRQS)  /* # vectors free for IRQs */

void (*x86_irq_funcs[NR_IRQ_VECTORS])(void *);
void *x86_irq_args[NR_IRQ_VECTORS];

/*
 * Find a free IRQ vector at the specified priority, or return -1 if none left.
 */

static int allocate_vector(unsigned int priority)
{
	const int VECTORS_PER_PRIORITY = 16;
	const int MAX_PRIORITY = 13;

	int vector;
	int i;

	if (priority >= MAX_PRIORITY) {
		priority = MAX_PRIORITY;
	}

	vector = (priority * VECTORS_PER_PRIORITY) + IV_IRQS;

	for (i = 0; i < VECTORS_PER_PRIORITY; ++i, ++vector) {
#ifdef CONFIG_IRQ_OFFLOAD
		if (vector == CONFIG_IRQ_OFFLOAD_VECTOR) {
			continue;
		}
#endif
		if (x86_irq_funcs[vector - IV_IRQS] == NULL) {
			return vector;
		}
	}

	return -1;
}

/*
 * N.B.: the API docs don't say anything about returning error values, but
 * this function returns -1 if a vector at the specific priority can't be
 * allocated. Whether it should simply __ASSERT instead is up for debate.
 */

int z_arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
		void (*func)(void *arg), void *arg, u32_t flags)
{
	u32_t key;
	int vector;

	__ASSERT(irq <= CONFIG_MAX_IRQ_LINES, "IRQ %u out of range", irq);

	key = irq_lock();

	vector = allocate_vector(priority);
	if (vector >= 0) {
		_irq_to_interrupt_vector[irq] = vector;
		z_irq_controller_irq_config(vector, irq, flags);
		x86_irq_funcs[vector - IV_IRQS] = func;
		x86_irq_args[vector - IV_IRQS] = arg;
	}

	irq_unlock(key);
	return vector;
}

#ifdef CONFIG_IRQ_OFFLOAD
#include <irq_offload.h>

void z_arch_irq_offload(irq_offload_routine_t routine, void *parameter)
{
	x86_irq_funcs[CONFIG_IRQ_OFFLOAD_VECTOR - IV_IRQS] = routine;
	x86_irq_args[CONFIG_IRQ_OFFLOAD_VECTOR - IV_IRQS] = parameter;
	__asm__ volatile("int %0" : : "i" (CONFIG_IRQ_OFFLOAD_VECTOR)
			  : "memory");
	x86_irq_funcs[CONFIG_IRQ_OFFLOAD_VECTOR - IV_IRQS] = NULL;
}

#endif /* CONFIG_IRQ_OFFLOAD */

#if defined(CONFIG_SMP)

void z_x86_ipi_setup(void)
{
	/*
	 * z_sched_ipi() doesn't have the same signature as a typical ISR, so
	 * we fudge it with a cast. the argument is ignored, no harm done.
	 */

	x86_irq_funcs[CONFIG_SCHED_IPI_VECTOR - IV_IRQS] =
		(void *) z_sched_ipi;
}

/*
 * it is not clear exactly how/where/why to abstract this, as it
 * assumes the use of a local APIC (but there's no other mechanism).
 */
void z_arch_sched_ipi(void)
{
	z_loapic_ipi(0, LOAPIC_ICR_IPI_OTHERS, CONFIG_SCHED_IPI_VECTOR);
}
#endif
