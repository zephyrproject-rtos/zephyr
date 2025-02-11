/*
 * ARMv7 MMU support
 *
 * Private data declarations
 *
 * Copyright (c) 2021 Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_AARCH32_ARM_MMU_PRIV_H_
#define ZEPHYR_ARCH_AARCH32_ARM_MMU_PRIV_H_

/*
 * Comp.:
 * ARM Architecture Reference Manual, ARMv7-A and ARMv7-R edition
 * ARM document ID DDI0406C Rev. d, March 2018
 * L1 / L2 page table entry formats and entry type IDs:
 * Chapter B3.5.1, fig. B3-4 and B3-5, p. B3-1323 f.
 */

#define ARM_MMU_PT_L1_NUM_ENTRIES		4096
#define ARM_MMU_PT_L2_NUM_ENTRIES		256

#define ARM_MMU_PTE_L1_INDEX_PA_SHIFT		20
#define ARM_MMU_PTE_L1_INDEX_MASK		0xFFF
#define ARM_MMU_PTE_L2_INDEX_PA_SHIFT		12
#define ARM_MMU_PTE_L2_INDEX_MASK		0xFF
#define ARM_MMU_PT_L2_ADDR_SHIFT		10
#define ARM_MMU_PT_L2_ADDR_MASK			0x3FFFFF
#define ARM_MMU_PTE_L2_SMALL_PAGE_ADDR_SHIFT	12
#define ARM_MMU_PTE_L2_SMALL_PAGE_ADDR_MASK	0xFFFFF
#define ARM_MMU_ADDR_BELOW_PAGE_GRAN_MASK	0xFFF

#define ARM_MMU_PTE_ID_INVALID			0x0
#define ARM_MMU_PTE_ID_L2_PT			0x1
#define ARM_MMU_PTE_ID_SECTION			0x2
#define ARM_MMU_PTE_ID_LARGE_PAGE		0x1
#define ARM_MMU_PTE_ID_SMALL_PAGE		0x2

#define ARM_MMU_PERMS_AP2_DISABLE_WR		0x2
#define ARM_MMU_PERMS_AP1_ENABLE_PL0		0x1
#define ARM_MMU_TEX2_CACHEABLE_MEMORY		0x4

#define ARM_MMU_TEX_CACHE_ATTRS_WB_WA		0x1
#define ARM_MMU_TEX_CACHE_ATTRS_WT_nWA		0x2
#define ARM_MMU_TEX_CACHE_ATTRS_WB_nWA		0x3
#define ARM_MMU_C_CACHE_ATTRS_WB_WA		0
#define ARM_MMU_B_CACHE_ATTRS_WB_WA		1
#define ARM_MMU_C_CACHE_ATTRS_WT_nWA		1
#define ARM_MMU_B_CACHE_ATTRS_WT_nWA		0
#define ARM_MMU_C_CACHE_ATTRS_WB_nWA		1
#define ARM_MMU_B_CACHE_ATTRS_WB_nWA		1

/*
 * The following defines might vary if support for CPUs without
 * the multiprocessor extensions was to be implemented:
 */

#define ARM_MMU_TTBR_IRGN0_BIT_MP_EXT_ONLY	BIT(6)
#define ARM_MMU_TTBR_NOS_BIT			BIT(5)
#define ARM_MMU_TTBR_RGN_OUTER_NON_CACHEABLE	0x0
#define ARM_MMU_TTBR_RGN_OUTER_WB_WA_CACHEABLE	0x1
#define ARM_MMU_TTBR_RGN_OUTER_WT_CACHEABLE	0x2
#define ARM_MMU_TTBR_RGN_OUTER_WB_nWA_CACHEABLE	0x3
#define ARM_MMU_TTBR_RGN_SHIFT			3
#define ARM_MMU_TTBR_SHAREABLE_BIT		BIT(1)
#define ARM_MMU_TTBR_IRGN1_BIT_MP_EXT_ONLY	BIT(0)
#define ARM_MMU_TTBR_CACHEABLE_BIT_NON_MP_ONLY	BIT(0)

/* <-- end MP-/non-MP-specific */

#define ARM_MMU_DOMAIN_OS			0
#define ARM_MMU_DOMAIN_DEVICE			1
#define ARM_MMU_DACR_ALL_DOMAINS_CLIENT		0x55555555

#define ARM_MMU_SCTLR_AFE_BIT			BIT(29)
#define ARM_MMU_SCTLR_TEX_REMAP_ENABLE_BIT	BIT(28)
#define ARM_MMU_SCTLR_HA_BIT			BIT(17)
#define ARM_MMU_SCTLR_ICACHE_ENABLE_BIT		BIT(12)
#define ARM_MMU_SCTLR_DCACHE_ENABLE_BIT		BIT(2)
#define ARM_MMU_SCTLR_CHK_ALIGN_ENABLE_BIT	BIT(1)
#define ARM_MMU_SCTLR_MMU_ENABLE_BIT		BIT(0)

#define ARM_MMU_L2_PT_INDEX(pt) ((uint32_t)pt - (uint32_t)l2_page_tables) /\
				sizeof(struct arm_mmu_l2_page_table);

union arm_mmu_l1_page_table_entry {
	struct {
		uint32_t id			: 2;  /* [00] */
		uint32_t bufferable		: 1;
		uint32_t cacheable		: 1;
		uint32_t exec_never		: 1;
		uint32_t domain			: 4;
		uint32_t impl_def		: 1;
		uint32_t acc_perms10		: 2;
		uint32_t tex			: 3;
		uint32_t acc_perms2		: 1;
		uint32_t shared			: 1;
		uint32_t not_global		: 1;
		uint32_t zero			: 1;
		uint32_t non_sec		: 1;
		uint32_t base_address		: 12; /* [31] */
	} l1_section_1m;
	struct {
		uint32_t id			: 2;  /* [00] */
		uint32_t zero0			: 1;  /* PXN if avail. */
		uint32_t non_sec		: 1;
		uint32_t zero1			: 1;
		uint32_t domain			: 4;
		uint32_t impl_def		: 1;
		uint32_t l2_page_table_address	: 22; /* [31] */
	} l2_page_table_ref;
	struct {
		uint32_t id			: 2;  /* [00] */
		uint32_t reserved		: 30; /* [31] */
	} undefined;
	uint32_t word;
};

struct arm_mmu_l1_page_table {
	union arm_mmu_l1_page_table_entry entries[ARM_MMU_PT_L1_NUM_ENTRIES];
};

union arm_mmu_l2_page_table_entry {
	struct {
		uint32_t id			: 2;  /* [00] */
		uint32_t bufferable		: 1;
		uint32_t cacheable		: 1;
		uint32_t acc_perms10		: 2;
		uint32_t tex			: 3;
		uint32_t acc_perms2		: 1;
		uint32_t shared			: 1;
		uint32_t not_global		: 1;
		uint32_t pa_base		: 20; /* [31] */
	} l2_page_4k;
	struct {
		uint32_t id			: 2;  /* [00] */
		uint32_t bufferable		: 1;
		uint32_t cacheable		: 1;
		uint32_t acc_perms10		: 2;
		uint32_t zero			: 3;
		uint32_t acc_perms2		: 1;
		uint32_t shared			: 1;
		uint32_t not_global		: 1;
		uint32_t tex			: 3;
		uint32_t exec_never		: 1;
		uint32_t pa_base		: 16; /* [31] */
	} l2_page_64k;
	struct {
		uint32_t id			: 2;  /* [00] */
		uint32_t reserved		: 30; /* [31] */
	} undefined;
	uint32_t word;
};

struct arm_mmu_l2_page_table {
	union arm_mmu_l2_page_table_entry entries[ARM_MMU_PT_L2_NUM_ENTRIES];
};

/*
 * Data structure for L2 table usage tracking, contains a
 * L1 index reference if the respective L2 table is in use.
 */

struct arm_mmu_l2_page_table_status {
	uint32_t l1_index : 12;
	uint32_t entries  : 9;
	uint32_t reserved : 11;
};

/*
 * Data structure used to describe memory areas defined by the
 * current Zephyr image, for which an identity mapping (pa = va)
 * will be set up. Those memory areas are processed during the
 * MMU initialization.
 */
struct arm_mmu_flat_range {
	const char	*name;
	uint32_t	start;
	uint32_t	end;
	uint32_t	attrs;
};

/*
 * Data structure containing the memory attributes and permissions
 * data derived from a memory region's attr flags word in the format
 * required for setting up the corresponding PTEs.
 */
struct arm_mmu_perms_attrs {
	uint32_t acc_perms	: 2;
	uint32_t bufferable	: 1;
	uint32_t cacheable	: 1;
	uint32_t not_global	: 1;
	uint32_t non_sec	: 1;
	uint32_t shared		: 1;
	uint32_t tex		: 3;
	uint32_t exec_never	: 1;
	uint32_t id_mask	: 2;
	uint32_t domain		: 4;
	uint32_t reserved	: 15;
};

#endif /* ZEPHYR_ARCH_AARCH32_ARM_MMU_PRIV_H_ */
