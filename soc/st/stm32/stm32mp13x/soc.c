/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32MP13 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>

#include <stm32_ll_bus.h>
#include <cmsis_core.h>

#define VECTOR_ADDRESS ((uintptr_t)_vector_start)

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
	/* Update CMSIS SystemCoreClock variable (HCLK) */
	SystemCoreClock = 1000000000U;

	/* Clear TE bit to take exceptions in Thumb mode to fix the DDR init */
	write_sctlr(read_sctlr() & ~SCTLR_TE_Msk);
	barrier_isync_fence_full();
}

static const struct arm_mmu_region mmu_regions[] = {
	MMU_REGION_FLAT_ENTRY("APB1", 0x40000000, 0x19400, MPERM_R | MPERM_W | MT_DEVICE),

	MMU_REGION_FLAT_ENTRY("APB2", 0x44000000, 0x14000, MPERM_R | MPERM_W | MT_DEVICE),

	MMU_REGION_FLAT_ENTRY("AHB2", 0x48000000, 0x1040000, MPERM_R | MPERM_W | MT_DEVICE),

	MMU_REGION_FLAT_ENTRY("APB6", 0x4C000000, 0xC400, MPERM_R | MPERM_W | MT_DEVICE),

	MMU_REGION_FLAT_ENTRY("AHB4", 0x50000000, 0xD400, MPERM_R | MPERM_W | MT_DEVICE),

	MMU_REGION_FLAT_ENTRY("APB3", 0x50020000, 0xA400, MPERM_R | MPERM_W | MT_DEVICE),

	MMU_REGION_FLAT_ENTRY("DEBUG APB", 0x50080000, 0x5D000, MPERM_R | MPERM_W | MT_DEVICE),

	MMU_REGION_FLAT_ENTRY("AHB5", 0x54000000, 0x8000, MPERM_R | MPERM_W | MT_DEVICE),

	MMU_REGION_FLAT_ENTRY("AXIMC", 0x57000000, 0x100000, MPERM_R | MPERM_W | MT_DEVICE),

	MMU_REGION_FLAT_ENTRY("AHB6", 0x58000000, 0x10000, MPERM_R | MPERM_W | MT_DEVICE),

	MMU_REGION_FLAT_ENTRY("APB4", 0x5A000000, 0x7400, MPERM_R | MPERM_W | MT_DEVICE),

	MMU_REGION_FLAT_ENTRY("APB5", 0x5C000000, 0xA400, MPERM_R | MPERM_W | MT_DEVICE),

	MMU_REGION_FLAT_ENTRY("GIC", 0xA0021000, 0x7000, MPERM_R | MPERM_W | MT_DEVICE),

	MMU_REGION_FLAT_ENTRY("vectors", 0xC0000000, 0x1000, MPERM_R | MPERM_X | MT_NORMAL),

	MMU_REGION_FLAT_ENTRY("DAPBUS", 0xE0080000, 0x5D000, MPERM_R | MPERM_W | MT_DEVICE),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
