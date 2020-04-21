/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <sw_isr_table.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>
#include <drivers/interrupt_controller/gic.h>
#include "intc_gic_common_priv.h"
#include "intc_gicv3_priv.h"

/* Redistributor base addresses for each core */
mem_addr_t gic_rdists[GIC_NUM_CPU_IF];

/*
 * Wait for register write pending
 * TODO: add timed wait
 */
static int gic_wait_rwp(u32_t intid)
{
	u32_t rwp_mask;
	mem_addr_t base;

	if (intid < GIC_SPI_INT_BASE) {
		base = (GIC_GET_RDIST(GET_CPUID) + GICR_CTLR);
		rwp_mask = BIT(GICR_CTLR_RWP);
	} else {
		base = GICD_CTLR;
		rwp_mask = BIT(GICD_CTLR_RWP);
	}

	while (sys_read32(base) & rwp_mask)
		;

	return 0;
}

void arm_gic_irq_set_priority(unsigned int intid,
			      unsigned int prio, u32_t flags)
{
	u32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	u32_t idx = intid / GIC_NUM_INTR_PER_REG;
	u32_t shift;
	u32_t val;
	mem_addr_t base = GET_DIST_BASE(intid);

	/* Disable the interrupt */
	sys_write32(mask, ICENABLER(base, idx));
	gic_wait_rwp(intid);

	/* PRIORITYR registers provide byte access */
	sys_write8(prio & GIC_PRI_MASK, IPRIORITYR(base, intid));

	/* Interrupt type config */
	idx = intid / GIC_NUM_CFG_PER_REG;
	shift = (intid & (GIC_NUM_CFG_PER_REG - 1)) * 2;

	val = sys_read32(ICFGR(base, idx));
	val &= ~(GICD_ICFGR_MASK << shift);
	if (flags & IRQ_TYPE_EDGE) {
		val |= (GICD_ICFGR_TYPE << shift);
	}
	sys_write32(val, ICFGR(base, idx));
}

void arm_gic_irq_enable(unsigned int intid)
{
	u32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	u32_t idx = intid / GIC_NUM_INTR_PER_REG;

	sys_write32(mask, ISENABLER(GET_DIST_BASE(intid), idx));
}

void arm_gic_irq_disable(unsigned int intid)
{
	u32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	u32_t idx = intid / GIC_NUM_INTR_PER_REG;

	sys_write32(mask, ICENABLER(GET_DIST_BASE(intid), idx));
	/* poll to ensure write is complete */
	gic_wait_rwp(intid);
}

bool arm_gic_irq_is_enabled(unsigned int intid)
{
	u32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	u32_t idx = intid / GIC_NUM_INTR_PER_REG;
	u32_t val;

	val = sys_read32(ISENABLER(GET_DIST_BASE(intid), idx));

	return (val & mask) != 0;
}

unsigned int arm_gic_get_active(void)
{
	int intid;

	/* (Pending -> Active / AP) or (AP -> AP) */
	intid = read_sysreg(ICC_IAR1_EL1);

	return intid;
}

void arm_gic_eoi(unsigned int intid)
{
	/* (AP -> Pending) Or (Active -> Inactive) or (AP to AP) nested case */
	write_sysreg(intid, ICC_EOIR1_EL1);
}

/*
 * Wake up GIC redistributor.
 * clear ProcessorSleep and wait till ChildAsleep is cleared.
 * ProcessSleep to be cleared only when ChildAsleep is set
 * Check if redistributor is not powered already.
 */
static void gicv3_rdist_enable(mem_addr_t rdist)
{
	if (!(sys_read32(rdist + GICR_WAKER) & BIT(GICR_WAKER_CA)))
		return;

	sys_clear_bit(rdist + GICR_WAKER, GICR_WAKER_PS);
	while (sys_read32(rdist + GICR_WAKER) & BIT(GICR_WAKER_CA))
		;
}

/*
 * Initialize the cpu interface. This should be called by each core.
 */
static void gicv3_cpuif_init(void)
{
	u32_t icc_sre;
	u32_t intid;

	mem_addr_t base = gic_rdists[GET_CPUID] + GICR_SGI_BASE_OFF;

	/* Disable all sgi ppi */
	sys_write32(BIT_MASK(GIC_NUM_INTR_PER_REG), ICENABLER(base, 0));
	/* Any sgi/ppi intid ie. 0-31 will select GICR_CTRL */
	gic_wait_rwp(0);

	/* Clear pending */
	sys_write32(BIT_MASK(GIC_NUM_INTR_PER_REG), ICPENDR(base, 0));

	/* Configure all SGIs/PPIs as G1S.
	 * TODO: G1S or G1NS dependending on Zephyr is run in EL1S
	 * or EL1NS respectively.
	 * All interrupts will be delivered as irq
	 */
	sys_write32(0, IGROUPR(base, 0));
	sys_write32(BIT_MASK(GIC_NUM_INTR_PER_REG), IGROUPMODR(base, 0));

	/*
	 * Configure default priorities for SGI 0:15 and PPI 0:15.
	 */
	for (intid = 0; intid < GIC_SPI_INT_BASE;
	     intid += GIC_NUM_PRI_PER_REG) {
		sys_write32(GIC_INT_DEF_PRI_X4, IPRIORITYR(base, intid));
	}

	/* Configure PPIs as level triggered */
	sys_write32(0, ICFGR(base, 1));

	/*
	 * Check if system interface can be enabled.
	 * 'icc_sre_el3' needs to be configured at 'EL3'
	 * to allow access to 'icc_sre_el1' at 'EL1'
	 * eg: z_arch_el3_plat_init can be used by platform.
	 */
	icc_sre = read_sysreg(ICC_SRE_EL1);

	if (!(icc_sre & ICC_SRE_ELx_SRE)) {
		icc_sre = (icc_sre | ICC_SRE_ELx_SRE |
			   ICC_SRE_ELx_DIB | ICC_SRE_ELx_DFB);
		write_sysreg(icc_sre, ICC_SRE_EL1);
		icc_sre = read_sysreg(ICC_SRE_EL1);

		assert(icc_sre & ICC_SRE_ELx_SRE);
	}

	write_sysreg(GIC_IDLE_PRIO, ICC_PMR_EL1);

	/* Allow group1 interrupts */
	write_sysreg(1, ICC_IGRPEN1_EL1);
}

/*
 * TODO: Consider Zephyr in EL1NS.
 */
static void gicv3_dist_init(void)
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
		/* All SPIs are G1S and owned by Zephyr */
		sys_write32(0, IGROUPR(base, idx));
		sys_write32(BIT_MASK(GIC_NUM_INTR_PER_REG),
			    IGROUPMODR(base, idx));

	}
	/* wait for rwp on GICD */
	gic_wait_rwp(GIC_SPI_INT_BASE);

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

	/* enable Group 1 secure interrupts */
	sys_set_bit(GICD_CTLR, GICD_CTLR_ENABLE_G1S);
}

/* TODO: add arm_gic_secondary_init() for multicore support */
int arm_gic_init(void)
{
	gicv3_dist_init();

	/* Fixme: populate each redistributor */
	gic_rdists[0] = GIC_RDIST_BASE;

	gicv3_rdist_enable(GIC_GET_RDIST(GET_CPUID));

	gicv3_cpuif_init();

	return 0;
}
