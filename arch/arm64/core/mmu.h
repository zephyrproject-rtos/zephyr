/*
 * Copyright 2019 Broadcom
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Set below flag to get debug prints */
#define MMU_DEBUG_PRINTS	0

#if MMU_DEBUG_PRINTS
/* To dump page table entries while filling them, set DUMP_PTE macro */
#define DUMP_PTE		0
#define MMU_DEBUG(fmt, ...)	printk(fmt, ##__VA_ARGS__)
#else
#define MMU_DEBUG(...)
#endif

/*
 * 48-bit address with 4KB granule size:
 *
 * +------------+------------+------------+------------+-----------+
 * | VA [47:39] | VA [38:30] | VA [29:21] | VA [20:12] | VA [11:0] |
 * +---------------------------------------------------------------+
 * |     L0     |     L1     |     L2     |     L3     | block off |
 * +------------+------------+------------+------------+-----------+
 */

/* Only 4K granule is supported */
#define PAGE_SIZE_SHIFT		12U

/* 48-bit VA address */
#define VA_SIZE_SHIFT_MAX	48U

/* Maximum 4 XLAT table levels (L0 - L3) */
#define XLAT_LAST_LEVEL		3U

/* The VA shift of L3 depends on the granule size */
#define L3_XLAT_VA_SIZE_SHIFT	PAGE_SIZE_SHIFT

/* Number of VA bits to assign to each table (9 bits) */
#define Ln_XLAT_VA_SIZE_SHIFT	(PAGE_SIZE_SHIFT - 3)

/* Starting bit in the VA address for each level */
#define L2_XLAT_VA_SIZE_SHIFT	(L3_XLAT_VA_SIZE_SHIFT + Ln_XLAT_VA_SIZE_SHIFT)
#define L1_XLAT_VA_SIZE_SHIFT	(L2_XLAT_VA_SIZE_SHIFT + Ln_XLAT_VA_SIZE_SHIFT)
#define L0_XLAT_VA_SIZE_SHIFT	(L1_XLAT_VA_SIZE_SHIFT + Ln_XLAT_VA_SIZE_SHIFT)

#define LEVEL_TO_VA_SIZE_SHIFT(level)			\
	(PAGE_SIZE_SHIFT + (Ln_XLAT_VA_SIZE_SHIFT *	\
	(XLAT_LAST_LEVEL - (level))))

/* Number of entries for each table (512) */
#define Ln_XLAT_NUM_ENTRIES	((1U << PAGE_SIZE_SHIFT) / 8U)

/* Virtual Address Index within a given translation table level */
#define XLAT_TABLE_VA_IDX(va_addr, level) \
	((va_addr >> LEVEL_TO_VA_SIZE_SHIFT(level)) & (Ln_XLAT_NUM_ENTRIES - 1))

/*
 * Calculate the initial translation table level from CONFIG_ARM64_VA_BITS
 * For a 4 KB page size:
 *
 * (va_bits <= 21)	 - base level 3
 * (22 <= va_bits <= 30) - base level 2
 * (31 <= va_bits <= 39) - base level 1
 * (40 <= va_bits <= 48) - base level 0
 */
#define GET_BASE_XLAT_LEVEL(va_bits)				\
	 ((va_bits > L0_XLAT_VA_SIZE_SHIFT) ? 0U		\
	: (va_bits > L1_XLAT_VA_SIZE_SHIFT) ? 1U		\
	: (va_bits > L2_XLAT_VA_SIZE_SHIFT) ? 2U : 3U)

/* Level for the base XLAT */
#define BASE_XLAT_LEVEL	GET_BASE_XLAT_LEVEL(CONFIG_ARM64_VA_BITS)

#if (CONFIG_ARM64_PA_BITS == 48)
#define TCR_PS_BITS TCR_PS_BITS_256TB
#elif (CONFIG_ARM64_PA_BITS == 44)
#define TCR_PS_BITS TCR_PS_BITS_16TB
#elif (CONFIG_ARM64_PA_BITS == 42)
#define TCR_PS_BITS TCR_PS_BITS_4TB
#elif (CONFIG_ARM64_PA_BITS == 40)
#define TCR_PS_BITS TCR_PS_BITS_1TB
#elif (CONFIG_ARM64_PA_BITS == 36)
#define TCR_PS_BITS TCR_PS_BITS_64GB
#else
#define TCR_PS_BITS TCR_PS_BITS_4GB
#endif

/* Upper and lower attributes mask for page/block descriptor */
#define DESC_ATTRS_UPPER_MASK	GENMASK(63, 51)
#define DESC_ATTRS_LOWER_MASK	GENMASK(11, 2)

#define DESC_ATTRS_MASK		(DESC_ATTRS_UPPER_MASK | DESC_ATTRS_LOWER_MASK)

/*
 * PTE descriptor can be Block descriptor or Table descriptor
 * or Page descriptor.
 */
#define PTE_DESC_TYPE_MASK	3ULL
#define PTE_BLOCK_DESC		1ULL
#define PTE_TABLE_DESC		3ULL
#define PTE_PAGE_DESC		3ULL
#define PTE_INVALID_DESC	0ULL

/*
 * Block and Page descriptor attributes fields
 */
#define PTE_BLOCK_DESC_MEMTYPE(x)	(x << 2)
#define PTE_BLOCK_DESC_NS		(1ULL << 5)
#define PTE_BLOCK_DESC_AP_ELx		(1ULL << 6)
#define PTE_BLOCK_DESC_AP_EL_HIGHER	(0ULL << 6)
#define PTE_BLOCK_DESC_AP_RO		(1ULL << 7)
#define PTE_BLOCK_DESC_AP_RW		(0ULL << 7)
#define PTE_BLOCK_DESC_NON_SHARE	(0ULL << 8)
#define PTE_BLOCK_DESC_OUTER_SHARE	(2ULL << 8)
#define PTE_BLOCK_DESC_INNER_SHARE	(3ULL << 8)
#define PTE_BLOCK_DESC_AF		(1ULL << 10)
#define PTE_BLOCK_DESC_NG		(1ULL << 11)
#define PTE_BLOCK_DESC_PXN		(1ULL << 53)
#define PTE_BLOCK_DESC_UXN		(1ULL << 54)

/*
 * Descriptor physical address field bits
 */
#define PTE_PHYSADDR_MASK		GENMASK64(47, PAGE_SIZE_SHIFT)

/*
 * Descriptor bits 58 to 55 are defined as "Reserved for Software Use".
 *
 * When using demand paging, RW memory is marked RO to trap the first write
 * for dirty page tracking. Bit 55 indicates if memory is actually writable.
 */
#define PTE_SW_WRITABLE			(1ULL << 55)

/*
 * TCR definitions.
 */
#define TCR_EL1_IPS_SHIFT	32U
#define TCR_EL2_PS_SHIFT	16U
#define TCR_EL3_PS_SHIFT	16U

#define TCR_T0SZ_SHIFT		0U
#define TCR_T0SZ(x)		((64 - (x)) << TCR_T0SZ_SHIFT)

#define TCR_IRGN_NC		(0ULL << 8)
#define TCR_IRGN_WBWA		(1ULL << 8)
#define TCR_IRGN_WT		(2ULL << 8)
#define TCR_IRGN_WBNWA		(3ULL << 8)
#define TCR_IRGN_MASK		(3ULL << 8)
#define TCR_ORGN_NC		(0ULL << 10)
#define TCR_ORGN_WBWA		(1ULL << 10)
#define TCR_ORGN_WT		(2ULL << 10)
#define TCR_ORGN_WBNWA		(3ULL << 10)
#define TCR_ORGN_MASK		(3ULL << 10)
#define TCR_SHARED_NON		(0ULL << 12)
#define TCR_SHARED_OUTER	(2ULL << 12)
#define TCR_SHARED_INNER	(3ULL << 12)
#define TCR_TG0_4K		(0ULL << 14)
#define TCR_TG0_64K		(1ULL << 14)
#define TCR_TG0_16K		(2ULL << 14)
#define TCR_EPD1_DISABLE	(1ULL << 23)
#define TCR_TG1_16K		(1ULL << 30)
#define TCR_TG1_4K		(2ULL << 30)
#define TCR_TG1_64K		(3ULL << 30)

#define TCR_PS_BITS_4GB		0x0ULL
#define TCR_PS_BITS_64GB	0x1ULL
#define TCR_PS_BITS_1TB		0x2ULL
#define TCR_PS_BITS_4TB		0x3ULL
#define TCR_PS_BITS_16TB	0x4ULL
#define TCR_PS_BITS_256TB	0x5ULL

/*
 * ARM guarantees at least 8 ASID bits.
 * We may have more available, but do not make use of them for the time being.
 */
#define VM_ASID_BITS 8
#define TTBR_ASID_SHIFT 48
