/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#include <nanokernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>

#include "board.h"

#include <toolchain.h>
#include <sections.h>

#include <drivers/ioapic.h> /* IO APIC public API declarations. */
#include <drivers/loapic.h> /* Local APIC public API declarations.*/

/* defines */

/* IO APIC direct register offsets */

#define IOAPIC_IND 0x00   /* Index Register - MVIC IOREGSEL register */
#define IOAPIC_DATA 0x10  /* IO window (data) - pc.h */

/* MVIC IOREGSEL register usage defines */
#define MVIC_LOW_NIBBLE_MASK 0x07
#define MVIC_HIGH_NIBBLE_MASK 0x18

/* MVIC Local APIC Vector Table Bits */

#define LOAPIC_VECTOR 0x000000ff /* vectorNo */

/* MVIC Local APIC Spurious-Interrupt Register Bits */

#define LOAPIC_ENABLE 0x100	/* APIC Enabled */

#define LOAPIC_MVIC_ISR 0x110 /* MVIC In-Service Register offset */

/* forward declarations */

static void _mvic_rte_set(unsigned int irq, uint32_t value);
static uint32_t _mvic_rte_get(unsigned int irq);
static void _mvic_rte_update(unsigned int irq, uint32_t value,
		uint32_t mask);

/*
 * The functions irq_enable() and irq_disable() are implemented
 * in the platforms that incorporate this interrupt controller driver due to the
 * IRQ virtualization imposed by the platform.
 */

/**
 *
 * @brief initialize the MVIC IO APIC and local APIC register sets.
 *
 * This routine initializes the Quark D2000 Interrupt Controller (MVIC).
 * This routine replaces the standard Local APIC / IO APIC init routines.
 *
 * @returns: N/A
 */
int _mvic_init(struct device *unused)
{
	ARG_UNUSED(unused);
	int32_t ix;	/* Interrupt line register index */
	uint32_t rteValue; /* value to copy into interrupt line register */

	/*
	 * The platform must define the CONFIG_IOAPIC_NUM_RTES macro to indicate the number
	 * of redirection table entries supported by the IOAPIC on the board.
	 *

	 * By default mask all interrupt lines and set default sensitivity to edge.
	 *
	 */

	rteValue = IOAPIC_EDGE | IOAPIC_INT_MASK;

	for (ix = 0; ix < CONFIG_IOAPIC_NUM_RTES; ix++) {
		_mvic_rte_set(ix, rteValue);
	}

	/* enable the MVIC Local APIC */

	_loapic_enable();

	/* reset the TPR, and TIMER_ICR */

	*(volatile int *)(CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_TPR) = (int)0x0;
	*(volatile int *)(CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_TIMER_ICR) = (int)0x0;

	/* program Local Vector Table for the Virtual Wire Mode */

	/* lock the MVIC timer interrupt, set which IRQ line should be
	 * used for timer interrupts (this is unlike LOAPIC where the
	 * vector is programmed instead).
	 */
	__ASSERT_NO_MSG(CONFIG_LOAPIC_TIMER_IRQ <= 15);
	*(volatile int *)(CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_TIMER) =
		LOAPIC_LVT_MASKED | CONFIG_LOAPIC_TIMER_IRQ;

	/* discard a pending interrupt if any */

	*(volatile int *)(CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_EOI) = 0;

	return 0;

}

/**
 *
 * @brief Send EOI (End Of Interrupt) signal to IO APIC
 *
 * This routine sends an EOI signal to the IO APIC's interrupting source.
 *
 * All line interrupts on Quark D2000 are EOI'ed with local APIC EOI register.
 *
 * @param irq INT number to send EOI
 *
 * @returns: N/A
 */
void _ioapic_eoi(unsigned int irq)
{
	_loapic_eoi();
}

/**
 *
 * @brief Get EOI (End Of Interrupt) information
 *
 * This routine returns EOI signalling information for a specific IRQ.
 *
 * @param irq INTIN number of interest
 * @param argRequired ptr to "argument required" result area
 * @param arg ptr to "argument value" result area
 *
 * @returns: address of routine to be called to signal EOI;
 *          as a side effect, also passes back indication if routine requires
 *          an interrupt vector argument and what the argument value should be
 */
void *_ioapic_eoi_get(unsigned int irq, char *argRequired, void **arg)
{

	/* indicate that an argument to the EOI handler is required */

	*argRequired = 1;

	/*
	 * The parameter to the ioApicIntEoi() routine is the vector programmed
	 * into the redirection table.  The platform _SysIntVecAlloc() routine
	 * must invoke _IoApicIntEoiGet() after _IoApicRedVecSet() to ensure the
	 * redirection table contains the desired interrupt vector.
	 *
	 * Vectors fixed on this CPU Arch, no memory location on this CPU
	 * arch with this information.
	 */

	*arg = NULL;


	/* lo eoi always used on this CPU arch. */

	return _loapic_eoi;
}

/**
 *
 * @brief Enable a specified APIC interrupt input line
 *
 * This routine enables a specified APIC interrupt input line.
 *
 * @param irq INTIN number to enable
 *
 * @returns: N/A
 */
void _ioapic_irq_enable(unsigned int irq)
{
	_mvic_rte_update(irq, 0, IOAPIC_INT_MASK);
}

/**
 *
 * @brief disable a specified APIC interrupt input line
 *
 * This routine disables a specified APIC interrupt input line.
 *
 * @param irq INTIN number to disable
 *
 * @returns: N/A
 */
void _ioapic_irq_disable(unsigned int irq)
{
	_mvic_rte_update(irq, IOAPIC_INT_MASK, IOAPIC_INT_MASK);
}

/**
 *
 * @brief Programs Rte interrupt line register.
 *
 * Always mask interrupt as part of programming like standard IOAPIC
 * version of this routine.
 * Vector is fixed by this HW and is unused.
 * Or in flags for trigger bit.
 *
 * @param irq Virtualized IRQ
 * @param vector Vector Number
 * @param flags Interrupt flags
 *
 * @returns: N/A
 */
void _ioapic_irq_set(unsigned int irq, unsigned int vector, uint32_t flags)
{
	uint32_t rteValue;   /* value to copy into Rte register */

	ARG_UNUSED(vector);

	rteValue = IOAPIC_INT_MASK | flags;
	_mvic_rte_set(irq, rteValue);
}

/**
 *
 * @brief program interrupt vector for specified irq
 *
 * Fixed vector on this HW. Nothing to do.
 *
 * @param irq Interrupt number
 * @param vector Vector number
 *
 * @returns: N/A
 */
void _ioapic_int_vec_set(unsigned int irq, unsigned int vector)
{
	ARG_UNUSED(irq);
	ARG_UNUSED(vector);
}

/**
 *
 * @brief Enable the MVIC Local APIC
 *
 * This routine enables the MVIC Local APIC.
 *
 * @returns: N/A
 */
void _loapic_enable(void)
{
	int32_t oldLevel = irq_lock(); /* LOCK INTERRUPTS */

	*(volatile int *)(CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_SVR) |= LOAPIC_ENABLE;

	irq_unlock(oldLevel); /* UNLOCK INTERRUPTS */
}

/**
 *
 * @brief Disable the MVIC Local APIC.
 *
 * This routine disables the MVIC Local APIC.
 *
 * @returns: N/A
 */
void _loapic_disable(void)
{
	int32_t oldLevel = irq_lock(); /* LOCK INTERRUPTS */

	*(volatile int *)(CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_SVR) &= ~LOAPIC_ENABLE;

	irq_unlock(oldLevel); /* UNLOCK INTERRUPTS */
}


/**
 *
 * @brief Set the vector field in the specified RTE
 *
 * Fixed vectors on this HW. Nothing to do.
 *
 * @param irq IRQ number of the interrupt
 * @param vector vector to copy into the LVT
 *
 * @returns N/A
 */
void _loapic_int_vec_set(unsigned int irq, unsigned int vector)
{
	ARG_UNUSED(irq);
	ARG_UNUSED(vector);
}

/**
 *
 * @brief enable an individual LOAPIC interrupt (IRQ)
 *
 * This routine clears the interrupt mask bit in the LVT for the specified IRQ
 *
 * @param irq IRQ number of the interrupt
 *
 * @returns N/A
 */
void _loapic_irq_enable(unsigned int irq)
{
	volatile int *pLvt; /* pointer to local vector table */
	int32_t oldLevel;   /* previous interrupt lock level */

	/*
	 * irq is actually an index to local APIC LVT register.
	 * ASSERT if out of range for MVIC implementation.
	 */
	__ASSERT_NO_MSG(irq < LOAPIC_IRQ_COUNT);

	/*
	 * See the comments in _LoApicLvtVecSet() regarding IRQ to LVT mappings
	 * and ths assumption concerning LVT spacing.
	 */

	pLvt = (volatile int *)(CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_TIMER +
			(irq * LOAPIC_LVT_REG_SPACING));

	/* clear the mask bit in the LVT */

	oldLevel = irq_lock();
	*pLvt = *pLvt & ~LOAPIC_LVT_MASKED;
	irq_unlock(oldLevel);

}

/**
 *
 * @brief disable an individual LOAPIC interrupt (IRQ)
 *
 * This routine clears the interrupt mask bit in the LVT for the specified IRQ
 *
 * @param irq IRQ number of the interrupt
 *
 * @returns N/A
 */
void _loapic_irq_disable(unsigned int irq)
{
	volatile int *pLvt; /* pointer to local vector table */
	int32_t oldLevel;   /* previous interrupt lock level */

	/*
	 * irq is actually an index to local APIC LVT register.
	 * ASSERT if out of range for MVIC implementation.
	 */
	__ASSERT_NO_MSG(irq < LOAPIC_IRQ_COUNT);

	/*
	 * See the comments in _LoApicLvtVecSet() regarding IRQ to LVT mappings
	 * and ths assumption concerning LVT spacing.
	 */

	pLvt = (volatile int *)(CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_TIMER +
			(irq * LOAPIC_LVT_REG_SPACING));

	/* set the mask bit in the LVT */

	oldLevel = irq_lock();
	*pLvt = *pLvt | LOAPIC_LVT_MASKED;
	irq_unlock(oldLevel);

}

/**
 *
 * @brief read a 32 bit MVIC IO APIC register
 *
 * @param irq INTIN number
 *
 * @returns register value
 */
static uint32_t _mvic_rte_get(unsigned int irq)
{
	uint32_t value; /* value */
	int key;	/* interrupt lock level */
	volatile unsigned int *rte;
	volatile unsigned int *index;
	unsigned int low_nibble;
	unsigned int high_nibble;

	index = (unsigned int *)(CONFIG_IOAPIC_BASE_ADDRESS + IOAPIC_IND);
	rte = (unsigned int *)(CONFIG_IOAPIC_BASE_ADDRESS + IOAPIC_DATA);

	/* Set index in the IOREGSEL */
	__ASSERT(irq < CONFIG_IOAPIC_NUM_RTES, "INVL");

	low_nibble = ((irq & MVIC_LOW_NIBBLE_MASK) << 0x1);
	high_nibble = ((irq & MVIC_HIGH_NIBBLE_MASK) << 0x2);

	/* lock interrupts to ensure indirect addressing works "atomically" */

	key = irq_lock();

	*(index) = high_nibble | low_nibble;
	value = *(rte);

	irq_unlock(key);

	return value;
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
static void _mvic_rte_set(unsigned int irq, uint32_t value)
{
	int key; /* interrupt lock level */
	volatile unsigned int *rte;
	volatile unsigned int *index;
	unsigned int low_nibble;
	unsigned int high_nibble;

	index = (unsigned int *)(CONFIG_IOAPIC_BASE_ADDRESS + IOAPIC_IND);
	rte = (unsigned int *)(CONFIG_IOAPIC_BASE_ADDRESS + IOAPIC_DATA);

	/* Set index in the IOREGSEL */
	__ASSERT(irq < CONFIG_IOAPIC_NUM_RTES, "INVL");

	low_nibble = ((irq & MVIC_LOW_NIBBLE_MASK) << 0x1);
	high_nibble = ((irq & MVIC_HIGH_NIBBLE_MASK) << 0x2);

	/* lock interrupts to ensure indirect addressing works "atomically" */

	key = irq_lock();

	*(index) = high_nibble | low_nibble;
	*(rte) = (value & IOAPIC_LO32_RTE_SUPPORTED_MASK);

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
static void _mvic_rte_update(unsigned int irq, uint32_t value, uint32_t mask)
{
	_mvic_rte_set(irq, (_mvic_rte_get(irq) & ~mask) | (value & mask));
}

/**
 * @brief Find the currently executing interrupt vector, if any
 *
 * This routine finds the vector of the interrupt that is being processed.
 * The ISR (In-Service Register) register contain the vectors of the interrupts
 * in service. And the higher vector is the indentification of the interrupt
 * being currently processed.
 *
 * MVIC ISR registers' offsets:
 * --------------------
 * | Offset | bits    |
 * --------------------
 * | 0110H  |  32:63  |
 * --------------------
 *
 * @return The vector of the interrupt that is currently being processed.
 */
int _loapic_isr_vector_get(void)
{
	/* pointer to ISR vector table */
	volatile int *pReg;

	pReg = (volatile int *)(CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_MVIC_ISR);

	return 32 + (find_msb_set(*pReg) - 1);
}
