/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/dt-bindings/interrupt-controller/arm-gic.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include "intc_gic_common_priv.h"
#include "intc_gicv3_priv.h"

#include <string.h>

/* Redistributor base addresses for each core */
mem_addr_t gic_rdists[CONFIG_MP_MAX_NUM_CPUS];

#if defined(CONFIG_ARMV8_A_NS) || defined(CONFIG_GIC_SINGLE_SECURITY_STATE)
#define IGROUPR_VAL	0xFFFFFFFFU
#else
#define IGROUPR_VAL	0x0U
#endif

/*
 * We allocate memory for PROPBASE to cover 2 ^ lpi_id_bits LPIs to
 * deal with (one configuration byte per interrupt). PENDBASE has to
 * be 64kB aligned (one bit per LPI, plus 8192 bits for SPI/PPI/SGI).
 */
#define ITS_MAX_LPI_NRBITS	16 /* 64K LPIs */

#define LPI_PROPBASE_SZ(nrbits)	ROUND_UP(BIT(nrbits), KB(64))
#define LPI_PENDBASE_SZ(nrbits)	ROUND_UP(BIT(nrbits) / 8, KB(64))

#ifdef CONFIG_GIC_V3_ITS
static uintptr_t lpi_prop_table;

atomic_t nlpi_intid = ATOMIC_INIT(8192);
#endif

static inline mem_addr_t gic_get_rdist(void)
{
	return gic_rdists[arch_curr_cpu()->id];
}

/*
 * Wait for register write pending
 * TODO: add timed wait
 */
static int gic_wait_rwp(uint32_t intid)
{
	uint32_t rwp_mask;
	mem_addr_t base;

	if (intid < GIC_SPI_INT_BASE) {
		base = (gic_get_rdist() + GICR_CTLR);
		rwp_mask = BIT(GICR_CTLR_RWP);
	} else {
		base = GICD_CTLR;
		rwp_mask = BIT(GICD_CTLR_RWP);
	}

	while (sys_read32(base) & rwp_mask) {
		;
	}

	return 0;
}

#ifdef CONFIG_GIC_V3_ITS
static void arm_gic_lpi_setup(unsigned int intid, bool enable)
{
	uint8_t *cfg = &((uint8_t *)lpi_prop_table)[intid - 8192];

	if (enable) {
		*cfg |= BIT(0);
	} else {
		*cfg &= ~BIT(0);
	}

	dsb();

	its_rdist_invall();
}

static void arm_gic_lpi_set_priority(unsigned int intid, unsigned int prio)
{
	uint8_t *cfg = &((uint8_t *)lpi_prop_table)[intid - 8192];

	*cfg &= 0xfc;
	*cfg |= prio & 0xfc;

	dsb();

	its_rdist_invall();
}

static bool arm_gic_lpi_is_enabled(unsigned int intid)
{
	uint8_t *cfg = &((uint8_t *)lpi_prop_table)[intid - 8192];

	return (*cfg & BIT(0));
}
#endif

#if defined(CONFIG_ARMV8_A_NS) || defined(CONFIG_GIC_SINGLE_SECURITY_STATE)
static inline void arm_gic_write_irouter(uint64_t val, unsigned int intid)
{
	mem_addr_t addr = IROUTER(GET_DIST_BASE(intid), intid);

#ifdef CONFIG_ARM
	sys_write32((uint32_t)val, addr);
	sys_write32((uint32_t)(val >> 32U), addr + 4);
#else
	sys_write64(val, addr);
#endif
}
#endif

void arm_gic_irq_set_priority(unsigned int intid,
			      unsigned int prio, uint32_t flags)
{
#ifdef CONFIG_GIC_V3_ITS
	if (intid >= 8192) {
		arm_gic_lpi_set_priority(intid, prio);
		return;
	}
#endif
	uint32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	uint32_t idx = intid / GIC_NUM_INTR_PER_REG;
	uint32_t shift;
	uint32_t val;
	mem_addr_t base = GET_DIST_BASE(intid);

	/* Disable the interrupt */
	sys_write32(mask, ICENABLER(base, idx));
	gic_wait_rwp(intid);

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
#ifdef CONFIG_GIC_V3_ITS
	if (intid >= 8192) {
		arm_gic_lpi_setup(intid, true);
		return;
	}
#endif
	uint32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	uint32_t idx = intid / GIC_NUM_INTR_PER_REG;

	sys_write32(mask, ISENABLER(GET_DIST_BASE(intid), idx));

#if defined(CONFIG_ARMV8_A_NS) || defined(CONFIG_GIC_SINGLE_SECURITY_STATE)
	/*
	 * Affinity routing is enabled for Armv8-A Non-secure state (GICD_CTLR.ARE_NS
	 * is set to '1') and for GIC single security state (GICD_CTRL.ARE is set to '1'),
	 * so need to set SPI's affinity, now set it to be the PE on which it is enabled.
	 */
	if (GIC_IS_SPI(intid)) {
		arm_gic_write_irouter(MPIDR_TO_CORE(GET_MPIDR()), intid);
	}
#endif
}

void arm_gic_irq_disable(unsigned int intid)
{
#ifdef CONFIG_GIC_V3_ITS
	if (intid >= 8192) {
		arm_gic_lpi_setup(intid, false);
		return;
	}
#endif
	uint32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	uint32_t idx = intid / GIC_NUM_INTR_PER_REG;

	sys_write32(mask, ICENABLER(GET_DIST_BASE(intid), idx));
	/* poll to ensure write is complete */
	gic_wait_rwp(intid);
}

bool arm_gic_irq_is_enabled(unsigned int intid)
{
#ifdef CONFIG_GIC_V3_ITS
	if (intid >= 8192) {
		return arm_gic_lpi_is_enabled(intid);
	}
#endif
	uint32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	uint32_t idx = intid / GIC_NUM_INTR_PER_REG;
	uint32_t val;

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
	/*
	 * Interrupt request deassertion from peripheral to GIC happens
	 * by clearing interrupt condition by a write to the peripheral
	 * register. It is desired that the write transfer is complete
	 * before the core tries to change GIC state from 'AP/Active' to
	 * a new state on seeing 'EOI write'.
	 * Since ICC interface writes are not ordered against Device
	 * memory writes, a barrier is required to ensure the ordering.
	 * The dsb will also ensure *completion* of previous writes with
	 * DEVICE nGnRnE attribute.
	 */
	__DSB();

	/* (AP -> Pending) Or (Active -> Inactive) or (AP to AP) nested case */
	write_sysreg(intid, ICC_EOIR1_EL1);
}

void gic_raise_sgi(unsigned int sgi_id, uint64_t target_aff,
		   uint16_t target_list)
{
	uint32_t aff3, aff2, aff1;
	uint64_t sgi_val;

	__ASSERT_NO_MSG(GIC_IS_SGI(sgi_id));

	/* Extract affinity fields from target */
	aff1 = MPIDR_AFFLVL(target_aff, 1);
	aff2 = MPIDR_AFFLVL(target_aff, 2);
#if defined(CONFIG_ARM)
	/* There is no Aff3 in AArch32 MPIDR */
	aff3 = 0;
#else
	aff3 = MPIDR_AFFLVL(target_aff, 3);
#endif
	sgi_val = GICV3_SGIR_VALUE(aff3, aff2, aff1, sgi_id,
				   SGIR_IRM_TO_AFF, target_list);

	__DSB();
	write_sysreg(sgi_val, ICC_SGI1R);
	__ISB();
}

/*
 * Wake up GIC redistributor.
 * clear ProcessorSleep and wait till ChildAsleep is cleared.
 * ProcessSleep to be cleared only when ChildAsleep is set
 * Check if redistributor is not powered already.
 */
static void gicv3_rdist_enable(mem_addr_t rdist)
{
	if (!(sys_read32(rdist + GICR_WAKER) & BIT(GICR_WAKER_CA))) {
		return;
	}

	sys_clear_bit(rdist + GICR_WAKER, GICR_WAKER_PS);
	while (sys_read32(rdist + GICR_WAKER) & BIT(GICR_WAKER_CA)) {
		;
	}
}

#ifdef CONFIG_GIC_V3_ITS
/*
 * Setup LPIs Configuration & Pending tables for redistributors
 * LPI configuration is global, each redistributor has a pending table
 */
static void gicv3_rdist_setup_lpis(mem_addr_t rdist)
{
	unsigned int lpi_id_bits = MIN(GICD_TYPER_IDBITS(sys_read32(GICD_TYPER)),
				       ITS_MAX_LPI_NRBITS);
	uintptr_t lpi_pend_table;
	uint64_t reg;
	uint32_t ctlr;

	/* If not, alloc a common prop table for all redistributors */
	if (!lpi_prop_table) {
		lpi_prop_table = (uintptr_t)k_aligned_alloc(4 * 1024, LPI_PROPBASE_SZ(lpi_id_bits));
		memset((void *)lpi_prop_table, 0, LPI_PROPBASE_SZ(lpi_id_bits));
	}

	lpi_pend_table = (uintptr_t)k_aligned_alloc(64 * 1024, LPI_PENDBASE_SZ(lpi_id_bits));
	memset((void *)lpi_pend_table, 0, LPI_PENDBASE_SZ(lpi_id_bits));

	ctlr = sys_read32(rdist + GICR_CTLR);
	ctlr &= ~GICR_CTLR_ENABLE_LPIS;
	sys_write32(ctlr, rdist + GICR_CTLR);

	/* PROPBASE */
	reg = (GIC_BASER_SHARE_INNER << GITR_PROPBASER_SHAREABILITY_SHIFT) |
	      (GIC_BASER_CACHE_RAWAWB << GITR_PROPBASER_INNER_CACHE_SHIFT) |
	      (lpi_prop_table & (GITR_PROPBASER_ADDR_MASK << GITR_PROPBASER_ADDR_SHIFT)) |
	      (GIC_BASER_CACHE_INNERLIKE << GITR_PROPBASER_OUTER_CACHE_SHIFT) |
	      ((lpi_id_bits - 1) & GITR_PROPBASER_ID_BITS_MASK);
	sys_write64(reg, rdist + GICR_PROPBASER);
	/* TOFIX: check SHAREABILITY validity */

	/* PENDBASE */
	reg = (GIC_BASER_SHARE_INNER << GITR_PENDBASER_SHAREABILITY_SHIFT) |
	      (GIC_BASER_CACHE_RAWAWB << GITR_PENDBASER_INNER_CACHE_SHIFT) |
	      (lpi_pend_table & (GITR_PENDBASER_ADDR_MASK << GITR_PENDBASER_ADDR_SHIFT)) |
	      (GIC_BASER_CACHE_INNERLIKE << GITR_PENDBASER_OUTER_CACHE_SHIFT) |
	      GITR_PENDBASER_PTZ;
	sys_write64(reg, rdist + GICR_PENDBASER);
	/* TOFIX: check SHAREABILITY validity */

	ctlr = sys_read32(rdist + GICR_CTLR);
	ctlr |= GICR_CTLR_ENABLE_LPIS;
	sys_write32(ctlr, rdist + GICR_CTLR);

	dsb();
}
#endif

/*
 * Initialize the cpu interface. This should be called by each core.
 */
static void gicv3_cpuif_init(void)
{
	uint32_t icc_sre;
	uint32_t intid;

	mem_addr_t base = gic_get_rdist() + GICR_SGI_BASE_OFF;

	/* Disable all sgi ppi */
	sys_write32(BIT64_MASK(GIC_NUM_INTR_PER_REG), ICENABLER(base, 0));
	/* Any sgi/ppi intid ie. 0-31 will select GICR_CTRL */
	gic_wait_rwp(0);

	/* Clear pending */
	sys_write32(BIT64_MASK(GIC_NUM_INTR_PER_REG), ICPENDR(base, 0));

	/* Configure all SGIs/PPIs as G1S or G1NS depending on Zephyr
	 * is run in EL1S or EL1NS respectively.
	 * All interrupts will be delivered as irq
	 */
	sys_write32(IGROUPR_VAL, IGROUPR(base, 0));
	sys_write32(BIT64_MASK(GIC_NUM_INTR_PER_REG), IGROUPMODR(base, 0));

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

	if (!(icc_sre & ICC_SRE_ELx_SRE_BIT)) {
		icc_sre = (icc_sre | ICC_SRE_ELx_SRE_BIT |
			   ICC_SRE_ELx_DIB_BIT | ICC_SRE_ELx_DFB_BIT);
		write_sysreg(icc_sre, ICC_SRE_EL1);
		icc_sre = read_sysreg(ICC_SRE_EL1);

		__ASSERT_NO_MSG(icc_sre & ICC_SRE_ELx_SRE_BIT);
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

	/* Disable the distributor */
	sys_write32(0, GICD_CTLR);
	gic_wait_rwp(GIC_SPI_INT_BASE);
#ifdef CONFIG_GIC_SINGLE_SECURITY_STATE
	/*
	 * Before configuration, we need to check whether
	 * the GIC single security state mode is supported.
	 * Make sure GICD_CTRL_NS is 1.
	 */
	sys_set_bit(GICD_CTLR, GICD_CTRL_NS);
	__ASSERT(sys_test_bit(GICD_CTLR, GICD_CTRL_NS),
		"Current GIC does not support single security state");
#endif

	/*
	 * Default configuration of all SPIs
	 */
	for (intid = GIC_SPI_INT_BASE; intid < num_ints;
	     intid += GIC_NUM_INTR_PER_REG) {
		idx = intid / GIC_NUM_INTR_PER_REG;
		/* Disable interrupt */
		sys_write32(BIT64_MASK(GIC_NUM_INTR_PER_REG),
			    ICENABLER(base, idx));
		/* Clear pending */
		sys_write32(BIT64_MASK(GIC_NUM_INTR_PER_REG),
			    ICPENDR(base, idx));
		sys_write32(IGROUPR_VAL, IGROUPR(base, idx));
		sys_write32(BIT64_MASK(GIC_NUM_INTR_PER_REG),
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

#ifdef CONFIG_ARMV8_A_NS
	/* Enable distributor with ARE */
	sys_write32(BIT(GICD_CTRL_ARE_NS) | BIT(GICD_CTLR_ENABLE_G1NS),
		    GICD_CTLR);
#elif defined(CONFIG_GIC_SINGLE_SECURITY_STATE)
	/*
	 * For GIC single security state, the config GIC_SINGLE_SECURITY_STATE
	 * means the GIC is under single security state which has only two
	 * groups: group 0 and group 1.
	 * Then set GICD_CTLR_ARE and GICD_CTLR_ENABLE_G1 to enable Group 1
	 * interrupt.
	 * Since the GICD_CTLR_ARE and GICD_CTRL_ARE_S share BIT(4), and
	 * similarly the GICD_CTLR_ENABLE_G1 and GICD_CTLR_ENABLE_G1NS share
	 * BIT(1), we can reuse them.
	 */
	sys_write32(BIT(GICD_CTRL_ARE_S) | BIT(GICD_CTLR_ENABLE_G1NS),
		    GICD_CTLR);
#else
	/* enable Group 1 secure interrupts */
	sys_set_bit(GICD_CTLR, GICD_CTLR_ENABLE_G1S);
#endif
}

static uint64_t arm_gic_mpidr_to_affinity(uint64_t mpidr)
{
	uint64_t aff3, aff2, aff1, aff0;

#if defined(CONFIG_ARM)
	/* There is no Aff3 in AArch32 MPIDR */
	aff3 = 0;
#else
	aff3 = MPIDR_AFFLVL(mpidr, 3);
#endif

	aff2 = MPIDR_AFFLVL(mpidr, 2);
	aff1 = MPIDR_AFFLVL(mpidr, 1);
	aff0 = MPIDR_AFFLVL(mpidr, 0);

	return (aff3 << 24 | aff2 << 16 | aff1 << 8 | aff0);
}

static bool arm_gic_aff_matching(uint64_t gicr_aff, uint64_t aff)
{
#if defined(CONFIG_GIC_V3_RDIST_MATCHING_AFF0_ONLY)
	uint64_t mask = BIT64_MASK(8);

	return (gicr_aff & mask) == (aff & mask);
#else
	return gicr_aff == aff;
#endif
}

static inline uint64_t arm_gic_get_typer(mem_addr_t addr)
{
	uint64_t val;

#if defined(CONFIG_ARM)
	val = sys_read32(addr);
	val |= (uint64_t)sys_read32(addr + 4) << 32;
#else
	val = sys_read64(addr);
#endif

	return val;
}

static mem_addr_t arm_gic_iterate_rdists(void)
{
	uint64_t aff = arm_gic_mpidr_to_affinity(GET_MPIDR());

	for (mem_addr_t rdist_addr = GIC_RDIST_BASE;
		rdist_addr < GIC_RDIST_BASE + GIC_RDIST_SIZE;
		rdist_addr += 0x20000) {
		uint64_t val = arm_gic_get_typer(rdist_addr + GICR_TYPER);
		uint64_t gicr_aff = GICR_TYPER_AFFINITY_VALUE_GET(val);

		if (arm_gic_aff_matching(gicr_aff, aff)) {
			return rdist_addr;
		}

		if (GICR_TYPER_LAST_GET(val) == 1) {
			return (mem_addr_t)NULL;
		}
	}

	return (mem_addr_t)NULL;
}

static void __arm_gic_init(void)
{
	uint8_t cpu;
	mem_addr_t gic_rd_base;

	cpu = arch_curr_cpu()->id;
	gic_rd_base = arm_gic_iterate_rdists();
	__ASSERT(gic_rd_base != (mem_addr_t)NULL, "");

	gic_rdists[cpu] = gic_rd_base;

#ifdef CONFIG_GIC_V3_ITS
	/* Enable LPIs in Redistributor */
	gicv3_rdist_setup_lpis(gic_get_rdist());
#endif

	gicv3_rdist_enable(gic_get_rdist());

	gicv3_cpuif_init();
}

int arm_gic_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	gicv3_dist_init();

	__arm_gic_init();

	return 0;
}
SYS_INIT(arm_gic_init, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY);

#ifdef CONFIG_SMP
void arm_gic_secondary_init(void)
{
	__arm_gic_init();

#ifdef CONFIG_GIC_V3_ITS
	/* Map this CPU Redistributor in all the ITS Collection tables */
	its_rdist_map();
#endif
}
#endif
