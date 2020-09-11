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

/* We support only 4kB translation granule */
#define PAGE_SIZE_SHIFT		12U
#define PAGE_SIZE		(1U << PAGE_SIZE_SHIFT)
#define XLAT_TABLE_SIZE_SHIFT   PAGE_SIZE_SHIFT /* Size of one complete table */
#define XLAT_TABLE_SIZE		(1U << XLAT_TABLE_SIZE_SHIFT)

#define XLAT_TABLE_ENTRY_SIZE_SHIFT	3U /* Each table entry is 8 bytes */
#define XLAT_TABLE_LEVEL_MAX	3U

#define XLAT_TABLE_ENTRIES_SHIFT \
			(XLAT_TABLE_SIZE_SHIFT - XLAT_TABLE_ENTRY_SIZE_SHIFT)
#define XLAT_TABLE_ENTRIES	(1U << XLAT_TABLE_ENTRIES_SHIFT)

/* Address size covered by each entry at given translation table level */
#define L3_XLAT_VA_SIZE_SHIFT	PAGE_SIZE_SHIFT
#define L2_XLAT_VA_SIZE_SHIFT	\
			(L3_XLAT_VA_SIZE_SHIFT + XLAT_TABLE_ENTRIES_SHIFT)
#define L1_XLAT_VA_SIZE_SHIFT	\
			(L2_XLAT_VA_SIZE_SHIFT + XLAT_TABLE_ENTRIES_SHIFT)
#define L0_XLAT_VA_SIZE_SHIFT	\
			(L1_XLAT_VA_SIZE_SHIFT + XLAT_TABLE_ENTRIES_SHIFT)

#define LEVEL_TO_VA_SIZE_SHIFT(level) \
				(PAGE_SIZE_SHIFT + (XLAT_TABLE_ENTRIES_SHIFT * \
				(XLAT_TABLE_LEVEL_MAX - (level))))

/* Virtual Address Index within given translation table level */
#define XLAT_TABLE_VA_IDX(va_addr, level) \
	((va_addr >> LEVEL_TO_VA_SIZE_SHIFT(level)) & (XLAT_TABLE_ENTRIES - 1))

/*
 * Calculate the initial translation table level from CONFIG_ARM64_VA_BITS
 * For a 4 KB page size,
 * (va_bits <= 21)	 - base level 3
 * (22 <= va_bits <= 30) - base level 2
 * (31 <= va_bits <= 39) - base level 1
 * (40 <= va_bits <= 48) - base level 0
 */
#define GET_XLAT_TABLE_BASE_LEVEL(va_bits)	\
	((va_bits > L0_XLAT_VA_SIZE_SHIFT)	\
	? 0U					\
	: (va_bits > L1_XLAT_VA_SIZE_SHIFT)	\
	? 1U					\
	: (va_bits > L2_XLAT_VA_SIZE_SHIFT)	\
	? 2U : 3U)

#define XLAT_TABLE_BASE_LEVEL	GET_XLAT_TABLE_BASE_LEVEL(CONFIG_ARM64_VA_BITS)

#define GET_NUM_BASE_LEVEL_ENTRIES(va_bits)	\
	(1U << (va_bits - LEVEL_TO_VA_SIZE_SHIFT(XLAT_TABLE_BASE_LEVEL)))

#define NUM_BASE_LEVEL_ENTRIES	GET_NUM_BASE_LEVEL_ENTRIES(CONFIG_ARM64_VA_BITS)

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
