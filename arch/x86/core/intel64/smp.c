/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <kernel_arch_data.h>
#include <x86_mmu.h>
#include <zephyr/init.h>

#define NR_IRQ_VECTORS (IV_NR_VECTORS - IV_IRQS)  /* # vectors free for IRQs */

extern void (*x86_irq_funcs[NR_IRQ_VECTORS])(const void *arg);
extern const void *x86_irq_args[NR_IRQ_VECTORS];


int arch_smp_init(void)
{
	/*
	 * z_sched_ipi() doesn't have the same signature as a typical ISR, so
	 * we fudge it with a cast. the argument is ignored, no harm done.
	 */

	x86_irq_funcs[CONFIG_SCHED_IPI_VECTOR - IV_IRQS] =
		(void *) z_sched_ipi;

	/* TLB shootdown handling */
	x86_irq_funcs[CONFIG_TLB_IPI_VECTOR - IV_IRQS] = z_x86_tlb_ipi;
	return 0;
}

/*
 * it is not clear exactly how/where/why to abstract this, as it
 * assumes the use of a local APIC (but there's no other mechanism).
 */
void arch_sched_ipi(void)
{
	z_loapic_ipi(0, LOAPIC_ICR_IPI_OTHERS, CONFIG_SCHED_IPI_VECTOR);
}

SYS_INIT(arch_smp_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
