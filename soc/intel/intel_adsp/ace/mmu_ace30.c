/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/xtensa/xtensa_mmu.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/util.h>
#include <adsp_memory.h>
#include <adsp_imr_layout.h>
#include <intel_adsp_ipc_devtree.h>

#include <xtensa_mmu_priv.h>

#include <inttypes.h>

extern char _cached_start[];
extern char _cached_end[];
extern char _imr_start[];
extern char _imr_end[];
extern char __common_rom_region_start[];
extern char __common_rom_region_end[];
extern char __common_ram_region_start[];
extern char __common_ram_region_end[];

const struct xtensa_mmu_range xtensa_soc_mmu_ranges[] = {
	{
		.start = (uint32_t)__common_ram_region_start,
		.end   = (uint32_t)__common_ram_region_end,
		.attrs = XTENSA_MMU_PERM_W,
		.name = "common-ram",
	},
	{
		/* Workaround for D3 flows. L2 TLB wider than MMU TLB */
		.start = (uint32_t)L2_SRAM_BASE,
		.end   = (uint32_t)VECBASE_RESET_PADDR_SRAM,
		.attrs = XTENSA_MMU_PERM_W | XTENSA_MMU_CACHED_WB,
		.name = "workaround L2 TLB/MMU TLB",
	},
	{
		.start = (uint32_t)VECBASE_RESET_PADDR_SRAM,
		.end   = (uint32_t)VECBASE_RESET_PADDR_SRAM + VECTOR_TBL_SIZE,
		.attrs = XTENSA_MMU_PERM_X | XTENSA_MMU_MAP_SHARED,
		.name = "exceptions",
	},
	{
		.start = (uint32_t)_cached_start,
		.end   = (uint32_t)_cached_end,
		.attrs = XTENSA_MMU_PERM_W | XTENSA_MMU_CACHED_WB,
		.name = "cached",
	},
	{
		.start = (uint32_t)HP_SRAM_WIN0_BASE,
		.end   = (uint32_t)HP_SRAM_WIN0_BASE + (uint32_t)HP_SRAM_WIN0_SIZE,
		.attrs = XTENSA_MMU_PERM_W,
		.name = "win0",
	},
	{
		.start = (uint32_t)HP_SRAM_WIN1_BASE,
		.end   = (uint32_t)HP_SRAM_WIN1_BASE + (uint32_t)HP_SRAM_WIN1_SIZE,
		.attrs = XTENSA_MMU_PERM_W,
		.name = "win2",
	},
	{
		.start = (uint32_t)HP_SRAM_WIN2_BASE,
		.end   = (uint32_t)HP_SRAM_WIN2_BASE + (uint32_t)HP_SRAM_WIN2_SIZE,
		.attrs = XTENSA_MMU_PERM_W,
		.name = "win2",
	},
	{
		.start = (uint32_t)HP_SRAM_WIN3_BASE,
		.end   = (uint32_t)HP_SRAM_WIN3_BASE + (uint32_t)HP_SRAM_WIN3_SIZE,
		.attrs = XTENSA_MMU_PERM_W,
		.name = "win3",
	},
	/* Map IMR */
	{
		.start = (uint32_t)(IMR_BOOT_LDR_MANIFEST_BASE - IMR_BOOT_LDR_MANIFEST_SIZE),
		.end   = (uint32_t)IMR_BOOT_LDR_MANIFEST_BASE,
		.attrs = XTENSA_MMU_PERM_W | XTENSA_MMU_CACHED_WB,
		.name = "imr stack",
	},
	{
		.start = (uint32_t)IMR_BOOT_LDR_MANIFEST_BASE,
		.end   = (uint32_t)(IMR_BOOT_LDR_MANIFEST_BASE + IMR_BOOT_LDR_MANIFEST_SIZE),
		.name = "imr text",
	},
	{
		.start = (uint32_t)IMR_BOOT_LDR_BSS_BASE,
		.end   = (uint32_t)(IMR_BOOT_LDR_BSS_BASE + IMR_BOOT_LDR_BSS_SIZE),
		.name = "imr bss",
	},
	{
		.start = (uint32_t)IMR_BOOT_LDR_TEXT_ENTRY_BASE,
		.end   = (uint32_t)(IMR_BOOT_LDR_TEXT_ENTRY_BASE + IMR_BOOT_LDR_TEXT_ENTRY_SIZE),
		.attrs = XTENSA_MMU_PERM_X | XTENSA_MMU_MAP_SHARED,
		.name = "imr text",
	},
	{
		.start = (uint32_t)IMR_BOOT_LDR_STACK_BASE,
		.end   = (uint32_t)(IMR_BOOT_LDR_STACK_BASE + IMR_BOOT_LDR_STACK_SIZE),
		.attrs = XTENSA_MMU_PERM_W | XTENSA_MMU_CACHED_WB,
		.name = "imr stack",
	},
	{
		.start = (uint32_t)IMR_LAYOUT_ADDRESS,
		/* sizeof(struct imr_layout) happens to be 0x1000 (4096) bytes. */
		.end   = (uint32_t)(IMR_LAYOUT_ADDRESS + sizeof(struct imr_layout)),
		.attrs = XTENSA_MMU_PERM_W | XTENSA_MMU_CACHED_WB,
		.name = "imr layout",
	},
	{
		.start = (uint32_t)IMR_L3_HEAP_BASE,
		.end   = (uint32_t)(IMR_L3_HEAP_BASE + IMR_L3_HEAP_SIZE),
		.attrs = XTENSA_MMU_PERM_W | XTENSA_MMU_CACHED_WB,
		.name = "imr L3 heap",
	},
	{
		.start = (uint32_t)LP_SRAM_BASE,
		.end   = (uint32_t)(LP_SRAM_BASE + LP_SRAM_SIZE),
		.attrs = XTENSA_MMU_PERM_W | XTENSA_MMU_CACHED_WB,
		.name = "lpsram",
	},
	{
		.start = (uint32_t)(ADSP_L1CC_ADDR),
		.end   = (uint32_t)(ADSP_L1CC_ADDR + CONFIG_MMU_PAGE_SIZE),
		.attrs = XTENSA_MMU_PERM_W,
		.name = "l1cc",
	},
	{
		/* FIXME: definitely need more refinements... */
		.start = (uint32_t)0x0,
		.end   = (uint32_t)0x100000,
		.attrs = XTENSA_MMU_PERM_W,
		.name = "hwreg0",
	},
	{
		/* FIXME: definitely need more refinements... */
		.start = (uint32_t)0x170000,
		.end   = (uint32_t)0x180000,
		.attrs = XTENSA_MMU_PERM_W,
		.name = "hwreg1",
	},
};

int xtensa_soc_mmu_ranges_num = ARRAY_SIZE(xtensa_soc_mmu_ranges);

void arch_xtensa_mmu_post_init(bool is_core0)
{
	ARG_UNUSED(is_core0);

	/*
	 * Mapping the Host IPC MMIO registers into TLB way 9.
	 * The registers are R/W and ring 0 only.
	 */
	uint32_t ipc_addr_aligned = ROUND_DOWN(INTEL_ADSP_IPC_REG_ADDRESS, CONFIG_MMU_PAGE_SIZE);
	uint32_t ipc_tlb_as = (ipc_addr_aligned & XTENSA_MMU_PTE_VPN_MASK) | 0x09U;
	uint32_t ipc_tlb_at =
		XTENSA_MMU_PTE(ipc_addr_aligned, XTENSA_MMU_KERNEL_RING, 0, XTENSA_MMU_PERM_W);

	xtensa_dtlb_entry_write_sync(ipc_tlb_at, ipc_tlb_as);
}
