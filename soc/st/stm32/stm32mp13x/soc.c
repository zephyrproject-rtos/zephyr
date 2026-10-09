/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32MP13 processor
 */

#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>

#include <stm32_ll_bus.h>
#include <cmsis_core.h>

#define VECTOR_ADDRESS ((uintptr_t)_vector_start)

#define DEVICE_RO (MPERM_R | MT_DEVICE | MATTR_MAY_MAP_L1_SECTION)
#define DEVICE_RW (MPERM_R | MPERM_W | MT_DEVICE | MATTR_MAY_MAP_L1_SECTION)
#define NORMAL_RX                                                                                  \
	(MT_NORMAL | MATTR_SHARED | MPERM_R | MPERM_X | MATTR_CACHE_OUTER_WB_WA |                  \
	 MATTR_CACHE_INNER_WB_WA | MATTR_MAY_MAP_L1_SECTION)

void relocate_vector_table(void)
{
	write_sctlr(read_sctlr() & ~HIVECS);
	write_vbar(VECTOR_ADDRESS & VBAR_MASK);
	barrier_isync_fence_full();
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */

void soc_early_init_hook(void)
{
	/* Record the clock established by the previous boot stage. */
	SystemCoreClockUpdate();

	/* Clear TE bit to take exceptions in Thumb mode to fix the DDR init */
	write_sctlr(read_sctlr() & ~SCTLR_TE_Msk);
	barrier_isync_fence_full();
}

static const struct arm_mmu_region mmu_regions[] = {
	/* 0x40000000 */ MMU_REGION_FLAT_ENTRY("PERIPH",   PERIPH_BASE,         MB(512), DEVICE_RW),
	/* 0x60000000 */ MMU_REGION_FLAT_ENTRY("FMC_NOR",  FMC_NOR_MEM_BASE,    MB(256), DEVICE_RO),
	/* 0x70000000 */ MMU_REGION_FLAT_ENTRY("QSPI",     QSPI_MEM_BASE,       MB(256), DEVICE_RO),
	/* 0x80000000 */ MMU_REGION_FLAT_ENTRY("FMC_NAND", FMC_NAND_MEM_BASE,   MB(256), DEVICE_RW),
	/* 0xA0021000 */ MMU_REGION_FLAT_ENTRY("GIC",      GIC_BASE,            KB(28),  DEVICE_RW),
	/*            */ MMU_REGION_FLAT_ENTRY("vectors",  VECTOR_ADDRESS,      KB(4),   NORMAL_RX),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
