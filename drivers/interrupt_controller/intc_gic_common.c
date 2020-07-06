/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <sw_isr_table.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>
#include <drivers/interrupt_controller/gic.h>

#if CONFIG_GIC_VER <= 2
#include "intc_gicv1v2_priv.h"
#else
#include "intc_gicv3_priv.h"
#endif
#include "intc_gic_common_priv.h"

void arm_gic_irq_set_priority(unsigned int intid,
			      unsigned int prio, uint32_t flags)
{
	uint32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	uint32_t idx = intid / GIC_NUM_INTR_PER_REG;
	uint32_t shift;
	uint32_t val;
	mem_addr_t base = GET_DIST_BASE(intid);

	/* Disable the interrupt */
	sys_write32(mask, ICENABLER(base, idx));
#if defined(CONFIG_GIC_V3)
	gic_wait_rwp(intid);
#endif
	/* PRIORITYR registers provide byte access */
	sys_write8(prio & GIC_PRI_MASK, IPRIORITYR(base, intid));

	/* Interrupt type config */
	if (!GIC_IS_SGI(intid)) {
		idx = intid / GIC_NUM_CFG_PER_REG;
		shift = (intid & (GIC_NUM_CFG_PER_REG - 1)) * 2;

		val = sys_read32(ICFGR(base, idx));
		val &= ~(GICD_ICFGR_MASK << shift);
		if (flags & IRQ_TYPE_EDGE) {
			val |= (GICD_ICFGR_TYPE << shift);
		}
		sys_write32(val, ICFGR(base, idx));
	}
}

void arm_gic_irq_enable(unsigned int intid)
{
	uint32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	uint32_t idx = intid / GIC_NUM_INTR_PER_REG;

	sys_write32(mask, ISENABLER(GET_DIST_BASE(intid), idx));
}

void arm_gic_irq_disable(unsigned int intid)
{
	uint32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	uint32_t idx = intid / GIC_NUM_INTR_PER_REG;

	sys_write32(mask, ICENABLER(GET_DIST_BASE(intid), idx));

#if defined(CONFIG_GIC_V3)
	/* poll to ensure write is complete */
	gic_wait_rwp(intid);
#endif
}

bool arm_gic_irq_is_enabled(unsigned int intid)
{
	uint32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	uint32_t idx = intid / GIC_NUM_INTR_PER_REG;
	uint32_t val;

	val = sys_read32(ISENABLER(GET_DIST_BASE(intid), idx));

	return (val & mask) != 0;
}

/*
 * TODO: Consider Zephyr in EL1NS.
 */
void gic_dist_init(void)
{
	unsigned int num_ints;
	unsigned int intid;
	unsigned int idx;
	mem_addr_t base = GIC_DIST_BASE;

	num_ints = sys_read32(GICD_TYPER);
	num_ints &= GICD_TYPER_ITLINESNUM_MASK;
	num_ints = (num_ints + 1) << 5;

	/*
	 * Default configuration of all SPIs
	 */
	for (intid = GIC_SPI_INT_BASE; intid < num_ints;
	     intid += GIC_NUM_INTR_PER_REG) {
		idx = intid / GIC_NUM_INTR_PER_REG;
		/* Disable interrupt */
		sys_write32(BIT_MASK(GIC_NUM_INTR_PER_REG),
			    ICENABLER(base, idx));
		/* Clear pending */
		sys_write32(BIT_MASK(GIC_NUM_INTR_PER_REG),
			    ICPENDR(base, idx));
		/* Set all SPIs to group-0 secure */
		sys_write32(0, IGROUPR(base, idx));

		#if defined(CONFIG_GIC_V3)
		/* SPI to be routed as native EL1S irqs ie:G1S */
		sys_write32(BIT_MASK(GIC_NUM_INTR_PER_REG),
			    IGROUPMODR(base, idx));
		#endif
	}

	#if CONFIG_GIC_VER >= 3
	/* wait for rwp on GICD */
	gic_wait_rwp(GIC_SPI_INT_BASE);
	#endif

	/* Configure default priorities for all SPIs. */
	for (intid = GIC_SPI_INT_BASE; intid < num_ints;
	     intid += GIC_NUM_PRI_PER_REG) {
		sys_write32(GIC_INT_DEF_PRI_X4, IPRIORITYR(base, intid));
	}

	/* Configure all SPIs as active low, level triggered by default */
	for (intid = GIC_SPI_INT_BASE; intid < num_ints;
	     intid += GIC_NUM_CFG_PER_REG) {
		idx = intid / GIC_NUM_CFG_PER_REG;
		sys_write32(0, ICFGR(base, idx));
	}

	#if defined(CONFIG_GIC_V2)
	/* Set all SPI to CPU0 only */
	for (intid = GIC_SPI_INT_BASE; intid < num_ints;
	     intid += GIC_NUM_TGT_PER_REG) {
		idx = intid / GIC_NUM_TGT_PER_REG;
		sys_write32(0x01010101, ITARGETSR(base, idx));
	}
	/* enable Group 0 secure interrupts */
	sys_set_bit(GICD_CTLR, GICD_CTLR_ENABLE_G0);
	#elif defined(CONFIG_GIC_V3)
	/* enable Group 1 secure interrupts */
	sys_set_bit(GICD_CTLR, GICD_CTLR_ENABLE_G1S);
	#endif
}
