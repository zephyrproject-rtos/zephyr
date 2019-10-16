/*
 * Copyright (c) 2018 Marvell
 * Copyright (c) 2018 Lexmark International, Inc.
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <sw_isr_table.h>
#include <interrupt_controller/gic.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>

#define DT_GIC_DIST_BASE	DT_INST_0_ARM_GIC_BASE_ADDRESS_0
#define DT_GIC_CPU_BASE		DT_INST_0_ARM_GIC_BASE_ADDRESS_1

#define	GICD_CTRL				(DT_GIC_DIST_BASE +     0)
#define	GICD_TYPER				(DT_GIC_DIST_BASE +   0x4)
#define	GICD_IIDR				(DT_GIC_DIST_BASE +   0x8)
#define	GICD_IGROUPRn			(DT_GIC_DIST_BASE +  0x80)
#define	GICD_ISENABLERn			(DT_GIC_DIST_BASE + 0x100)
#define	GICD_ICENABLERn			(DT_GIC_DIST_BASE + 0x180)
#define	GICD_ISPENDRn			(DT_GIC_DIST_BASE + 0x200)
#define	GICD_ICPENDRn			(DT_GIC_DIST_BASE + 0x280)
#define	GICD_ISACTIVERn			(DT_GIC_DIST_BASE + 0x300)
#define	GICD_ICACTIVERn			(DT_GIC_DIST_BASE + 0x380)
#define	GICD_IPRIORITYRn		(DT_GIC_DIST_BASE + 0x400)
#define	GICD_ITARGETSRn			(DT_GIC_DIST_BASE + 0x800)
#define	GICD_ICFGRn				(DT_GIC_DIST_BASE + 0xc00)
#define	GICD_SGIR				(DT_GIC_DIST_BASE + 0xf00)

#define GICC_CTRL				(DT_GIC_CPU_BASE + 0x00)
#define GICC_PMR				(DT_GIC_CPU_BASE + 0x04)
#define GICC_BPR				(DT_GIC_CPU_BASE + 0x08)
#define GICC_IAR				(DT_GIC_CPU_BASE + 0x0c)
#define GICC_EOIR				(DT_GIC_CPU_BASE + 0x10)

#define GICC_ENABLE	3
#define GICC_DIS_BYPASS_MASK	0x1e0

#define	NO_GIC_INT_PENDING	1023

#define	GIC_SPI_INT_BASE	32

#define GIC_INT_TYPE_MASK	0x3
#define GIC_INT_TYPE_EDGE	(1 << 1)

void arm_gic_irq_enable(unsigned int irq)
{
	int int_grp, int_off;

	irq += GIC_SPI_INT_BASE;
	int_grp = irq / 32;
	int_off = irq % 32;

	sys_write32((1 << int_off), (GICD_ISENABLERn + int_grp * 4));
}

void arm_gic_irq_disable(unsigned int irq)
{
	int int_grp, int_off;

	irq += GIC_SPI_INT_BASE;
	int_grp = irq / 32;
	int_off = irq % 32;

	sys_write32((1 << int_off), (GICD_ICENABLERn + int_grp * 4));
}

int arm_gic_irq_is_enabled(unsigned int irq)
{
	int int_grp, int_off;
	unsigned int enabler;

	irq += GIC_SPI_INT_BASE;
	int_grp = irq / 32;
	int_off = irq % 32;

	enabler = sys_read32(GICD_ICENABLERn + int_grp * 4);

	return (enabler & (1 << int_off)) != 0;
}

void arm_gic_irq_set_priority(
	unsigned int irq, unsigned int prio, unsigned int flags)
{
	int int_grp, int_off;
	u8_t val;

	irq += GIC_SPI_INT_BASE;

	/* Set priority */
	sys_write8(prio & 0xff, GICD_IPRIORITYRn + irq);

	/* Set interrupt type */
	int_grp = irq / 4;
	int_off = (irq % 4) * 2;

	val = sys_read8(GICD_ICFGRn + int_grp);
	val &= ~(GIC_INT_TYPE_MASK << int_off);
	if (flags & IRQ_TYPE_EDGE)
		val |= (GIC_INT_TYPE_EDGE << int_off);
	sys_write8(val, GICD_ICFGRn + int_grp);
}

unsigned int arm_gic_irq_get_active(void)
{
	int irq;

	irq = sys_read32(GICC_IAR) & 0x3ff;
	irq -= GIC_SPI_INT_BASE;

	return irq;
}

void arm_gic_irq_eoi(unsigned int irq)
{
	irq += GIC_SPI_INT_BASE;

	/* set to inactive */
	sys_write32(irq, GICC_EOIR);
}

static void gic_dist_init(void)
{
	unsigned int gic_irqs, i;

	gic_irqs = sys_read32(GICD_TYPER) & 0x1f;
	gic_irqs = (gic_irqs + 1) * 32;
	if (gic_irqs > 1020)
		gic_irqs = 1020;

	/*
	 * Disable the forwarding of pending interrupts
	 * from the Distributor to the CPU interfaces
	 */
	sys_write32(0, GICD_CTRL);

	/*
	 * Set all global interrupts to this CPU only.
	 */
	for (i = GIC_SPI_INT_BASE; i < gic_irqs; i += 4)
		sys_write32(0x01010101, GICD_ITARGETSRn + i);

	/*
	 * Set all global interrupts to be level triggered, active low.
	 */
	for (i = GIC_SPI_INT_BASE; i < gic_irqs; i += 16)
		sys_write32(0, GICD_ICFGRn + i / 4);

	/*  Set priority on all global interrupts.   */
	for (i = GIC_SPI_INT_BASE; i < gic_irqs; i += 4)
		sys_write32(0, GICD_IPRIORITYRn + i);

	/* Set all interrupts to group 0 */
	for (i = GIC_SPI_INT_BASE; i < gic_irqs; i += 32)
		sys_write32(0, GICD_IGROUPRn + i / 8);

	/*
	 * Disable all interrupts.  Leave the PPI and SGIs alone
	 * as these enables are banked registers.
	 */
	for (i = GIC_SPI_INT_BASE; i < gic_irqs; i += 32) {
		sys_write32(0xffffffff, GICD_ICACTIVERn + i / 8);
		sys_write32(0xffffffff, GICD_ICENABLERn + i / 8);
	}

	/*
	 * Enable the forwarding of pending interrupts
	 * from the Distributor to the CPU interfaces
	 */
	sys_write32(1, GICD_CTRL);
}

static void gic_cpu_init(void)
{
	int i;
	u32_t val;

	/*
	 * Deal with the banked PPI and SGI interrupts - disable all
	 * PPI interrupts, ensure all SGI interrupts are enabled.
	 */
	sys_write32(0xffffffff, GICD_ICACTIVERn);
	sys_write32(0xffff0000, GICD_ICENABLERn);
	sys_write32(0x0000ffff, GICD_ISENABLERn);

	/*
	 * Set priority on PPI and SGI interrupts
	 */
	for (i = 0; i < 32; i += 4)
		sys_write32(0xa0a0a0a0, GICD_IPRIORITYRn + i);

	sys_write32(0xf0, GICC_PMR);

	/*
	 * Enable interrupts and signal them using the IRQ signal.
	 */
	val = sys_read32(GICC_CTRL);
	val &= GICC_DIS_BYPASS_MASK;
	val |= GICC_ENABLE;
	sys_write32(val, GICC_CTRL);
}

/**
 *
 * @brief Initialize the GIC device driver
 *
 *
 * @return N/A
 */
int arm_gic_init(void)
{
	/* Init of Distributor interface registers */
	gic_dist_init();

	/* Init CPU interface registers */
	gic_cpu_init();

	return 0;
}
