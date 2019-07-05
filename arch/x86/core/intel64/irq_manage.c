/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <kernel_arch_data.h>
#include <drivers/interrupt_controller/sysapic.h>
#include <irq.h>

unsigned char _irq_to_interrupt_vector[CONFIG_MAX_IRQ_LINES];

/*
 * The low-level interrupt code consults these arrays to dispatch IRQs, so
 * so be sure to keep locore.S up to date with any changes. Note the indices:
 * use (vector - IV_IRQS), since exception vectors do not appear here.
 */

#define NR_IRQ_VECTORS (IV_NR_VECTORS - IV_IRQS)  /* # vectors free for IRQs */

void (*x86_irq_funcs[NR_IRQ_VECTORS])(void *);
void *x86_irq_args[NR_IRQ_VECTORS];

/*
 *
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
