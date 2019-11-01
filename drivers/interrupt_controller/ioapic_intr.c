/*
 * Copyright (c) 1997-1998, 2000-2002, 2004, 2006-2008, 2011-2015 Wind River
 * Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Intel IO APIC/xAPIC driver
 *
 * This module is a driver for the IO APIC/xAPIC (Advanced Programmable
 * Interrupt Controller) for P6 (PentiumPro, II, III) family processors
 * and P7 (Pentium4) family processors.  The IO APIC/xAPIC is included
 * in the Intel's system chip set, such as ICH2.  Software intervention
 * may be required to enable the IO APIC/xAPIC in some chip sets.
 * The 8259A interrupt controller is intended for use in a uni-processor
 * system, IO APIC can be used in either a uni-processor or multi-processor
 * system.  The IO APIC handles interrupts very differently than the 8259A.
 * Briefly, these differences are:
 *  - Method of Interrupt Transmission. The IO APIC transmits interrupts
 *    through a 3-wire bus and interrupts are handled without the need for
 *    the processor to run an interrupt acknowledge cycle.
 *  - Interrupt Priority. The priority of interrupts in the IO APIC is
 *    independent of the interrupt number.  For example, interrupt 10 can
 *    be given a higher priority than interrupt 3.
 *  - More Interrupts. The IO APIC supports a total of 24 interrupts.
 *
 * The IO APIC unit consists of a set of interrupt input signals, a 24-entry
 * by 64-bit Interrupt Redirection Table, programmable registers, and a message
 * unit for sending and receiving APIC messages over the APIC bus or the
 * Front-Side (system) bus.  IO devices inject interrupts into the system by
 * asserting one of the interrupt lines to the IO APIC.  The IO APIC selects the
 * corresponding entry in the Redirection Table and uses the information in that
 * entry to format an interrupt request message.  Each entry in the Redirection
 * Table can be individually programmed to indicate edge/level sensitive interrupt
 * signals, the interrupt vector and priority, the destination processor, and how
 * the processor is selected (statically and dynamically).  The information in
 * the table is used to transmit a message to other APIC units (via the APIC bus
 * or the Front-Side (system) bus).  IO APIC is used in the Symmetric IO Mode.
 * The base address of IO APIC is determined in loapic_init() and stored in the
 * global variable ioApicBase and ioApicData.
 * The lower 32 bit value of the redirection table entries for IRQ 0
 * to 15 are edge triggered positive high, and for IRQ 16 to 23 are level
 * triggered positive low.
 *
 * This implementation doesn't support multiple IO APICs.
 *
 * INCLUDE FILES: ioapic.h loapic.h
 *
 */

#include <kernel.h>
#include <arch/cpu.h>

#include <toolchain.h>
#include <linker/sections.h>
#include <init.h>
#include <string.h>

#include <drivers/interrupt_controller/ioapic.h> /* public API declarations */
#include <drivers/interrupt_controller/loapic.h> /* public API declarations and registers */
#include "ioapic_priv.h"

#define BITS_PER_IRQ  3
#define IOAPIC_BITFIELD_HI_LO	0
#define IOAPIC_BITFIELD_LVL_EDGE 1
#define IOAPIC_BITFIELD_ENBL_DSBL 2
#define BIT_POS_FOR_IRQ_OPTION(irq, option) ((irq) * BITS_PER_IRQ + (option))
#define SUSPEND_BITS_REQD (ROUND_UP((CONFIG_IOAPIC_NUM_RTES * BITS_PER_IRQ), 32))

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
#include <power/power.h>
u32_t ioapic_suspend_buf[SUSPEND_BITS_REQD / 32] = {0};
static u32_t ioapic_device_power_state = DEVICE_PM_ACTIVE_STATE;
#endif

static u32_t __IoApicGet(s32_t offset);
static void __IoApicSet(s32_t offset, u32_t value);
static void ioApicRedSetHi(unsigned int irq, u32_t upper32);
static void ioApicRedSetLo(unsigned int irq, u32_t lower32);
static u32_t ioApicRedGetLo(unsigned int irq);
static void IoApicRedUpdateLo(unsigned int irq, u32_t value,
					u32_t mask);

/*
 * The functions irq_enable() and irq_disable() are implemented in the
 * interrupt controller driver due to the IRQ virtualization imposed by
 * the x86 architecture.
 */

/**
 *
 * @brief Initialize the IO APIC or xAPIC
 *
 * This routine initializes the IO APIC or xAPIC.
 *
 * @return N/A
 */
int _ioapic_init(struct device *unused)
{
	ARG_UNUSED(unused);
#ifdef CONFIG_IOAPIC_MASK_RTE
	s32_t ix;	/* redirection table index */
	u32_t rteValue; /* value to copy into redirection table entry */

	/*
	 * The platform must set the Kconfig option IOAPIC_NUM_RTES to indicate
	 * the number of redirection table entries supported by the IOAPIC.
	 *
	 * Note: The number of actual IRQs supported by the IOAPIC can be
	 * determined at runtime by computing:
	 *
	 * ((__IoApicGet(IOAPIC_VERS) & IOAPIC_MRE_MASK) >> 16) + 1
	 */
	rteValue = IOAPIC_EDGE | IOAPIC_HIGH | IOAPIC_FIXED | IOAPIC_INT_MASK |
		   IOAPIC_PHYSICAL | 0 /* dummy vector */;

	for (ix = 0; ix < CONFIG_IOAPIC_NUM_RTES; ix++) {
		ioApicRedSetHi(ix, 0xFF000000);
		ioApicRedSetLo(ix, rteValue);
	}
#endif
	return 0;
}

/**
 *
 * @brief Enable a specified APIC interrupt input line
 *
 * This routine enables a specified APIC interrupt input line.
 * @param irq IRQ number to enable
 *
 * @return N/A
 */
void z_ioapic_irq_enable(unsigned int irq)
{
	IoApicRedUpdateLo(irq, 0, IOAPIC_INT_MASK);
}

/**
 *
 * @brief Disable a specified APIC interrupt input line
 *
 * This routine disables a specified APIC interrupt input line.
 * @param irq IRQ number to disable
 *
 * @return N/A
 */
void z_ioapic_irq_disable(unsigned int irq)
{
	IoApicRedUpdateLo(irq, IOAPIC_INT_MASK, IOAPIC_INT_MASK);
}


#ifdef CONFIG_DEVICE_POWER_MANAGEMENT

void store_flags(unsigned int irq, u32_t flags)
{
	/* Currently only the following three flags are modified */
	if (flags & IOAPIC_LOW) {
		sys_bitfield_set_bit((mem_addr_t) ioapic_suspend_buf,
			BIT_POS_FOR_IRQ_OPTION(irq, IOAPIC_BITFIELD_HI_LO));
	}

	if (flags & IOAPIC_LEVEL) {
		sys_bitfield_set_bit((mem_addr_t) ioapic_suspend_buf,
			BIT_POS_FOR_IRQ_OPTION(irq, IOAPIC_BITFIELD_LVL_EDGE));
	}

	if (flags & IOAPIC_INT_MASK) {
		sys_bitfield_set_bit((mem_addr_t) ioapic_suspend_buf,
			BIT_POS_FOR_IRQ_OPTION(irq, IOAPIC_BITFIELD_ENBL_DSBL));
	}
}

u32_t restore_flags(unsigned int irq)
{
	u32_t flags = 0U;

	if (sys_bitfield_test_bit((mem_addr_t) ioapic_suspend_buf,
		BIT_POS_FOR_IRQ_OPTION(irq, IOAPIC_BITFIELD_HI_LO))) {
		flags |= IOAPIC_LOW;
	}

	if (sys_bitfield_test_bit((mem_addr_t) ioapic_suspend_buf,
		BIT_POS_FOR_IRQ_OPTION(irq, IOAPIC_BITFIELD_LVL_EDGE))) {
		flags |= IOAPIC_LEVEL;
	}

	if (sys_bitfield_test_bit((mem_addr_t) ioapic_suspend_buf,
		BIT_POS_FOR_IRQ_OPTION(irq, IOAPIC_BITFIELD_ENBL_DSBL))) {
		flags |= IOAPIC_INT_MASK;
	}

	return flags;
}


int ioapic_suspend(struct device *port)
{
	int irq;
	u32_t rte_lo;

	ARG_UNUSED(port);
	(void)memset(ioapic_suspend_buf, 0, (SUSPEND_BITS_REQD >> 3));
	for (irq = 0; irq < CONFIG_IOAPIC_NUM_RTES; irq++) {
		/*
		 * The following check is to figure out the registered
		 * IRQ lines, so as to limit ourselves to saving the
		 *  flags for them only.
		 */
		if (_irq_to_interrupt_vector[irq]) {
			rte_lo = ioApicRedGetLo(irq);
			store_flags(irq, rte_lo);
		}
	}
	ioapic_device_power_state = DEVICE_PM_SUSPEND_STATE;
	return 0;
}

int ioapic_resume_from_suspend(struct device *port)
{
	int irq;
	u32_t flags;
	u32_t rteValue;

	ARG_UNUSED(port);

	for (irq = 0; irq < CONFIG_IOAPIC_NUM_RTES; irq++) {
		if (_irq_to_interrupt_vector[irq]) {
			/* Get the saved flags */
			flags = restore_flags(irq);
			/* Appending the flags that are never modified */
			flags = flags | IOAPIC_FIXED | IOAPIC_PHYSICAL;

			rteValue = (_irq_to_interrupt_vector[irq] &
					IOAPIC_VEC_MASK) | flags;
		} else {
			/* Initialize the other RTEs to sane values */
			rteValue = IOAPIC_EDGE | IOAPIC_HIGH |
				IOAPIC_FIXED | IOAPIC_INT_MASK |
				IOAPIC_PHYSICAL | 0 ; /* dummy vector*/
		}
		ioApicRedSetHi(irq, 0xFF000000);
		ioApicRedSetLo(irq, rteValue);
	}
	ioapic_device_power_state = DEVICE_PM_ACTIVE_STATE;
	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int ioapic_device_ctrl(struct device *device, u32_t ctrl_command,
			      void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			ret = ioapic_suspend(device);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			ret = ioapic_resume_from_suspend(device);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = ioapic_device_power_state;
	}

	if (cb) {
		cb(device, ret, context, arg);
	}

	return ret;
}


#endif  /*CONFIG_DEVICE_POWER_MANAGEMENT*/

/**
 *
 * @brief Programs the interrupt redirection table
 *
 * This routine sets up the redirection table entry for the specified IRQ
 * @param irq Virtualized IRQ
 * @param vector Vector number
 * @param flags Interrupt flags
 *
 * @return N/A
 */
void z_ioapic_irq_set(unsigned int irq, unsigned int vector, u32_t flags)
{
	u32_t rteValue;   /* value to copy into redirection table entry */

	rteValue = IOAPIC_FIXED | IOAPIC_INT_MASK | IOAPIC_PHYSICAL |
		   (vector & IOAPIC_VEC_MASK) | flags;
	ioApicRedSetHi(irq, 0xFF000000);
	ioApicRedSetLo(irq, rteValue);
}

/**
 *
 * @brief Program interrupt vector for specified irq
 *
 * The routine writes the interrupt vector in the Interrupt Redirection
 * Table for specified irq number
 *
 * @param irq Interrupt number
 * @param vector Vector number
 * @return N/A
 */
void z_ioapic_int_vec_set(unsigned int irq, unsigned int vector)
{
	IoApicRedUpdateLo(irq, vector, IOAPIC_VEC_MASK);
}

/**
 *
 * @brief Read a 32 bit IO APIC register
 *
 * This routine reads the specified IO APIC register using indirect addressing.
 * @param offset Register offset (8 bits)
 *
 * @return register value
 */
static u32_t __IoApicGet(s32_t offset)
{
	u32_t value; /* value */
	unsigned int key;	/* interrupt lock level */

	/* lock interrupts to ensure indirect addressing works "atomically" */

	key = irq_lock();

	*((volatile u32_t *)
		(DT_IOAPIC_BASE_ADDRESS + IOAPIC_IND)) = (char)offset;
	value = *((volatile u32_t *)(DT_IOAPIC_BASE_ADDRESS + IOAPIC_DATA));

	irq_unlock(key);

	return value;
}

/**
 *
 * @brief Write a 32 bit IO APIC register
 *
 * This routine writes the specified IO APIC register using indirect addressing.
 *
 * @param offset Register offset (8 bits)
 * @param value Value to set the register
 * @return N/A
 */
static void __IoApicSet(s32_t offset, u32_t value)
{
	unsigned int key; /* interrupt lock level */

	/* lock interrupts to ensure indirect addressing works "atomically" */

	key = irq_lock();

	*(volatile u32_t *)(DT_IOAPIC_BASE_ADDRESS + IOAPIC_IND) = (char)offset;
	*((volatile u32_t *)(DT_IOAPIC_BASE_ADDRESS + IOAPIC_DATA)) = value;

	irq_unlock(key);
}

/**
 *
 * @brief Get low 32 bits of Redirection Table entry
 *
 * This routine reads the low-order 32 bits of a Redirection Table entry.
 *
 * @param irq INTIN number
 * @return 32 low-order bits
 */
static u32_t ioApicRedGetLo(unsigned int irq)
{
	s32_t offset = IOAPIC_REDTBL + (irq << 1); /* register offset */

	return __IoApicGet(offset);
}

/**
 *
 * @brief Set low 32 bits of Redirection Table entry
 *
 * This routine writes the low-order 32 bits of a Redirection Table entry.
 *
 * @param irq INTIN number
 * @param lower32 Value to be written
 * @return N/A
 */
static void ioApicRedSetLo(unsigned int irq, u32_t lower32)
{
	s32_t offset = IOAPIC_REDTBL + (irq << 1); /* register offset */

	__IoApicSet(offset, lower32);
}

/**
 *
 * @brief Set high 32 bits of Redirection Table entry
 *
 * This routine writes the high-order 32 bits of a Redirection Table entry.
 *
 * @param irq INTIN number
 * @param upper32 Value to be written
 * @return N/A
 */
static void ioApicRedSetHi(unsigned int irq, u32_t upper32)
{
	s32_t offset = IOAPIC_REDTBL + (irq << 1) + 1; /* register offset */

	__IoApicSet(offset, upper32);
}

/**
 *
 * @brief Modify low 32 bits of Redirection Table entry
 *
 * This routine modifies selected portions of the low-order 32 bits of a
 * Redirection Table entry, as indicated by the associate bit mask.
 *
 * @param irq INTIN number
 * @param value Value to be written
 * @param mask  Mask of bits to be modified
 * @return N/A
 */
static void IoApicRedUpdateLo(unsigned int irq,
				u32_t value,
				u32_t mask)
{
	ioApicRedSetLo(irq, (ioApicRedGetLo(irq) & ~mask) | (value & mask));
}


#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
SYS_DEVICE_DEFINE("ioapic", _ioapic_init, ioapic_device_ctrl, PRE_KERNEL_1,
		  CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#else
SYS_INIT(_ioapic_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
