/*
 * Copyright (c) 2021 Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <cmsis_core.h>
#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include "soc.h"

#ifdef CONFIG_CACHE_PL310
#include <zephyr/drivers/cache/pl310.h>
#endif

/* System Level Control Registers (SLCR) */
#define SLCR_UNLOCK     0x0008
#define SLCR_UNLOCK_KEY 0xdf0d

/*
 * L2C RAM read/write latency control (Xilinx design advisory AR#54190). Must be
 * programmed before the PL310 is enabled. Mirrors the Linux Zynq mach code:
 * regmap_update_bits(SLCR_L2C_RAM, 0x70707, 0x20202) - a read-modify-write that
 * touches only the latency fields.
 */
#define SLCR_L2C_RAM      0x0A1C
#define SLCR_L2C_RAM_MASK 0x00070707
#define SLCR_L2C_RAM_VAL  0x00020202
#define AXI_GPIO_MMU_ENTRY(id)\
	MMU_REGION_FLAT_ENTRY("axigpio",\
			      DT_REG_ADDR(id),\
			      DT_REG_SIZE(id),\
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),

static const struct arm_mmu_region mmu_regions[] = {

	MMU_REGION_FLAT_ENTRY("vectors",
			      0x00000000,
			      0x1000,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_X),
	MMU_REGION_FLAT_ENTRY("mpcore",
			      0xF8F00000,
			      0x2000,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),
	MMU_REGION_FLAT_ENTRY("ocm",
			      DT_REG_ADDR(DT_CHOSEN(zephyr_ocm)),
			      DT_REG_SIZE(DT_CHOSEN(zephyr_ocm)),
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),
	/* ARM Arch timer, GIC are covered by the MPCore mapping */

#ifdef CONFIG_CACHE_PL310
	/*
	 * PL310 (L2C-310) register bank. It sits at the very end of the MPCore
	 * region (0xF8F00000 + 0x2000 = 0xF8F02000), so it is not covered by
	 * that mapping and needs its own; the outer-cache range ops touch these
	 * registers once the MMU is on. Unlike the GEMs/SLCR, there is no device
	 * node that maps it at runtime, so it must be in the static table.
	 */
	MMU_REGION_FLAT_ENTRY("pl310",
			      DT_REG_ADDR(DT_NODELABEL(l2)),
			      DT_REG_SIZE(DT_NODELABEL(l2)),
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),
#endif

DT_FOREACH_STATUS_OKAY(xlnx_xps_gpio_1_00_a, AXI_GPIO_MMU_ENTRY)

};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};

/* Platform-specific early initialization */

void soc_reset_hook(void)
{
	/*
	 * When coming out of u-boot rather than downloading the Zephyr binary
	 * via JTAG, a few things modified by u-boot have to be re-set to a
	 * suitable default value for Zephyr to run, namely:
	 *
	 * - u-boot places the exception vectors somewhere in RAM and then
	 *   lets the VBAR register point to them. Zephyr uses the default
	 *   vector table location at address zero (and maybe at some later
	 *   time alternatively the HIVECS position). If VBAR isn't reset
	 *   to zero, the system crashes during the first context switch when
	 *   SVC is invoked.
	 * - u-boot sets the following bits in the SCTLR register:
	 *   - [I] ICache enable
	 *   - [C] DCache enable
	 *   - [Z] Branch prediction enable
	 *   - [A] Enforce strict alignment enable
	 *   [I] and [C] will be enabled during the MMU init -> disable them
	 *   until then. [Z] is probably not harmful. [A] will cause a crash
	 *   as early as z_mem_manage_init when an unaligned access is performed
	 *   -> clear [A].
	 */

	uint32_t vbar = 0;

	__set_VBAR(vbar);

	uint32_t sctlr = __get_SCTLR();

	sctlr &= ~SCTLR_I_Msk;
	sctlr &= ~SCTLR_C_Msk;
	sctlr &= ~SCTLR_A_Msk;
	__set_SCTLR(sctlr);

#ifdef CONFIG_CACHE_PL310
	/*
	 * Cortex-A9: enable SMP / coherency participation (ACTLR.SMP, bit 6).
	 *
	 * On the A9 the load/store-exclusive monitor (LDREX/STREX) for Normal
	 * Shareable Cacheable memory -- which is how Zephyr maps its data
	 * (MATTR_SHARED) -- and the SCU cache coherency logic only function when
	 * ACTLR.SMP == 1. With SMP == 0 a STREX to such memory never succeeds, so
	 * the first atomic op (e.g. atomic_inc in logging init) spins forever.
	 * This is masked while the outer cache is off (shareable WB then behaves
	 * as non-cacheable and exclusives resolve trivially), which is why the
	 * board only hangs once the PL310 is enabled. Set it here, with caches
	 * still off, before anything atomic runs. Harmless on a single-core boot.
	 */
	{
		uint32_t actlr = __get_ACTLR();

		actlr |= BIT(6);
		__set_ACTLR(actlr);
	}
#endif /* CONFIG_CACHE_PL310 */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(slcr))
	mm_reg_t addr = DT_REG_ADDR(DT_NODELABEL(slcr));

	/* Unlock System Level Control Registers (SLCR) */
	sys_write32(SLCR_UNLOCK_KEY, addr + SLCR_UNLOCK);

#ifdef CONFIG_CACHE_PL310
	/* AR#54190: fix L2C RAM write-latency before enabling PL310. */
	{
		uint32_t l2c_ram = sys_read32(addr + SLCR_L2C_RAM);

		l2c_ram &= ~SLCR_L2C_RAM_MASK;
		l2c_ram |= SLCR_L2C_RAM_VAL;
		sys_write32(l2c_ram, addr + SLCR_L2C_RAM);
	}
#endif /* CONFIG_CACHE_PL310 */
#endif /* slcr okay */

#ifdef CONFIG_CACHE_PL310
	/* Enable the PL310 outer cache while the MMU and L1 are still off. */
	z_pl310_early_enable();
#endif
}
