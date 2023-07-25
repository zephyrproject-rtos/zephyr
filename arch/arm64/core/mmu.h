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
