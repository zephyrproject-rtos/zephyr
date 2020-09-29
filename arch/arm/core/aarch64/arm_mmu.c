/*
 * Copyright 2019 Broadcom
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <arch/arm/aarch64/cpu.h>
#include <arch/arm/aarch64/arm_mmu.h>
#include <linker/linker-defs.h>
#include <sys/util.h>

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

static uint64_t base_xlat_table[NUM_BASE_LEVEL_ENTRIES]
		__aligned(NUM_BASE_LEVEL_ENTRIES * sizeof(uint64_t));

static uint64_t xlat_tables[CONFIG_MAX_XLAT_TABLES][XLAT_TABLE_ENTRIES]
		__aligned(XLAT_TABLE_ENTRIES * sizeof(uint64_t));

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

/* Translation table control register settings */
static uint64_t get_tcr(int el)
{
	uint64_t tcr;
	uint64_t va_bits = CONFIG_ARM64_VA_BITS;
	uint64_t tcr_ps_bits;

	tcr_ps_bits = TCR_PS_BITS;

	if (el == 1) {
		tcr = (tcr_ps_bits << TCR_EL1_IPS_SHIFT);
		/*
		 * TCR_EL1.EPD1: Disable translation table walk for addresses
		 * that are translated using TTBR1_EL1.
		 */
		tcr |= TCR_EPD1_DISABLE;
	} else
		tcr = (tcr_ps_bits << TCR_EL3_PS_SHIFT);

	tcr |= TCR_T0SZ(va_bits);
	/*
	 * Translation table walk is cacheable, inner/outer WBWA and
	 * inner shareable
	 */
	tcr |= TCR_TG0_4K | TCR_SHARED_INNER | TCR_ORGN_WBWA | TCR_IRGN_WBWA;

	return tcr;
}

static int pte_desc_type(uint64_t *pte)
{
	return *pte & PTE_DESC_TYPE_MASK;
}

static uint64_t *calculate_pte_index(uint64_t addr, int level)
{
	int base_level = XLAT_TABLE_BASE_LEVEL;
	uint64_t *pte;
	uint64_t idx;
	unsigned int i;

	/* Walk through all translation tables to find pte index */
	pte = (uint64_t *)base_xlat_table;
	for (i = base_level; i <= XLAT_TABLE_LEVEL_MAX; i++) {
		idx = XLAT_TABLE_VA_IDX(addr, i);
		pte += idx;

		/* Found pte index */
		if (i == level)
			return pte;
		/* if PTE is not table desc, can't traverse */
		if (pte_desc_type(pte) != PTE_TABLE_DESC)
			return NULL;
		/* Move to the next translation table level */
		pte = (uint64_t *)(*pte & 0x0000fffffffff000ULL);
	}

	return NULL;
}

static void set_pte_table_desc(uint64_t *pte, uint64_t *table, unsigned int level)
{
#if DUMP_PTE
	MMU_DEBUG("%s", XLAT_TABLE_LEVEL_SPACE(level));
	MMU_DEBUG("%p: [Table] %p\n", pte, table);
#endif
	/* Point pte to new table */
	*pte = PTE_TABLE_DESC | (uint64_t)table;
}

static void set_pte_block_desc(uint64_t *pte, uint64_t addr_pa,
			       unsigned int attrs, unsigned int level)
{
	uint64_t desc = addr_pa;
	unsigned int mem_type;

	desc |= (level == 3) ? PTE_PAGE_DESC : PTE_BLOCK_DESC;

	/* NS bit for security memory access from secure state */
	desc |= (attrs & MT_NS) ? PTE_BLOCK_DESC_NS : 0;

	/* AP bits for Data access permission */
	desc |= (attrs & MT_RW) ? PTE_BLOCK_DESC_AP_RW : PTE_BLOCK_DESC_AP_RO;

	/* the access flag */
	desc |= PTE_BLOCK_DESC_AF;

	/* memory attribute index field */
	mem_type = MT_TYPE(attrs);
	desc |= PTE_BLOCK_DESC_MEMTYPE(mem_type);

	switch (mem_type) {
	case MT_DEVICE_nGnRnE:
	case MT_DEVICE_nGnRE:
	case MT_DEVICE_GRE:
		/* Access to Device memory and non-cacheable memory are coherent
		 * for all observers in the system and are treated as
		 * Outer shareable, so, for these 2 types of memory,
		 * it is not strictly needed to set shareability field
		 */
		desc |= PTE_BLOCK_DESC_OUTER_SHARE;
		/* Map device memory as execute-never */
		desc |= PTE_BLOCK_DESC_PXN;
		desc |= PTE_BLOCK_DESC_UXN;
		break;
	case MT_NORMAL_NC:
	case MT_NORMAL:
		/* Make Normal RW memory as execute never */
		if ((attrs & MT_RW) || (attrs & MT_EXECUTE_NEVER))
			desc |= PTE_BLOCK_DESC_PXN;
		if (mem_type == MT_NORMAL)
			desc |= PTE_BLOCK_DESC_INNER_SHARE;
		else
			desc |= PTE_BLOCK_DESC_OUTER_SHARE;
	}

#if DUMP_PTE
	MMU_DEBUG("%s", XLAT_TABLE_LEVEL_SPACE(level));
	MMU_DEBUG("%p: ", pte);
	MMU_DEBUG((mem_type == MT_NORMAL) ? "MEM" :
		  ((mem_type == MT_NORMAL_NC) ? "NC" : "DEV"));
	MMU_DEBUG((attrs & MT_RW) ? "-RW" : "-RO");
	MMU_DEBUG((attrs & MT_NS) ? "-NS" : "-S");
	MMU_DEBUG((attrs & MT_EXECUTE_NEVER) ? "-XN" : "-EXEC");
	MMU_DEBUG("\n");
#endif

	*pte = desc;
}

/* Returns a new reallocated table */
static uint64_t *new_prealloc_table(void)
{
	static unsigned int table_idx;

	__ASSERT(table_idx < CONFIG_MAX_XLAT_TABLES,
		"Enough xlat tables not allocated");

	return (uint64_t *)(xlat_tables[table_idx++]);
}

/* Splits a block into table with entries spanning the old block */
static void split_pte_block_desc(uint64_t *pte, int level)
{
	uint64_t old_block_desc = *pte;
	uint64_t *new_table;
	unsigned int i = 0;
	/* get address size shift bits for next level */
	int levelshift = LEVEL_TO_VA_SIZE_SHIFT(level + 1);

	MMU_DEBUG("Splitting existing PTE %p(L%d)\n", pte, level);

	new_table = new_prealloc_table();

	for (i = 0; i < XLAT_TABLE_ENTRIES; i++) {
		new_table[i] = old_block_desc | (i << levelshift);

		if ((level + 1) == 3)
			new_table[i] |= PTE_PAGE_DESC;
	}

	/* Overwrite existing PTE set the new table into effect */
	set_pte_table_desc(pte, new_table, level);
}

/* Create/Populate translation table(s) for given region */
static void init_xlat_tables(const struct arm_mmu_region *region)
{
	uint64_t *pte;
	uint64_t virt = region->base_va;
	uint64_t phys = region->base_pa;
	uint64_t size = region->size;
	uint64_t attrs = region->attrs;
	uint64_t level_size;
	uint64_t *new_table;
	unsigned int level = XLAT_TABLE_BASE_LEVEL;

	MMU_DEBUG("mmap: virt %llx phys %llx size %llx\n", virt, phys, size);
	/* check minimum alignment requirement for given mmap region */
	__ASSERT(((virt & (PAGE_SIZE - 1)) == 0) &&
		 ((size & (PAGE_SIZE - 1)) == 0),
		 "address/size are not page aligned\n");

	while (size) {
		__ASSERT(level <= XLAT_TABLE_LEVEL_MAX,
			 "max translation table level exceeded\n");

		/* Locate PTE for given virtual address and page table level */
		pte = calculate_pte_index(virt, level);
		__ASSERT(pte != NULL, "pte not found\n");

		level_size = 1ULL << LEVEL_TO_VA_SIZE_SHIFT(level);

		if (size >= level_size && !(virt & (level_size - 1))) {
			/* Given range fits into level size,
			 * create block/page descriptor
			 */
			set_pte_block_desc(pte, phys, attrs, level);
			virt += level_size;
			phys += level_size;
			size -= level_size;
			/* Range is mapped, start again for next range */
			level = XLAT_TABLE_BASE_LEVEL;
		} else if (pte_desc_type(pte) == PTE_INVALID_DESC) {
			/* Range doesn't fit, create subtable */
			new_table = new_prealloc_table();
			set_pte_table_desc(pte, new_table, level);
			level++;
		} else if (pte_desc_type(pte) == PTE_BLOCK_DESC) {
			split_pte_block_desc(pte, level);
			level++;
		} else if (pte_desc_type(pte) == PTE_TABLE_DESC)
			level++;
	}
}

/* zephyr execution regions with appropriate attributes */
static const struct arm_mmu_region mmu_zephyr_regions[] = {

	/* Mark text segment cacheable,read only and executable */
	MMU_REGION_FLAT_ENTRY("zephyr_code",
			      (uintptr_t)_image_text_start,
			      (uintptr_t)_image_text_size,
			      MT_CODE | MT_SECURE),

	/* Mark rodata segment cacheable, read only and execute-never */
	MMU_REGION_FLAT_ENTRY("zephyr_rodata",
			      (uintptr_t)_image_rodata_start,
			      (uintptr_t)_image_rodata_size,
			      MT_RODATA | MT_SECURE),

	/* Mark rest of the zephyr execution regions (data, bss, noinit, etc.)
	 * cacheable, read-write
	 * Note: read-write region is marked execute-ever internally
	 */
	MMU_REGION_FLAT_ENTRY("zephyr_data",
			      (uintptr_t)__kernel_ram_start,
			      (uintptr_t)__kernel_ram_size,
			      MT_NORMAL | MT_RW | MT_SECURE),
};

static void setup_page_tables(void)
{
	unsigned int index;
	const struct arm_mmu_region *region;
	uint64_t max_va = 0, max_pa = 0;

	for (index = 0; index < mmu_config.num_regions; index++) {
		region = &mmu_config.mmu_regions[index];
		max_va = MAX(max_va, region->base_va + region->size);
		max_pa = MAX(max_pa, region->base_pa + region->size);
	}

	__ASSERT(max_va <= (1ULL << CONFIG_ARM64_VA_BITS),
		 "Maximum VA not supported\n");
	__ASSERT(max_pa <= (1ULL << CONFIG_ARM64_PA_BITS),
		 "Maximum PA not supported\n");

	/* create translation tables for user provided platform regions */
	for (index = 0; index < mmu_config.num_regions; index++) {
		region = &mmu_config.mmu_regions[index];
		if (region->size || region->attrs)
			init_xlat_tables(region);
	}

	/* setup translation table for zephyr execution regions */
	for (index = 0; index < ARRAY_SIZE(mmu_zephyr_regions); index++) {
		region = &mmu_zephyr_regions[index];
		if (region->size || region->attrs)
			init_xlat_tables(region);
	}
}

static void enable_mmu_el1(unsigned int flags)
{
	ARG_UNUSED(flags);
	uint64_t val;

	/* Set MAIR, TCR and TBBR registers */
	__asm__ volatile("msr mair_el1, %0"
			:
			: "r" (MEMORY_ATTRIBUTES)
			: "memory", "cc");
	__asm__ volatile("msr tcr_el1, %0"
			:
			: "r" (get_tcr(1))
			: "memory", "cc");
	__asm__ volatile("msr ttbr0_el1, %0"
			:
			: "r" ((uint64_t)base_xlat_table)
			: "memory", "cc");

	/* Ensure these changes are seen before MMU is enabled */
	__ISB();

	/* Enable the MMU and data cache */
	__asm__ volatile("mrs %0, sctlr_el1" : "=r" (val));
	__asm__ volatile("msr sctlr_el1, %0"
			:
			: "r" (val | SCTLR_M_BIT | SCTLR_C_BIT)
			: "memory", "cc");

	/* Ensure the MMU enable takes effect immediately */
	__ISB();

	MMU_DEBUG("MMU enabled with dcache\n");
}

/* ARM MMU Driver Initial Setup */

/*
 * @brief MMU default configuration
 *
 * This function provides the default configuration mechanism for the Memory
 * Management Unit (MMU).
 */
static int arm_mmu_init(const struct device *arg)
{
	uint64_t val;
	unsigned int idx, flags = 0;

	/* Current MMU code supports only EL1 */
	__asm__ volatile("mrs %0, CurrentEL" : "=r" (val));

	__ASSERT(GET_EL(val) == MODE_EL1,
		 "Exception level not EL1, MMU not enabled!\n");

	/* Ensure that MMU is already not enabled */
	__asm__ volatile("mrs %0, sctlr_el1" : "=r" (val));
	__ASSERT((val & SCTLR_M_BIT) == 0, "MMU is already enabled\n");

	MMU_DEBUG("xlat tables:\n");
	MMU_DEBUG("base table(L%d): %p, %d entries\n", XLAT_TABLE_BASE_LEVEL,
			(uint64_t *)base_xlat_table, NUM_BASE_LEVEL_ENTRIES);
	for (idx = 0; idx < CONFIG_MAX_XLAT_TABLES; idx++)
		MMU_DEBUG("%d: %p\n", idx, (uint64_t *)(xlat_tables + idx));

	setup_page_tables();

	/* currently only EL1 is supported */
	enable_mmu_el1(flags);

	return 0;
}

SYS_INIT(arm_mmu_init, PRE_KERNEL_1,
#if MMU_DEBUG_PRINTS
	 MMU_DEBUG_PRIORITY
#else
	 CONFIG_KERNEL_INIT_PRIORITY_DEVICE
#endif
);
