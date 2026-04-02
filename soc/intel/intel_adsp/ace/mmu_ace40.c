/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/xtensa/xtensa_mmu.h>
#include <zephyr/linker/linker-defs.h>
#include <adsp_memory.h>
#include <adsp_imr_layout.h>

#include <inttypes.h>

extern char _shared_heap_start[];
extern char _shared_heap_end[];

#define RPO_SET(addr, reg) (((uintptr_t)addr & 0x1fffffff) | (reg << 29))

#define MEM_MAP_SYM_DECLARE(sym)                                                                   \
	extern char sym##_c[];                                                                     \
	extern char sym##_uc[];

#define MEM_MAP_SYM_C(sym)  ((uint32_t)(uintptr_t)sym##_c)
#define MEM_MAP_SYM_UC(sym) ((uint32_t)(uintptr_t)sym##_uc)

#define MEM_MAP_CONST_C(val) ((uint32_t)RPO_SET((uintptr_t)(val), CONFIG_INTEL_ADSP_CACHED_REGION))

#define MEM_MAP_CONST_UC(val) \
	((uint32_t)RPO_SET((uintptr_t)(val), CONFIG_INTEL_ADSP_UNCACHED_REGION))

#define MEM_MAP_SYM_REGION(sym_start, sym_end, region_attrs, region_name)                          \
	{                                                                                          \
		.start = MEM_MAP_SYM_UC(sym_start),                                                \
		.end   = MEM_MAP_SYM_UC(sym_end),                                                  \
		.attrs = region_attrs,                                                             \
		.name = region_name,                                                               \
	},                                                                                         \
	{                                                                                          \
		.start = MEM_MAP_SYM_C(sym_start),                                                 \
		.end   = MEM_MAP_SYM_C(sym_end),                                                   \
		.attrs = region_attrs | XTENSA_MMU_CACHED_WB,                                      \
		.name = region_name,                                                               \
	},

#define MEM_MAP_CONST_REGION(const_start, const_end, region_attrs, region_name)                    \
	{                                                                                          \
		.start = MEM_MAP_CONST_UC(const_start),                                            \
		.end   = MEM_MAP_CONST_UC(const_end),                                              \
		.attrs = region_attrs,                                                             \
		.name = region_name,                                                               \
	},                                                                                         \
	{                                                                                          \
		.start = MEM_MAP_CONST_C(const_start),                                             \
		.end   = MEM_MAP_CONST_C(const_end),                                               \
		.attrs = region_attrs | XTENSA_MMU_CACHED_WB,                                      \
		.name = region_name,                                                               \
	},

MEM_MAP_SYM_DECLARE(_cached_start);
MEM_MAP_SYM_DECLARE(_cached_end);
MEM_MAP_SYM_DECLARE(__cold_start);
MEM_MAP_SYM_DECLARE(__cold_end);
MEM_MAP_SYM_DECLARE(__coldrodata_start);
MEM_MAP_SYM_DECLARE(_heap_start);
MEM_MAP_SYM_DECLARE(_heap_end);
MEM_MAP_SYM_DECLARE(_shared_heap_start);
MEM_MAP_SYM_DECLARE(_shared_heap_end);
MEM_MAP_SYM_DECLARE(_image_ram_start);
MEM_MAP_SYM_DECLARE(_image_ram_end);
MEM_MAP_SYM_DECLARE(__imr_data_start);
MEM_MAP_SYM_DECLARE(__imr_data_end);
MEM_MAP_SYM_DECLARE(_imr_end);
MEM_MAP_SYM_DECLARE(__common_ram_region_start);
MEM_MAP_SYM_DECLARE(__common_ram_region_end);
MEM_MAP_SYM_DECLARE(__rodata_region_start);
MEM_MAP_SYM_DECLARE(__rodata_region_end);
MEM_MAP_SYM_DECLARE(__text_region_start);
MEM_MAP_SYM_DECLARE(__text_region_end);

const struct xtensa_mmu_range xtensa_soc_mmu_ranges[] = {
	MEM_MAP_SYM_REGION(
		_image_ram_start,
		_image_ram_end,
		XTENSA_MMU_PERM_W,
		"data"
	)

#if K_HEAP_MEM_POOL_SIZE > 0
	MEM_MAP_SYM_REGION(
		_heap_start,
		_heap_end,
		XTENSA_MMU_PERM_W,
		"heap"
	)
#endif

	MEM_MAP_SYM_REGION(
		__text_region_start,
		__text_region_end,
		XTENSA_MMU_PERM_X | XTENSA_MMU_MAP_SHARED,
		"text"
	)

	MEM_MAP_SYM_REGION(
		__rodata_region_start,
		__rodata_region_end,
		XTENSA_MMU_MAP_SHARED,
		"rodata"
	)

	MEM_MAP_SYM_REGION(
		__common_ram_region_start,
		__common_ram_region_end,
		XTENSA_MMU_PERM_W,
		"common-ram"
	)

	/* Workaround for D3 flows. L2 TLB wider than MMU TLB */
	MEM_MAP_CONST_REGION(
		L2_SRAM_BASE,
		VECBASE_RESET_PADDR_SRAM,
		XTENSA_MMU_PERM_W,
		"workaround L2 TLB/MMU TLB"
	)

	MEM_MAP_CONST_REGION(
		VECBASE_RESET_PADDR_SRAM,
		VECBASE_RESET_PADDR_SRAM + VECTOR_TBL_SIZE,
		XTENSA_MMU_PERM_X | XTENSA_MMU_MAP_SHARED,
		"exceptions"
	)

	MEM_MAP_SYM_REGION(
		_cached_start,
		_cached_end,
		XTENSA_MMU_PERM_W,
		"cached"
	)

	MEM_MAP_CONST_REGION(
		HP_SRAM_WIN0_BASE,
		HP_SRAM_WIN0_BASE + HP_SRAM_WIN0_SIZE,
		XTENSA_MMU_PERM_W,
		"win0"
	)

	MEM_MAP_CONST_REGION(
		HP_SRAM_WIN1_BASE,
		HP_SRAM_WIN1_BASE + HP_SRAM_WIN1_SIZE,
		XTENSA_MMU_PERM_W,
		"win1"
	)

	MEM_MAP_CONST_REGION(
		HP_SRAM_WIN2_BASE,
		HP_SRAM_WIN2_BASE + HP_SRAM_WIN2_SIZE,
		XTENSA_MMU_PERM_W,
		"win2"
	)

	MEM_MAP_CONST_REGION(
		HP_SRAM_WIN3_BASE,
		HP_SRAM_WIN3_BASE + HP_SRAM_WIN3_SIZE,
		XTENSA_MMU_PERM_W,
		"win3"
	)

	/* Map IMR */
	MEM_MAP_CONST_REGION(
		IMR_BOOT_LDR_MANIFEST_BASE - IMR_BOOT_LDR_MANIFEST_SIZE,
		IMR_BOOT_LDR_MANIFEST_BASE,
		XTENSA_MMU_PERM_W | XTENSA_MMU_CACHED_WB,
		"imr stack"
	)

	MEM_MAP_CONST_REGION(
		IMR_BOOT_LDR_MANIFEST_BASE,
		IMR_BOOT_LDR_MANIFEST_BASE + IMR_BOOT_LDR_MANIFEST_SIZE,
		0,
		"imr text"
	)

	MEM_MAP_CONST_REGION(
		IMR_BOOT_LDR_BSS_BASE,
		IMR_BOOT_LDR_BSS_BASE + IMR_BOOT_LDR_BSS_SIZE,
		0,
		"imr bss"
	)

	MEM_MAP_CONST_REGION(
		IMR_BOOT_LDR_TEXT_ENTRY_BASE,
		IMR_BOOT_LDR_TEXT_ENTRY_BASE + IMR_BOOT_LDR_TEXT_ENTRY_SIZE,
		XTENSA_MMU_PERM_X | XTENSA_MMU_MAP_SHARED,
		"imr text"
	)

	MEM_MAP_CONST_REGION(
		IMR_BOOT_LDR_STACK_BASE,
		IMR_BOOT_LDR_STACK_BASE + IMR_BOOT_LDR_STACK_SIZE,
		XTENSA_MMU_PERM_W,
		"imr stack"
	)

	MEM_MAP_CONST_REGION(
		IMR_LAYOUT_ADDRESS,
		/* sizeof(struct imr_layout) happens to be 0x1000 (4096) bytes. */
		IMR_LAYOUT_ADDRESS + sizeof(struct imr_layout),
		XTENSA_MMU_PERM_W,
		"imr layout"
	)

	MEM_MAP_SYM_REGION(
		__cold_start,
		__cold_end,
		XTENSA_MMU_PERM_X,
		"imr cold"
	)

	MEM_MAP_SYM_REGION(
		__coldrodata_start,
		_imr_end,
		0,
		"imr coldrodata"
	)

	MEM_MAP_SYM_REGION(
		__imr_data_start,
		__imr_data_end,
		XTENSA_MMU_PERM_W,
		"imr data"
	)

	MEM_MAP_CONST_REGION(
		LP_SRAM_BASE,
		LP_SRAM_BASE + LP_SRAM_SIZE,
		XTENSA_MMU_PERM_W,
		"lpsram"
	)

	MEM_MAP_SYM_REGION(
		_shared_heap_start,
		_shared_heap_end,
		XTENSA_MMU_PERM_W | XTENSA_MMU_MAP_SHARED,
		"shared heap"
	)

	{
		.start = (uint32_t)(ADSP_L1CC_ADDR),
		.end = (uint32_t)(ADSP_L1CC_ADDR + CONFIG_MMU_PAGE_SIZE),
		.attrs = XTENSA_MMU_PERM_W,
		.name = "l1cc",
	},
	{
		/* FIXME: definitely need more refinements... */
		.start = (uint32_t)0x0,
		.end = (uint32_t)0x100000,
		.attrs = XTENSA_MMU_PERM_W,
		.name = "hwreg0",
	},
	{
		/* FIXME: definitely need more refinements... */
		.start = (uint32_t)0x160000,
		.end = (uint32_t)0x180000,
		.attrs = XTENSA_MMU_PERM_W,
		.name = "hwreg1",
	},
};

int xtensa_soc_mmu_ranges_num = ARRAY_SIZE(xtensa_soc_mmu_ranges);
