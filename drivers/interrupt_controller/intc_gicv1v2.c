/*
 * Copyright (c) 2018 Marvell
 * Copyright (c) 2018 Lexmark International, Inc.
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * NOTE: This driver implements the GICv1 and GICv2 interfaces.
 */

#include <assert.h>
#include <sw_isr_table.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>
#include <drivers/interrupt_controller/gic.h>
#include "intc_gicv1v2_priv.h"
#include "intc_gic_common_priv.h"

unsigned int arm_gic_get_active(void)
{
	int irq;

	irq = sys_read32(GICC_IAR) & 0x3ff;
	return irq;
}

void arm_gic_eoi(unsigned int irq)
{
	/*
	 * Ensure the write to peripheral registers are *complete* before the write
	 * to GIC_EOIR.
	 *
	 * Note: The completion gurantee depends on various factors of system design
	 * and the barrier is the best core can do by which execution of further
	 * instructions waits till the barrier is alive.
	 */
	__DSB();

	/* set to inactive */
	sys_write32(irq, GICC_EOIR);
}

void gic_raise_sgi(unsigned int sgi_id, uint64_t target_aff,
		   uint16_t target_list)
{
	ARG_UNUSED(target_aff);

	assert(GIC_IS_SGI(sgi_id));

	__DSB();
	/* Raise the interrupt */
	sys_write32((sgi_id | GICD_SGIR_CPULIST(target_list)), GICD_SGIR);
	__DSB();
}

static void gic_cpu_init(void)
{
	uint32_t val;
	unsigned int intid;
	mem_addr_t base = GIC_DIST_BASE;

	/*
	 * Deal with the banked PPI and SGI interrupts - disable all
	 * PPI interrupts, ensure all SGI interrupts are enabled.
	 */
#ifndef CONFIG_GIC_V1
	sys_write32(0xffffffff, GICD_ICACTIVERn);
#endif
	sys_write32(0xffff0000, ICENABLER(base, 0));
	sys_write32(0x0000ffff, ISENABLER(base, 0));

	/*
	 * Set priority on PPI and SGI interrupts
	 */
	for (intid = 0; intid < 32; intid += 4) {
		sys_write32(GIC_INT_DEF_PRI_X4, IPRIORITYR(base, intid));
	}

	sys_write32(0xf0, GICC_PMR);

	/*
	 * Enable interrupts and signal them using the IRQ signal.
	 */
	val = sys_read32(GICC_CTLR);
#ifndef CONFIG_GIC_V1
	val &= ~GICC_CTLR_BYPASS_MASK;
#endif
	val |= GICC_CTLR_ENABLE_MASK;
	sys_write32(val, GICC_CTLR);
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
