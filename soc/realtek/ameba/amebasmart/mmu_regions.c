/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "mmu_regions.h"

extern char _vector_start[];
extern char __image2_backtrace_start__[];
extern char __image2_backtrace_end__[];
extern char _image_ram_end[];

/*
 * soc_prep_hook patches the zero sizes of REGION_PSRAM_IMAGE2 and
 * REGION_DRAM_BEYOND before z_arm_mmu_init walks the table.
 */
BUILD_ASSERT(IS_ENABLED(CONFIG_SOC_PREP_HOOK),
	     "amebasmart MMU table requires CONFIG_SOC_PREP_HOOK=y");

enum amebasmart_mmu_region_idx {
	REGION_FLASH = 0,
	REGION_SRAM,
	REGION_IPC_RING,
	REGION_PERIPHERAL,
	REGION_DRAM_BELOW,
	REGION_VECTOR_TABLE,
	REGION_PSRAM_IMAGE2,
	REGION_DRAM_BEYOND,
	REGION_HIGH_PERIPHERAL,
#ifdef CONFIG_MBOX_REALTEK_AMEBA_IPC
	/*
	 * rpmsg vrings; this config shrinks dram0 to end at 0x60700000, so no
	 * overlap with REGION_DRAM_BEYOND. Normal NC, not strongly-ordered,
	 * so OpenAMP's unaligned memcpy is legal.
	 */
	REGION_KM4_VRING,
	REGION_KM0_VRING,
#endif
	REGION_COUNT,
};

/*
 * Static map: the vendor HAL blobs access the peripheral windows directly,
 * without a struct device. The aarch32 MMU code has no zephyr,memory-attr
 * support, so memory regions are listed here too.
 */
static struct arm_mmu_region amebasmart_mmu_regions[REGION_COUNT] = {
	/* Flash XIP 0x08000000 (128 MB), Normal Cacheable R. */
	[REGION_FLASH] = MMU_REGION_FLAT_ENTRY(
		"flash", 0x08000000, MB(128),
		MT_NORMAL | MATTR_SHARED | MATTR_CACHE_OUTER_WB_WA |
			MATTR_CACHE_INNER_WB_WA | MPERM_R |
			MATTR_MAY_MAP_L1_SECTION),

	/* Internal SRAM 0x20000000 (512 MB); the IPC ring is overridden below. */
	[REGION_SRAM] = MMU_REGION_FLAT_ENTRY(
		"sram", 0x20000000, MB(512),
		MT_NORMAL | MATTR_SHARED | MATTR_CACHE_OUTER_WB_WA |
			MATTR_CACHE_INNER_WB_WA | MPERM_R | MPERM_W |
			MATTR_MAY_MAP_L1_SECTION),

	/* KM0<->CA32 IPC ring @0x2301F000 (8 KB), strongly-ordered RW. */
	[REGION_IPC_RING] = MMU_REGION_FLAT_ENTRY(
		"ipc_ring", 0x2301F000, KB(8),
		MT_STRONGLY_ORDERED | MATTR_SHARED | MPERM_R | MPERM_W),

	/* Peripheral bus 0x40000000 (512 MB): RCC, LogUART, PMC, etc. */
	[REGION_PERIPHERAL] = MMU_REGION_FLAT_ENTRY(
		"peripheral", 0x40000000, MB(512),
		MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W |
			MATTR_MAY_MAP_L1_SECTION),

	/* 0x60000000 up to dram0: KM4 DRAM and ATF secure area. */
	[REGION_DRAM_BELOW] = MMU_REGION_FLAT_ENTRY(
		"dram_below", 0x60000000, AMEBASMART_DRAM0_BASE - 0x60000000,
		MT_NORMAL | MATTR_SHARED | MATTR_CACHE_OUTER_WB_WA |
			MATTR_CACHE_INNER_WB_WA | MPERM_R | MPERM_W |
			MATTR_MAY_MAP_L1_SECTION),

	/* ARMv7-A vector table @0x60300000, 4 KB, strongly-ordered R+X. */
	[REGION_VECTOR_TABLE] = MMU_REGION_FLAT_ENTRY(
		"vector_table", POINTER_TO_UINT(_vector_start), KB(4),
		MT_STRONGLY_ORDERED | MPERM_R | MPERM_X),

	/* HAL code in .psram_image2.text.data.  Size patched at boot. */
	[REGION_PSRAM_IMAGE2] = MMU_REGION_FLAT_ENTRY(
		"psram_image2", POINTER_TO_UINT(__image2_backtrace_start__), 0,
		MT_NORMAL | MATTR_SHARED | MATTR_CACHE_OUTER_WB_nWA |
			MATTR_CACHE_INNER_WB_nWA | MPERM_R | MPERM_X),

	/* _image_ram_end - AMEBASMART_DRAM_END. Size patched at boot. */
	[REGION_DRAM_BEYOND] = MMU_REGION_FLAT_ENTRY(
		"dram_beyond", POINTER_TO_UINT(_image_ram_end), 0,
		MT_NORMAL | MATTR_SHARED | MATTR_CACHE_OUTER_WB_WA |
			MATTR_CACHE_INNER_WB_WA | MPERM_R | MPERM_W),

	/* High device window 0x80000000 (1 GB): GIC, debug, secure mirrors. */
	[REGION_HIGH_PERIPHERAL] = MMU_REGION_FLAT_ENTRY(
		"high_peripheral", 0x80000000, GB(1),
		MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W |
			MATTR_MAY_MAP_L1_SECTION),

#ifdef CONFIG_MBOX_REALTEK_AMEBA_IPC
	/* CA32<->KM4 rpmsg vrings @0x60700000 (64 KB), Normal Non-cacheable RW. */
	[REGION_KM4_VRING] = MMU_REGION_FLAT_ENTRY(
		"km4_vring", 0x60700000, KB(64),
		MT_NORMAL | MATTR_SHARED | MPERM_R | MPERM_W),

	/* CA32<->KM0 rpmsg vrings @0x2301B000 (16 KB), overriding REGION_SRAM as NC. */
	[REGION_KM0_VRING] = MMU_REGION_FLAT_ENTRY(
		"km0_vring", 0x2301B000, KB(16),
		MT_NORMAL | MATTR_SHARED | MPERM_R | MPERM_W),
#endif
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(amebasmart_mmu_regions),
	.mmu_regions = amebasmart_mmu_regions,
};

void amebasmart_mmu_set_psram_image2_size(size_t size)
{
	amebasmart_mmu_regions[REGION_PSRAM_IMAGE2].size = size;
}

void amebasmart_mmu_set_dram_beyond_size(uintptr_t image_ram_end)
{
	amebasmart_mmu_regions[REGION_DRAM_BEYOND].size = AMEBASMART_DRAM_END - image_ram_end;
}
