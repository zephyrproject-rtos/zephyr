/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Quark D2000 Interrupt Controller (MVIC)
 *
 * This module is based on the standard Local APIC and IO APIC source modules.
 * This modules combines these modules into one source module that exports the
 * same APIs defined by the Local APIC and IO APIC header modules. These
 * routine have been adapted for the Quark D2000 Interrupt Controller which has
 * a cutdown implementation of the Local APIC & IO APIC register sets.
 *
 * The MVIC (Quark D2000 Interrupt Controller) is configured by default
 * to support 32 external interrupt lines.
 * Unlike the traditional IA LAPIC/IOAPIC, the interrupt vectors in MVIC are fixed
 * and not programmable.
 * The larger the vector number, the higher the priority of the interrupt.
 * Higher priority interrupts preempt lower priority interrupts.
 * Lower priority interrupts do not preempt higher priority interrupts.
 * The MVIC holds the lower priority interrupts pending until the interrupt
 * service routine for the higher priority interrupt writes to the End of
 * Interrupt (EOI) register.
 * After an EOI write, the MVIC asserts the next highest pending interrupt.
 *
 * INCLUDE FILES: ioapic.h loapic.h
 *
 */

/* includes */

#include <kernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>
#include <misc/util.h>
#include <init.h>
#include <arch/x86/irq_controller.h>
#include <inttypes.h>

static inline u32_t compute_ioregsel(unsigned int irq)
{
	unsigned int low_nibble;
	unsigned int high_nibble;

	__ASSERT(irq < MVIC_NUM_RTES, "invalid irq line %d", irq);

	low_nibble = ((irq & MVIC_LOW_NIBBLE_MASK) << 0x1);
	high_nibble = ((irq & MVIC_HIGH_NIBBLE_MASK) << 0x2);
	return low_nibble | high_nibble;
}


/**
 *
 * @brief write to 32 bit MVIC IO APIC register
 *
 * @param irq INTIN number
 * @param value value to be written
 *
 * @returns N/A
 */
static void _mvic_rte_set(unsigned int irq, u32_t value)
{
	unsigned int key; /* interrupt lock level */
	u32_t regsel;

	__ASSERT(!(value & ~MVIC_IOWIN_SUPPORTED_BITS_MASK),
		 "invalid IRQ flags %" PRIx32 " for irq %d", value, irq);

	regsel = compute_ioregsel(irq);

	/* lock interrupts to ensure indirect addressing works "atomically" */
	key = irq_lock();

	sys_write32(regsel, MVIC_IOREGSEL);
	sys_write32(value, MVIC_IOWIN);

	irq_unlock(key);
}


/**
 *
 * @brief modify interrupt line register.
 *
 * @param irq INTIN number
 * @param value value to be written
 * @param mask of bits to be modified
 *
 * @returns N/A
 */
static void _mvic_rte_update(unsigned int irq, u32_t value, u32_t mask)
{
	unsigned int key;
	u32_t regsel, old_value, updated_value;

	__ASSERT(!(value & ~MVIC_IOWIN_SUPPORTED_BITS_MASK),
		 "invalid IRQ flags %" PRIx32 " for irq %d", value, irq);

	regsel = compute_ioregsel(irq);

	key = irq_lock();

	sys_write32(regsel, MVIC_IOREGSEL);

	old_value = sys_read32(MVIC_IOWIN);
	updated_value = (old_value & ~mask) | (value & mask);
	sys_write32(updated_value, MVIC_IOWIN);

	irq_unlock(key);
}


/**
 *
 * @brief initialize the MVIC IO APIC and local APIC register sets.
 *
 * This routine initializes the Quark D2000 Interrupt Controller (MVIC).
 * This routine replaces the standard Local APIC / IO APIC init routines.
 *
 * @returns: N/A
 */
static int _mvic_init(struct device *unused)
{
	ARG_UNUSED(unused);
	int i;

	/* By default mask all interrupt lines */
	for (i = 0; i < MVIC_NUM_RTES; i++) {
		_mvic_rte_set(i, MVIC_IOWIN_MASK);
	}

	/* reset the task priority and timer initial count registers */
	sys_write32(0, MVIC_TPR);
	sys_write32(0, MVIC_ICR);

	/* Initialize and mask the timer interrupt.
	 * Bits 0-3 program the interrupt line number we will use
	 * for the timer interrupt.
	 */
	__ASSERT(CONFIG_MVIC_TIMER_IRQ < 16,
		 "Bad irq line %d chosen for timer irq", CONFIG_MVIC_TIMER_IRQ);
	sys_write32(MVIC_LVTTIMER_MASK | CONFIG_MVIC_TIMER_IRQ, MVIC_LVTTIMER);

	/* discard a pending interrupt if any */
	sys_write32(0, MVIC_EOI);

	return 0;

}
SYS_INIT(_mvic_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);


void _arch_irq_enable(unsigned int irq)
{
	if (irq == CONFIG_MVIC_TIMER_IRQ) {
		sys_write32(sys_read32(MVIC_LVTTIMER) & ~MVIC_LVTTIMER_MASK,
			    MVIC_LVTTIMER);
	} else {
		_mvic_rte_update(irq, 0, MVIC_IOWIN_MASK);
	}
}


void _arch_irq_disable(unsigned int irq)
{
	if (irq == CONFIG_MVIC_TIMER_IRQ) {
		sys_write32(sys_read32(MVIC_LVTTIMER) | MVIC_LVTTIMER_MASK,
			    MVIC_LVTTIMER);
	} else {
		_mvic_rte_update(irq, MVIC_IOWIN_MASK, MVIC_IOWIN_MASK);
	}
}


void __irq_controller_irq_config(unsigned int vector, unsigned int irq,
				 u32_t flags)
{
	ARG_UNUSED(vector);

	/* Vector argument always ignored. There are no triggering options
	 * for the timer, so nothing to do at all for that case. Other I/O
	 * interrupts need their triggering set
	 */
	if (irq != CONFIG_MVIC_TIMER_IRQ) {
		_mvic_rte_set(irq, MVIC_IOWIN_MASK | flags);
	} else {
		__ASSERT(flags == 0,
			 "Timer interrupt cannot have triggering flags set");
	}
}


/**
 * @brief Find the currently executing interrupt vector, if any
 *
 * This routine finds the vector of the interrupt that is being processed.
 * The ISR (In-Service Register) register contain the vectors of the interrupts
 * in service. And the higher vector is the identification of the interrupt
 * being currently processed.
 *
 * MVIC ISR registers' offsets:
 * --------------------
 * | Offset | bits    |
 * --------------------
 * | 0110H  |  32:63  |
 * --------------------
 *
 * @return The vector of the interrupt that is currently being processed, or
 * -1 if this can't be determined
 */
int __irq_controller_isr_vector_get(void)
{
	/* In-service register value */
	int isr;

	isr = sys_read32(MVIC_ISR);
	if (unlikely(!isr)) {
		return -1;
	}
	return 32 + (find_msb_set(isr) - 1);
}
