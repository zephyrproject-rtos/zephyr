/*
 * Copyright 2020 Broadcom
 * Copyright 2024 NXP
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/dt-bindings/interrupt-controller/arm-gic.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/sys/barrier.h>
#include "intc_gic_common_priv.h"
#include "intc_gicv3_priv.h"

#include <string.h>

#define DT_DRV_COMPAT arm_gic_v3

#define GIC_V3_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(DT_DRV_COMPAT)

#define GIC_REDISTRIBUTOR_STRIDE      DT_PROP_OR(GIC_V3_NODE, redistributor_stride, 0)
#define GIC_NUM_REDISTRIBUTOR_REGIONS DT_PROP_OR(GIC_V3_NODE, redistributor_regions, 1)

#define GIC_REG_REGION(idx, node_id)                                                               \
	{                                                                                          \
		.base = DT_REG_ADDR_BY_IDX(node_id, idx),                                          \
		.size = DT_REG_SIZE_BY_IDX(node_id, idx),                                          \
	}

/*
 * Structure to save GIC register region info
 */
struct gic_reg_region {
	mem_addr_t base;
	mem_addr_t size;
};

/*
 * GIC register regions info table
 */
static struct gic_reg_region gic_reg_regions[] = {
	LISTIFY(DT_NUM_REGS(GIC_V3_NODE), GIC_REG_REGION, (,), GIC_V3_NODE) };

/* Redistributor base addresses for each core */
mem_addr_t gic_rdists[CONFIG_MP_MAX_NUM_CPUS];

#if defined(CONFIG_ARMV8_A_NS) || defined(CONFIG_GIC_SINGLE_SECURITY_STATE)
#define IGROUPR_VAL 0xFFFFFFFFU
#else
#define IGROUPR_VAL 0x0U
#endif

/*
 * We allocate memory for PROPBASE to cover 2 ^ lpi_id_bits LPIs to
 * deal with (one configuration byte per interrupt). PENDBASE has to
 * be 64kB aligned (one bit per LPI, plus 8192 bits for SPI/PPI/SGI).
 */
#define ITS_MAX_LPI_NRBITS 16 /* 64K LPIs */

#define LPI_PROPBASE_SZ(nrbits) ROUND_UP(BIT(nrbits), KB(64))
#define LPI_PENDBASE_SZ(nrbits) ROUND_UP(BIT(nrbits) / 8, KB(64))

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

	barrier_dsync_fence_full();

	its_rdist_invall();
}

static void arm_gic_lpi_set_priority(unsigned int intid, unsigned int prio)
{
	uint8_t *cfg = &((uint8_t *)lpi_prop_table)[intid - 8192];

	*cfg &= 0xfc;
	*cfg |= prio & 0xfc;

	barrier_dsync_fence_full();

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
	mem_addr_t addr;

	if (GIC_IS_ESPI(intid)) {
		addr = IROUTERnE(GET_DIST_BASE(intid), intid - GIC_ESPI_START);
	} else {
		addr = IROUTER(GET_DIST_BASE(intid), intid);
	}

#ifdef CONFIG_ARM
	sys_write32((uint32_t)val, addr);
	sys_write32((uint32_t)(val >> 32U), addr + 4);
#else
	sys_write64(val, addr);
#endif
}
#endif

void arm_gic_irq_set_priority(unsigned int intid, unsigned int prio, uint32_t flags)
{
#ifdef CONFIG_GIC_V3_ITS
	if (intid >= 8192) {
		arm_gic_lpi_set_priority(intid, prio);
		return;
	}
#endif
	uint32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	uint32_t idx = GIC_IS_ESPI(intid) ? ((intid - GIC_ESPI_START) / GIC_NUM_INTR_PER_REG)
					  : (intid / GIC_NUM_INTR_PER_REG);
	uint32_t shift;
	uint32_t val;
	mem_addr_t base = GET_DIST_BASE(intid);

	/* Disable the interrupt */
	if (GIC_IS_ESPI(intid)) {
		sys_write32(mask, ICENABLERnE(base, idx));
	} else {
		sys_write32(mask, ICENABLER(base, idx));
	}

	gic_wait_rwp(intid);

	/* PRIORITYR registers provide byte access */
	if (GIC_IS_ESPI(intid)) {
		sys_write8(prio & GIC_PRI_MASK, IPRIORITYRnE(base, intid - GIC_ESPI_START));
	} else {
		sys_write8(prio & GIC_PRI_MASK, IPRIORITYR(base, intid));
	}

	/* Interrupt type config */
	if (!GIC_IS_SGI(intid)) {
		idx = GIC_IS_ESPI(intid) ? ((intid - GIC_ESPI_START) / GIC_NUM_CFG_PER_REG)
					 : (intid / GIC_NUM_CFG_PER_REG);
		shift = (intid & (GIC_NUM_CFG_PER_REG - 1)) * 2;

		val = GIC_IS_ESPI(intid) ? sys_read32(ICFGRnE(base, idx))
					 : sys_read32(ICFGR(base, idx));
		val &= ~(GICD_ICFGR_MASK << shift);
		if (flags & IRQ_TYPE_EDGE) {
			val |= (GICD_ICFGR_TYPE << shift);
		}
		if (GIC_IS_ESPI(intid)) {
			sys_write32(val, ICFGRnE(base, idx));
		} else {
			sys_write32(val, ICFGR(base, idx));
		}
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
	uint32_t idx = GIC_IS_ESPI(intid) ? ((intid - GIC_ESPI_START) / GIC_NUM_INTR_PER_REG)
					  : (intid / GIC_NUM_INTR_PER_REG);

#if defined(CONFIG_ARMV8_A_NS) || defined(CONFIG_GIC_SINGLE_SECURITY_STATE)
	/*
	 * Affinity routing is enabled for Armv8-A Non-secure state (GICD_CTLR.ARE_NS
	 * is set to '1') and for GIC single security state (GICD_CTRL.ARE is set to '1'),
	 * so need to set SPI's affinity, now set it to be the PE on which it is enabled.
	 */
	if (GIC_IS_SPI(intid) || GIC_IS_ESPI(intid)) {
		arm_gic_write_irouter(MPIDR_TO_CORE(GET_MPIDR()), intid);
	}
#endif

	if (GIC_IS_ESPI(intid)) {
		sys_write32(mask, ISENABLERnE(GET_DIST_BASE(intid), idx));
	} else {
		sys_write32(mask, ISENABLER(GET_DIST_BASE(intid), idx));
	}
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
	uint32_t idx = GIC_IS_ESPI(intid) ? ((intid - GIC_ESPI_START) / GIC_NUM_INTR_PER_REG)
					  : (intid / GIC_NUM_INTR_PER_REG);

	if (GIC_IS_ESPI(intid)) {
		sys_write32(mask, ICENABLERnE(GET_DIST_BASE(intid), idx));
	} else {
		sys_write32(mask, ICENABLER(GET_DIST_BASE(intid), idx));
	}

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
	uint32_t idx = GIC_IS_ESPI(intid) ? ((intid - GIC_ESPI_START) / GIC_NUM_INTR_PER_REG)
					  : (intid / GIC_NUM_INTR_PER_REG);
	uint32_t val;

	val = GIC_IS_ESPI(intid) ? sys_read32(ISENABLERnE(GET_DIST_BASE(intid), idx))
				 : sys_read32(ISENABLER(GET_DIST_BASE(intid), idx));

	return (val & mask) != 0;
}

bool arm_gic_irq_is_pending(unsigned int intid)
{
	uint32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	uint32_t idx = GIC_IS_ESPI(intid) ? ((intid - GIC_ESPI_START) / GIC_NUM_INTR_PER_REG)
					  : (intid / GIC_NUM_INTR_PER_REG);
	uint32_t val;

	val = GIC_IS_ESPI(intid) ? sys_read32(ISPENDRnE(GET_DIST_BASE(intid), idx))
				 : sys_read32(ISPENDR(GET_DIST_BASE(intid), idx));

	return (val & mask) != 0;
}

void arm_gic_irq_set_pending(unsigned int intid)
{
	uint32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	uint32_t idx = GIC_IS_ESPI(intid) ? ((intid - GIC_ESPI_START) / GIC_NUM_INTR_PER_REG)
					  : (intid / GIC_NUM_INTR_PER_REG);

	if (GIC_IS_ESPI(intid)) {
		sys_write32(mask, ISPENDRnE(GET_DIST_BASE(intid), idx));
	} else {
		sys_write32(mask, ISPENDR(GET_DIST_BASE(intid), idx));
	}
}

void arm_gic_irq_clear_pending(unsigned int intid)
{
	uint32_t mask = BIT(intid & (GIC_NUM_INTR_PER_REG - 1));
	uint32_t idx = GIC_IS_ESPI(intid) ? ((intid - GIC_ESPI_START) / GIC_NUM_INTR_PER_REG)
					  : (intid / GIC_NUM_INTR_PER_REG);

	if (GIC_IS_ESPI(intid)) {
		sys_write32(mask, ICPENDRnE(GET_DIST_BASE(intid), idx));
	} else {
		sys_write32(mask, ICPENDR(GET_DIST_BASE(intid), idx));
	}
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
	barrier_dsync_fence_full();

	/* (AP -> Pending) Or (Active -> Inactive) or (AP to AP) nested case */
	write_sysreg(intid, ICC_EOIR1_EL1);
}

void gic_raise_sgi(unsigned int sgi_id, uint64_t target_aff, uint16_t target_list)
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
	sgi_val = GICV3_SGIR_VALUE(aff3, aff2, aff1, sgi_id, SGIR_IRM_TO_AFF, target_list);

	barrier_dsync_fence_full();
	write_sysreg(sgi_val, ICC_SGI1R);
	barrier_isync_fence_full();
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

	if (GICR_IIDR_PRODUCT_ID_GET(sys_read32(rdist + GICR_IIDR)) >= 0x2) {
		if (sys_read32(rdist + GICR_PWRR) & BIT(GICR_PWRR_RDPD)) {
			sys_set_bit(rdist + GICR_PWRR, GICR_PWRR_RDAG);
			sys_clear_bit(rdist + GICR_PWRR, GICR_PWRR_RDPD);
			while (sys_read32(rdist + GICR_PWRR) & BIT(GICR_PWRR_RDPD)) {
				;
			}
		}
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
	unsigned int lpi_id_bits =
		MIN(GICD_TYPER_IDBITS(sys_read32(GICD_TYPER)), ITS_MAX_LPI_NRBITS);
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
	      (GIC_BASER_CACHE_INNERLIKE << GITR_PENDBASER_OUTER_CACHE_SHIFT) | GITR_PENDBASER_PTZ;
	sys_write64(reg, rdist + GICR_PENDBASER);
	/* TOFIX: check SHAREABILITY validity */

	ctlr = sys_read32(rdist + GICR_CTLR);
	ctlr |= GICR_CTLR_ENABLE_LPIS;
	sys_write32(ctlr, rdist + GICR_CTLR);

	barrier_dsync_fence_full();
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
	for (intid = 0; intid < GIC_SPI_INT_BASE; intid += GIC_NUM_PRI_PER_REG) {
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
		icc_sre =
			(icc_sre | ICC_SRE_ELx_SRE_BIT | ICC_SRE_ELx_DIB_BIT | ICC_SRE_ELx_DFB_BIT);
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
	unsigned int num_ints, num_espi;
	unsigned int intid;
	unsigned int idx;
	mem_addr_t base = GIC_DIST_BASE;

#ifdef CONFIG_GIC_SAFE_CONFIG
	/*
	 * Currently multiple OSes can run one the different CPU Cores which share single GIC,
	 * but GIC distributor should avoid to be re-configured in order to avoid crash the
	 * OSes has already been started.
	 */
	if (sys_read32(GICD_CTLR) & (BIT(GICD_CTLR_ENABLE_G0) | BIT(GICD_CTLR_ENABLE_G1NS))) {
		return;
	}
#endif

	num_ints = sys_read32(GICD_TYPER);
	num_espi = (num_ints & GICD_TYPER_ESPI_MASK) ? ((num_ints >> 27) + 1) * 32 : 0;
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
	for (intid = GIC_SPI_INT_BASE; intid < num_ints; intid += GIC_NUM_INTR_PER_REG) {
		idx = intid / GIC_NUM_INTR_PER_REG;
		/* Disable interrupt */
		sys_write32(BIT64_MASK(GIC_NUM_INTR_PER_REG), ICENABLER(base, idx));
		/* Clear pending */
		sys_write32(BIT64_MASK(GIC_NUM_INTR_PER_REG), ICPENDR(base, idx));
		sys_write32(IGROUPR_VAL, IGROUPR(base, idx));
		sys_write32(BIT64_MASK(GIC_NUM_INTR_PER_REG), IGROUPMODR(base, idx));
	}

	/*
	 * Default configuration of all ESPIs
	 */
	for (intid = 0; intid < num_espi; intid += GIC_NUM_INTR_PER_REG) {
		idx = intid / GIC_NUM_INTR_PER_REG;
		/* Disable interrupt */
		sys_write32(BIT64_MASK(GIC_NUM_INTR_PER_REG), ICENABLERnE(base, idx));
		/* Clear pending */
		sys_write32(BIT64_MASK(GIC_NUM_INTR_PER_REG), ICPENDRnE(base, idx));
		sys_write32(IGROUPR_VAL, IGROUPRnE(base, idx));
		sys_write32(BIT64_MASK(GIC_NUM_INTR_PER_REG), IGROUPMODRnE(base, idx));
	}

	/* wait for rwp on GICD */
	gic_wait_rwp(GIC_SPI_INT_BASE);

	/* Configure default priorities for all SPIs. */
	for (intid = GIC_SPI_INT_BASE; intid < num_ints; intid += GIC_NUM_PRI_PER_REG) {
		sys_write32(GIC_INT_DEF_PRI_X4, IPRIORITYR(base, intid));
	}

	/* Configure default priorities for all ESPIs. */
	for (intid = 0; intid < num_espi; intid += GIC_NUM_PRI_PER_REG) {
		sys_write32(GIC_INT_DEF_PRI_X4, IPRIORITYRnE(base, intid));
	}

	/* Configure all SPIs as active low, level triggered by default */
	for (intid = GIC_SPI_INT_BASE; intid < num_ints; intid += GIC_NUM_CFG_PER_REG) {
		idx = intid / GIC_NUM_CFG_PER_REG;
		sys_write32(0, ICFGR(base, idx));
	}

	/* Configure all ESPIs as active low, level triggered by default */
	for (intid = 0; intid < num_espi; intid += GIC_NUM_CFG_PER_REG) {
		idx = intid / GIC_NUM_CFG_PER_REG;
		sys_write32(0, ICFGRnE(base, idx));
	}

#ifdef CONFIG_ARMV8_A_NS
	/* Enable distributor with ARE */
	sys_write32(BIT(GICD_CTRL_ARE_NS) | BIT(GICD_CTLR_ENABLE_G1NS), GICD_CTLR);
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
	sys_write32(BIT(GICD_CTRL_ARE_S) | BIT(GICD_CTLR_ENABLE_G1NS), GICD_CTLR);
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
	uint32_t idx;

	/* Skip the first array entry as it refers to the GIC distributor */
	for (idx = 1; idx < GIC_NUM_REDISTRIBUTOR_REGIONS + 1; idx++) {
		uint64_t val;
		mem_addr_t rdist_addr = gic_reg_regions[idx].base;
		mem_addr_t rdist_end = rdist_addr + gic_reg_regions[idx].size;

		do {
			val = arm_gic_get_typer(rdist_addr + GICR_TYPER);
			uint64_t gicr_aff = GICR_TYPER_AFFINITY_VALUE_GET(val);

			if (arm_gic_aff_matching(gicr_aff, aff)) {
				return rdist_addr;
			}

			if (GIC_REDISTRIBUTOR_STRIDE > 0) {
				rdist_addr += GIC_REDISTRIBUTOR_STRIDE;
			} else {
				/*
				 * Skip RD_base and SGI_base
				 * In GICv3, GICR_TYPER.VLPIS bit is RES0 and can can be ignored
				 * as there are no VLPI and reserved pages.
				 */
				rdist_addr += KB(64) * 2;
			}

		} while ((!GICR_TYPER_LAST_GET(val)) && (rdist_addr < rdist_end));
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

int arm_gic_init(const struct device *dev)
{
	gicv3_dist_init();

	__arm_gic_init();

	return 0;
}
DEVICE_DT_INST_DEFINE(0, arm_gic_init, NULL, NULL, NULL, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,
		      NULL);

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
