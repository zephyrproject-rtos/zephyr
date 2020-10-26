/*
 * Copyright 2019 Broadcom
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Set below flag to get debug prints */
#define MMU_DEBUG_PRINTS	0

/* To get prints from MMU driver, it has to initialized after console driver */
#define MMU_DEBUG_PRIORITY	70

#if MMU_DEBUG_PRINTS
/* To dump page table entries while filling them, set DUMP_PTE macro */
#define DUMP_PTE		0
#define MMU_DEBUG(fmt, ...)	printk(fmt, ##__VA_ARGS__)
#else
#define MMU_DEBUG(...)
#endif

#if DUMP_PTE
#define L0_SPACE ""
#define L1_SPACE "  "
#define L2_SPACE "    "
#define L3_SPACE "      "
#define XLAT_TABLE_LEVEL_SPACE(level)		\
	(((level) == 0) ? L0_SPACE :		\
	((level) == 1) ? L1_SPACE :		\
	((level) == 2) ? L2_SPACE : L3_SPACE)
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

/* Maximum 4 XLAT table (L0 - L3) */
#define XLAT_LEVEL_MAX		4U

/* The VA shift of L3 depends on the granule size */
#define L3_XLAT_VA_SIZE_SHIFT	PAGE_SIZE_SHIFT

/* Number of VA bits to assign to each table (9 bits) */
#define Ln_XLAT_VA_SIZE_SHIFT	((VA_SIZE_SHIFT_MAX - L3_XLAT_VA_SIZE_SHIFT) / \
				 XLAT_LEVEL_MAX)

/* Starting bit in the VA address for each level */
#define L2_XLAT_VA_SIZE_SHIFT	(L3_XLAT_VA_SIZE_SHIFT + Ln_XLAT_VA_SIZE_SHIFT)
#define L1_XLAT_VA_SIZE_SHIFT	(L2_XLAT_VA_SIZE_SHIFT + Ln_XLAT_VA_SIZE_SHIFT)
#define L0_XLAT_VA_SIZE_SHIFT	(L1_XLAT_VA_SIZE_SHIFT + Ln_XLAT_VA_SIZE_SHIFT)

#define LEVEL_TO_VA_SIZE_SHIFT(level)			\
	(PAGE_SIZE_SHIFT + (Ln_XLAT_VA_SIZE_SHIFT *	\
	((XLAT_LEVEL_MAX - 1) - (level))))

/* Number of entries for each table (512) */
#define Ln_XLAT_NUM_ENTRIES	(1U << Ln_XLAT_VA_SIZE_SHIFT)

/* Virtual Address Index within a given translation table level */
#define XLAT_TABLE_VA_IDX(va_addr, level) \
	((va_addr >> LEVEL_TO_VA_SIZE_SHIFT(level)) & (Ln_XLAT_NUM_ENTRIES - 1))

/*
 * Calculate the initial translation table level from CONFIG_ARM64_VA_BITS
 * For a 4 KB page size:
 *
 * (va_bits <= 20)	 - base level 3
 * (21 <= va_bits <= 29) - base level 2
 * (30 <= va_bits <= 38) - base level 1
 * (39 <= va_bits <= 47) - base level 0
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
