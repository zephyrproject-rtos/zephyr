/*
 * Copyright (c) 2021 Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/arch/arm/aarch32/mmu/arm_mmu.h>
#include "soc.h"

/* System Level Configuration Registers */
#define SLCR_UNLOCK     0x0008
#define SLCR_UNLOCK_KEY 0xdf0d

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

/* UARTs */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay)
	MMU_REGION_FLAT_ENTRY("uart0",
			      DT_REG_ADDR(DT_NODELABEL(uart0)),
			      DT_REG_SIZE(DT_NODELABEL(uart0)),
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	MMU_REGION_FLAT_ENTRY("uart1",
			      DT_REG_ADDR(DT_NODELABEL(uart1)),
			      DT_REG_SIZE(DT_NODELABEL(uart1)),
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),
#endif

/* GEMs */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gem0), okay)
	MMU_REGION_FLAT_ENTRY("gem0",
			      DT_REG_ADDR(DT_NODELABEL(gem0)),
			      DT_REG_SIZE(DT_NODELABEL(gem0)),
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gem1), okay)
	MMU_REGION_FLAT_ENTRY("gem1",
			      DT_REG_ADDR(DT_NODELABEL(gem1)),
			      DT_REG_SIZE(DT_NODELABEL(gem1)),
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),
#endif

/* GPIO controller */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(psgpio), okay)
	MMU_REGION_FLAT_ENTRY("psgpio",
			      DT_REG_ADDR(DT_NODELABEL(psgpio)),
			      DT_REG_SIZE(DT_NODELABEL(psgpio)),
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),
#endif

};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};

/**
 * @brief Basic hardware initialization of the Zynq-7000 SoC
 *
 * Performs the basic initialization of the Zynq-7000 SoC.
 *
 * @return 0
 */
static int soc_xlnx_zynq7000s_init(const struct device *arg)
{
	ARG_UNUSED(arg);
	NMI_INIT();

	return 0;
}

SYS_INIT(soc_xlnx_zynq7000s_init, PRE_KERNEL_1,
	CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/* Platform-specific early initialization */

void z_arm_platform_init(void)
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

#if DT_NODE_HAS_STATUS(DT_NODELABEL(slcr), okay)
	mm_reg_t addr = DT_REG_ADDR(DT_NODELABEL(slcr));

	/* Unlock System Level Control Registers (SLCR) */
	sys_write32(SLCR_UNLOCK_KEY, addr + SLCR_UNLOCK);
#endif
}
