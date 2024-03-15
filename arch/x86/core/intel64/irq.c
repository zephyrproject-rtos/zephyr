/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/arch/cpu.h>
#include <kernel_arch_data.h>
#include <kernel_arch_func.h>
#include <zephyr/drivers/interrupt_controller/sysapic.h>
#include <zephyr/drivers/interrupt_controller/loapic.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/iterable_sections.h>


LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

unsigned char _irq_to_interrupt_vector[CONFIG_MAX_IRQ_LINES];
#define NR_IRQ_VECTORS (IV_NR_VECTORS - IV_IRQS)  /* # vectors free for IRQs */

void (*x86_irq_funcs[NR_IRQ_VECTORS])(const void *arg);
const void *x86_irq_args[NR_IRQ_VECTORS];

#if defined(CONFIG_INTEL_VTD_ICTL)

#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/intel_vtd.h>

static const struct device *const vtd = DEVICE_DT_GET_ONE(intel_vt_d);

#endif /* CONFIG_INTEL_VTD_ICTL */

static void irq_spurious(const void *arg)
{
	LOG_ERR("Spurious interrupt, vector %d\n", (uint32_t)(uint64_t)arg);
	z_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

void x86_64_irq_init(void)
{
	for (int i = 0; i < NR_IRQ_VECTORS; i++) {
		x86_irq_funcs[i] = irq_spurious;
		x86_irq_args[i] = (const void *)(long)(i + IV_IRQS);
	}
}

int z_x86_allocate_vector(unsigned int priority, int prev_vector)
{
	const int VECTORS_PER_PRIORITY = 16;
	const int MAX_PRIORITY = 13;
	int vector = prev_vector;
	int i;

	if (priority >= MAX_PRIORITY) {
		priority = MAX_PRIORITY;
	}

	if (vector == -1) {
		vector = (priority * VECTORS_PER_PRIORITY) + IV_IRQS;
	}

	for (i = 0; i < VECTORS_PER_PRIORITY; ++i, ++vector) {
		if (prev_vector != 1 && vector == prev_vector) {
			continue;
		}

#ifdef CONFIG_IRQ_OFFLOAD
		if (vector == CONFIG_IRQ_OFFLOAD_VECTOR) {
			continue;
		}
#endif
		if (vector == Z_X86_OOPS_VECTOR) {
			continue;
		}

		if (x86_irq_funcs[vector - IV_IRQS] == irq_spurious) {
			return vector;
		}
	}

	return -1;
}

void z_x86_irq_connect_on_vector(unsigned int irq,
				 uint8_t vector,
				 void (*func)(const void *arg),
				 const void *arg)
{
	_irq_to_interrupt_vector[irq] = vector;
	x86_irq_funcs[vector - IV_IRQS] = func;
	x86_irq_args[vector - IV_IRQS] = arg;
}

/*
 * N.B.: the API docs don't say anything about returning error values, but
 * this function returns -1 if a vector at the specific priority can't be
 * allocated. Whether it should simply __ASSERT instead is up for debate.
 */

int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*func)(const void *arg),
			     const void *arg, uint32_t flags)
{
	uint32_t key;
	int vector;

	__ASSERT(irq <= CONFIG_MAX_IRQ_LINES, "IRQ %u out of range", irq);

	key = irq_lock();

	vector = z_x86_allocate_vector(priority, -1);
	if (vector >= 0) {
#if defined(CONFIG_INTEL_VTD_ICTL)
		if (device_is_ready(vtd)) {
			int irte = vtd_allocate_entries(vtd, 1);

			__ASSERT(irte >= 0, "IRTE allocation must succeed");

			vtd_set_irte_vector(vtd, irte, vector);
			vtd_set_irte_irq(vtd, irte, irq);
		}
#endif /* CONFIG_INTEL_VTD_ICTL */

		z_irq_controller_irq_config(vector, irq, flags);
		z_x86_irq_connect_on_vector(irq, vector, func, arg);
	}

	irq_unlock(key);
	return vector;
}


/* The first bit is used to indicate whether the list of reserved interrupts
 * have been initialized based on content stored in the irq_alloc linker
 * section in ROM.
 */
#define IRQ_LIST_INITIALIZED 0

static ATOMIC_DEFINE(irq_reserved, CONFIG_MAX_IRQ_LINES);

static void irq_init(void)
{
	TYPE_SECTION_FOREACH(const uint8_t, irq_alloc, irq) {
		__ASSERT_NO_MSG(*irq < CONFIG_MAX_IRQ_LINES);
		atomic_set_bit(irq_reserved, *irq);
	}
}

unsigned int arch_irq_allocate(void)
{
	unsigned int key = irq_lock();
	int i;

	if (!atomic_test_and_set_bit(irq_reserved, IRQ_LIST_INITIALIZED)) {
		irq_init();
	}

	for (i = 0; i < ARRAY_SIZE(irq_reserved); i++) {
		unsigned int fz, irq;

		while ((fz = find_lsb_set(~atomic_get(&irq_reserved[i])))) {
			irq = (fz - 1) + (i * sizeof(atomic_val_t) * 8);
			if (irq >= CONFIG_MAX_IRQ_LINES) {
				break;
			}

			if (!atomic_test_and_set_bit(irq_reserved, irq)) {
				irq_unlock(key);
				return irq;
			}
		}
	}

	irq_unlock(key);

	return UINT_MAX;
}

void arch_irq_set_used(unsigned int irq)
{
	unsigned int key = irq_lock();

	atomic_set_bit(irq_reserved, irq);

	irq_unlock(key);
}

bool arch_irq_is_used(unsigned int irq)
{
	return atomic_test_bit(irq_reserved, irq);
}
