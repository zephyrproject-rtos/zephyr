/*
 * Copyright (c) 1997-1998, 2000-2002, 2004, 2006-2008, 2011-2015 Wind River
 * Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_ioapic

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

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <string.h>

#include <zephyr/drivers/interrupt_controller/ioapic.h> /* public API declarations */
#include <zephyr/drivers/interrupt_controller/loapic.h> /* public API declarations and registers */
#include "intc_ioapic_priv.h"

DEVICE_MMIO_TOPLEVEL_STATIC(ioapic_regs, DT_DRV_INST(0));

#define IOAPIC_REG DEVICE_MMIO_TOPLEVEL_GET(ioapic_regs)

/*
 * Destination field (bits[56:63]) defines a set of processors, which is
 * used to be compared with local LDR to determine which local APICs accept
 * the interrupt.
 *
 * XAPIC: in logical destination mode and flat model (determined by DFR).
 * LDR bits[24:31] can accommodate up to 8 logical APIC IDs.
 *
 * X2APIC: in logical destination mode and cluster model.
 * In this case, LDR is read-only to system software and supports up to 16
 * logical IDs. (Cluster ID: don't care to IO APIC).
 *
 * In either case, regardless how many CPUs in the system, 0xff implies that
 * it's intended to deliver to all possible 8 local APICs.
 */
#define DEFAULT_RTE_DEST (0xFFU << 24)

static __pinned_bss uint32_t ioapic_rtes;

#ifdef CONFIG_PM_DEVICE

#define BITS_PER_IRQ  4
#define IOAPIC_BITFIELD_HI_LO	0
#define IOAPIC_BITFIELD_LVL_EDGE 1
#define IOAPIC_BITFIELD_ENBL_DSBL 2
#define IOAPIC_BITFIELD_DELIV_MODE 3

#define BIT_POS_FOR_IRQ_OPTION(irq, option) ((irq) * BITS_PER_IRQ + (option))

/* Allocating up to 256 irq bits bufffer for RTEs, RTEs are dynamically found
 * so let's just assume the maximum, it's only 128 bytes in total.
 */
#define SUSPEND_BITS_REQD (ROUND_UP((256 * BITS_PER_IRQ), 32))

__pinned_bss
uint32_t ioapic_suspend_buf[SUSPEND_BITS_REQD / 32] = {0};
#endif

static uint32_t __IoApicGet(int32_t offset);
static void __IoApicSet(int32_t offset, uint32_t value);
static void ioApicRedSetHi(unsigned int irq, uint32_t upper32);
static void ioApicRedSetLo(unsigned int irq, uint32_t lower32);
static uint32_t ioApicRedGetLo(unsigned int irq);
static void IoApicRedUpdateLo(unsigned int irq, uint32_t value,
					uint32_t mask);

#if defined(CONFIG_INTEL_VTD_ICTL) &&				\
	!defined(CONFIG_INTEL_VTD_ICTL_XAPIC_PASSTHROUGH)

#include <zephyr/drivers/interrupt_controller/intel_vtd.h>
#include <zephyr/acpi/acpi.h>

static const struct device *const vtd =
	DEVICE_DT_GET_OR_NULL(DT_INST(0, intel_vt_d));
static uint16_t ioapic_id;

static bool get_vtd(void)
{
	if (!device_is_ready(vtd)) {
		return false;
	}

	if (ioapic_id != 0) {
		return true;
	}

	return acpi_dmar_ioapic_get(&ioapic_id) == 0;
}
#endif /* CONFIG_INTEL_VTD_ICTL && !INTEL_VTD_ICTL_XAPIC_PASSTHROUGH */

/*
 * The functions irq_enable() and irq_disable() are implemented in the
 * interrupt controller driver due to the IRQ virtualization imposed by
 * the x86 architecture.
 */

/**
 * @brief Initialize the IO APIC or xAPIC
 *
 * This routine initializes the IO APIC or xAPIC.
 *
 * @retval 0 on success.
 */
__boot_func
int ioapic_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	DEVICE_MMIO_TOPLEVEL_MAP(ioapic_regs, K_MEM_CACHE_NONE);

	/* Reading MRE: this will give the number of RTEs available */
	ioapic_rtes = ((__IoApicGet(IOAPIC_VERS) &
			IOAPIC_MRE_MASK) >> IOAPIC_MRE_POS) + 1;

#ifdef CONFIG_IOAPIC_MASK_RTE
	int32_t ix;	/* redirection table index */
	uint32_t rteValue; /* value to copy into redirection table entry */

	rteValue = IOAPIC_EDGE | IOAPIC_HIGH | IOAPIC_FIXED | IOAPIC_INT_MASK |
		   IOAPIC_LOGICAL | 0 /* dummy vector */;

	for (ix = 0; ix < ioapic_rtes; ix++) {
		ioApicRedSetHi(ix, DEFAULT_RTE_DEST);
		ioApicRedSetLo(ix, rteValue);
	}
#endif
	return 0;
}

__pinned_func
uint32_t z_ioapic_num_rtes(void)
{
	return ioapic_rtes;
}

/**
 * @brief Enable a specified APIC interrupt input line
 *
 * This routine enables a specified APIC interrupt input line.
 *
 * @param irq IRQ number to enable
 */
__pinned_func
void z_ioapic_irq_enable(unsigned int irq)
{
	IoApicRedUpdateLo(irq, 0, IOAPIC_INT_MASK);
}

/**
 * @brief Disable a specified APIC interrupt input line
 *
 * This routine disables a specified APIC interrupt input line.
 * @param irq IRQ number to disable
 */
__pinned_func
void z_ioapic_irq_disable(unsigned int irq)
{
	IoApicRedUpdateLo(irq, IOAPIC_INT_MASK, IOAPIC_INT_MASK);
}


#ifdef CONFIG_PM_DEVICE

__pinned_func
void store_flags(unsigned int irq, uint32_t flags)
{
	/* Currently only the following four flags are modified */
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

	/*
	 * We support lowest priority and fixed mode only, so only one bit
	 * needs to be saved.
	 */
	if (flags & IOAPIC_LOWEST) {
		sys_bitfield_set_bit((mem_addr_t) ioapic_suspend_buf,
			BIT_POS_FOR_IRQ_OPTION(irq, IOAPIC_BITFIELD_DELIV_MODE));
	}
}

__pinned_func
uint32_t restore_flags(unsigned int irq)
{
	uint32_t flags = 0U;

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

	if (sys_bitfield_test_bit((mem_addr_t) ioapic_suspend_buf,
		BIT_POS_FOR_IRQ_OPTION(irq, IOAPIC_BITFIELD_DELIV_MODE))) {
		flags |= IOAPIC_LOWEST;
	}

	return flags;
}


__pinned_func
int ioapic_suspend(const struct device *port)
{
	int irq;
	uint32_t rte_lo;

	ARG_UNUSED(port);
	(void)memset(ioapic_suspend_buf, 0, (SUSPEND_BITS_REQD >> 3));
	for (irq = 0; irq < ioapic_rtes; irq++) {
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
	return 0;
}

__pinned_func
int ioapic_resume_from_suspend(const struct device *port)
{
	int irq;
	uint32_t flags;
	uint32_t rteValue;

	ARG_UNUSED(port);

	for (irq = 0; irq < ioapic_rtes; irq++) {
		if (_irq_to_interrupt_vector[irq]) {
			/* Get the saved flags */
			flags = restore_flags(irq);
			/* Appending the flags that are never modified */
			flags = flags | IOAPIC_LOGICAL;

			rteValue = (_irq_to_interrupt_vector[irq] &
					IOAPIC_VEC_MASK) | flags;
		} else {
			/* Initialize the other RTEs to sane values */
			rteValue = IOAPIC_EDGE | IOAPIC_HIGH |
				IOAPIC_FIXED | IOAPIC_INT_MASK |
				IOAPIC_LOGICAL | 0 ; /* dummy vector*/
		}
		ioApicRedSetHi(irq, DEFAULT_RTE_DEST);
		ioApicRedSetLo(irq, rteValue);
	}
	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
__pinned_func
static int ioapic_pm_action(const struct device *dev,
			    enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = ioapic_resume_from_suspend(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = ioapic_suspend(dev);
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

#endif  /*CONFIG_PM_DEVICE*/

/**
 * @brief Programs the interrupt redirection table
 *
 * This routine sets up the redirection table entry for the specified IRQ
 * @param irq Virtualized IRQ
 * @param vector Vector number
 * @param flags Interrupt flags
 */
__boot_func
void z_ioapic_irq_set(unsigned int irq, unsigned int vector, uint32_t flags)
{
	uint32_t rteValue;   /* value to copy into redirection table entry */
#if defined(CONFIG_INTEL_VTD_ICTL) &&				\
	!defined(CONFIG_INTEL_VTD_ICTL_XAPIC_PASSTHROUGH)
	int irte_idx;

	if (!get_vtd()) {
		goto no_vtd;
	}

	irte_idx = vtd_get_irte_by_vector(vtd, vector);
	if (irte_idx < 0) {
		irte_idx = vtd_get_irte_by_irq(vtd, irq);
	}

	if (irte_idx >= 0 && !vtd_irte_is_msi(vtd, irte_idx)) {
		/* Enable interrupt remapping format and set the irte index */
		rteValue = IOAPIC_VTD_REMAP_FORMAT |
			IOAPIC_VTD_INDEX(irte_idx);
		ioApicRedSetHi(irq, rteValue);

		/* Remapped: delivery mode is Fixed (000) and
		 * destination mode is no longer present as it is replaced by
		 * the 15th bit of irte index, which is always 0 in our case.
		 */
		rteValue = IOAPIC_INT_MASK |
			(vector & IOAPIC_VEC_MASK) |
			(flags & IOAPIC_TRIGGER_MASK) |
			(flags & IOAPIC_POLARITY_MASK);
		ioApicRedSetLo(irq, rteValue);

		vtd_remap(vtd, irte_idx, vector, flags, ioapic_id);
	} else {
no_vtd:
#else
	{
#endif /* CONFIG_INTEL_VTD_ICTL && !CONFIG_INTEL_VTD_ICTL_XAPIC_PASSTHROUGH */
		/* the delivery mode is determined by the flags
		 * passed from drivers
		 */
		rteValue = IOAPIC_INT_MASK | IOAPIC_LOGICAL |
			(vector & IOAPIC_VEC_MASK) | flags;
		ioApicRedSetHi(irq, DEFAULT_RTE_DEST);
		ioApicRedSetLo(irq, rteValue);
	}
}

/**
 * @brief Program interrupt vector for specified irq
 *
 * The routine writes the interrupt vector in the Interrupt Redirection
 * Table for specified irq number
 *
 * @param irq Interrupt number
 * @param vector Vector number
 */
__boot_func
void z_ioapic_int_vec_set(unsigned int irq, unsigned int vector)
{
	IoApicRedUpdateLo(irq, vector, IOAPIC_VEC_MASK);
}

/**
 * @brief Read a 32 bit IO APIC register
 *
 * This routine reads the specified IO APIC register using indirect addressing.
 * @param offset Register offset (8 bits)
 *
 * @return register value
 */
__pinned_func
static uint32_t __IoApicGet(int32_t offset)
{
	uint32_t value; /* value */
	unsigned int key;	/* interrupt lock level */

	/* lock interrupts to ensure indirect addressing works "atomically" */

	key = irq_lock();

	*((volatile uint32_t *) (IOAPIC_REG + IOAPIC_IND)) = (unsigned char)offset;
	value = *((volatile uint32_t *)(IOAPIC_REG + IOAPIC_DATA));

	irq_unlock(key);

	return value;
}

/**
 * @brief Write a 32 bit IO APIC register
 *
 * This routine writes the specified IO APIC register using indirect addressing.
 *
 * @param offset Register offset (8 bits)
 * @param value Value to set the register
 */
__pinned_func
static void __IoApicSet(int32_t offset, uint32_t value)
{
	unsigned int key; /* interrupt lock level */

	/* lock interrupts to ensure indirect addressing works "atomically" */

	key = irq_lock();

	*(volatile uint32_t *)(IOAPIC_REG + IOAPIC_IND) = (unsigned char)offset;
	*((volatile uint32_t *)(IOAPIC_REG + IOAPIC_DATA)) = value;

	irq_unlock(key);
}

/**
 * @brief Get low 32 bits of Redirection Table entry
 *
 * This routine reads the low-order 32 bits of a Redirection Table entry.
 *
 * @param irq INTIN number
 * @return 32 low-order bits
 */
__pinned_func
static uint32_t ioApicRedGetLo(unsigned int irq)
{
	int32_t offset = IOAPIC_REDTBL + (irq << 1); /* register offset */

	return __IoApicGet(offset);
}

/**
 * @brief Set low 32 bits of Redirection Table entry
 *
 * This routine writes the low-order 32 bits of a Redirection Table entry.
 *
 * @param irq INTIN number
 * @param lower32 Value to be written
 */
__pinned_func
static void ioApicRedSetLo(unsigned int irq, uint32_t lower32)
{
	int32_t offset = IOAPIC_REDTBL + (irq << 1); /* register offset */

	__IoApicSet(offset, lower32);
}

/**
 * @brief Set high 32 bits of Redirection Table entry
 *
 * This routine writes the high-order 32 bits of a Redirection Table entry.
 *
 * @param irq INTIN number
 * @param upper32 Value to be written
 */
__pinned_func
static void ioApicRedSetHi(unsigned int irq, uint32_t upper32)
{
	int32_t offset = IOAPIC_REDTBL + (irq << 1) + 1; /* register offset */

	__IoApicSet(offset, upper32);
}

/**
 * @brief Modify low 32 bits of Redirection Table entry
 *
 * This routine modifies selected portions of the low-order 32 bits of a
 * Redirection Table entry, as indicated by the associate bit mask.
 *
 * @param irq INTIN number
 * @param value Value to be written
 * @param mask  Mask of bits to be modified
 */
__pinned_func
static void IoApicRedUpdateLo(unsigned int irq,
				uint32_t value,
				uint32_t mask)
{
	ioApicRedSetLo(irq, (ioApicRedGetLo(irq) & ~mask) | (value & mask));
}

PM_DEVICE_DT_INST_DEFINE(0, ioapic_pm_action);

DEVICE_DT_INST_DEFINE(0, ioapic_init, PM_DEVICE_DT_INST_GET(0), NULL, NULL,
		      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);
