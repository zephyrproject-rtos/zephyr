/*
 * Copyright (c) 2013-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief  system module for variants with LOAPIC
 *
 */

#include <misc/__assert.h>
#include <kernel.h>
#include <arch/cpu.h>
#include <drivers/ioapic.h>
#include <drivers/loapic.h>
#include <drivers/sysapic.h>
#include <irq.h>

#ifdef CONFIG_IOAPIC
#define IS_IOAPIC_IRQ(irq)  (irq < LOAPIC_IRQ_BASE)
#else
#define IS_IOAPIC_IRQ(irq)  0
#endif

#define NR_APIC_IRQ (LOAPIC_IRQ_BASE + LOAPIC_IRQ_COUNT)

/**
 *
 * @brief Program interrupt controller
 *
 * This routine programs the interrupt controller with the given vector
 * based on the given IRQ parameter. If the IRQ is not "wired" (that is,
 * not connected to an APIC of any kind), this function is a no-op.
 *
 * @param vector the vector number
 * @param irq the virtualized IRQ
 * @param flags interrupt flags
 *
 */
void __irq_controller_irq_config(unsigned int vector, unsigned int irq,
				 u32_t flags)
{
	if (irq < NR_APIC_IRQ) {
		if (IS_IOAPIC_IRQ(irq)) {
			z_ioapic_irq_set(irq, vector, flags);
		} else {
			z_loapic_int_vec_set(irq - LOAPIC_IRQ_BASE, vector);
		}
	}
}

/**
 *
 * @brief Enable an individual interrupt (IRQ)
 *
 * The public interface for enabling/disabling a specific IRQ for the IA-32
 * architecture is defined as follows in include/arch/x86/arch.h
 *
 *   extern void  irq_enable  (unsigned int irq);
 *   extern void  irq_disable (unsigned int irq);
 *
 * These routines are no-ops if 'irq' is not wired to an APIC.
 *
 * @return N/A
 */
void z_arch_irq_enable(unsigned int irq)
{
	if (irq < NR_APIC_IRQ) {
		if (IS_IOAPIC_IRQ(irq)) {
			z_ioapic_irq_enable(irq);
		} else {
			z_loapic_irq_enable(irq - LOAPIC_IRQ_BASE);
		}
	}
}

void z_arch_irq_disable(unsigned int irq)
{
	if (irq < NR_APIC_IRQ) {
		if (IS_IOAPIC_IRQ(irq)) {
			z_ioapic_irq_disable(irq);
		} else {
			z_loapic_irq_disable(irq - LOAPIC_IRQ_BASE);
		}
	}
}

