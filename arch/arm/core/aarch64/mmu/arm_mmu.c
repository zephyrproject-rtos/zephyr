/*
 * Copyright 2019 Broadcom
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <kernel_arch_interface.h>
#include <logging/log.h>
#include <arch/arm/aarch64/cpu.h>
#include <arch/arm/aarch64/lib_helpers.h>
#include <arch/arm/aarch64/arm_mmu.h>
#include <linker/linker-defs.h>
#include <sys/util.h>

#include "arm_mmu.h"

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

static uint64_t xlat_tables[CONFIG_MAX_XLAT_TABLES * Ln_XLAT_NUM_ENTRIES]
		__aligned(Ln_XLAT_NUM_ENTRIES * sizeof(uint64_t));
static uint16_t xlat_use_count[CONFIG_MAX_XLAT_TABLES];

/* Returns a reference to a free table */
static uint64_t *new_table(void)
{
	unsigned int i;

	/* Look for a free table. */
	for (i = 0; i < CONFIG_MAX_XLAT_TABLES; i++) {
		if (xlat_use_count[i] == 0) {
			xlat_use_count[i] = 1;
			return &xlat_tables[i * Ln_XLAT_NUM_ENTRIES];
		}
	}

	LOG_ERR("CONFIG_MAX_XLAT_TABLES, too small");
	return NULL;
}

static inline unsigned int table_index(uint64_t *pte)
{
	unsigned int i = (pte - xlat_tables) / Ln_XLAT_NUM_ENTRIES;

	__ASSERT(i < CONFIG_MAX_XLAT_TABLES, "table out of range");
	return i;
}

/* Makes a table free for reuse. */
static void free_table(uint64_t *table)
{
	unsigned int i = table_index(table);

	__ASSERT(xlat_use_count[i] == 1, "table still in use");
	xlat_use_count[i] = 0;
}

/* Adjusts usage count and returns current count. */
static int table_usage(uint64_t *table, int adjustment)
{
	unsigned int i = table_index(table);

	xlat_use_count[i] += adjustment;
	__ASSERT(xlat_use_count[i] > 0, "usage count underflow");
	return xlat_use_count[i];
}

static inline bool is_table_unused(uint64_t *table)
{
	return table_usage(table, 0) == 1;
}

static inline bool is_free_desc(uint64_t desc)
{
	return (desc & PTE_DESC_TYPE_MASK) == PTE_INVALID_DESC;
}

static inline bool is_table_desc(uint64_t desc, unsigned int level)
{
	return level != XLAT_LAST_LEVEL &&
	       (desc & PTE_DESC_TYPE_MASK) == PTE_TABLE_DESC;
}

static inline bool is_block_desc(uint64_t desc)
{
	return (desc & PTE_DESC_TYPE_MASK) == PTE_BLOCK_DESC;
}

static inline uint64_t *pte_desc_table(uint64_t desc)
{
	uint64_t address = desc & GENMASK(47, PAGE_SIZE_SHIFT);

	return (uint64_t *)address;
}

static inline bool is_desc_superset(uint64_t desc1, uint64_t desc2,
				    unsigned int level)
{
	uint64_t mask = DESC_ATTRS_MASK | GENMASK(47, LEVEL_TO_VA_SIZE_SHIFT(level));

	return (desc1 & mask) == (desc2 & mask);
}

#if DUMP_PTE
static void debug_show_pte(uint64_t *pte, unsigned int level)
{
	MMU_DEBUG("%.*s", level * 2, ". . . ");
	MMU_DEBUG("[%d]%p: ", table_index(pte), pte);

	if (is_free_desc(*pte)) {
		MMU_DEBUG("---\n");
		return;
	}

	if (is_table_desc(*pte, level)) {
		uint64_t *table = pte_desc_table(*pte);

		MMU_DEBUG("[Table] [%d]%p\n", table_index(table), table);
		return;
	}

	if (is_block_desc(*pte)) {
		MMU_DEBUG("[Block] ");
	} else {
		MMU_DEBUG("[Page] ");
	}

	uint8_t mem_type = (*pte >> 2) & MT_TYPE_MASK;

	MMU_DEBUG((mem_type == MT_NORMAL) ? "MEM" :
		  ((mem_type == MT_NORMAL_NC) ? "NC" : "DEV"));
	MMU_DEBUG((*pte & PTE_BLOCK_DESC_AP_RO) ? "-RO" : "-RW");
	MMU_DEBUG((*pte & PTE_BLOCK_DESC_NS) ? "-NS" : "-S");
	MMU_DEBUG((*pte & PTE_BLOCK_DESC_AP_ELx) ? "-ELx" : "-ELh");
	MMU_DEBUG((*pte & PTE_BLOCK_DESC_PXN) ? "-PXN" : "-PX");
	MMU_DEBUG((*pte & PTE_BLOCK_DESC_UXN) ? "-UXN" : "-UX");
	MMU_DEBUG("\n");
}
#else
static inline void debug_show_pte(uint64_t *pte, unsigned int level) { }
#endif

static void set_pte_table_desc(uint64_t *pte, uint64_t *table, unsigned int level)
{
	/* Point pte to new table */
	*pte = PTE_TABLE_DESC | (uint64_t)table;
	debug_show_pte(pte, level);
}

static void set_pte_block_desc(uint64_t *pte, uint64_t desc, unsigned int level)
{
	if (desc) {
		desc |= (level == XLAT_LAST_LEVEL) ? PTE_PAGE_DESC : PTE_BLOCK_DESC;
	}
	*pte = desc;
	debug_show_pte(pte, level);
}

static void populate_table(uint64_t *table, uint64_t desc, unsigned int level)
{
	unsigned int stride_shift = LEVEL_TO_VA_SIZE_SHIFT(level);
	unsigned int i;

	MMU_DEBUG("Populating table with PTE 0x%016llx(L%d)\n", desc, level);

	if (level == XLAT_LAST_LEVEL) {
		desc |= PTE_PAGE_DESC;
	}

	for (i = 0; i < Ln_XLAT_NUM_ENTRIES; i++) {
		table[i] = desc | (i << stride_shift);
	}
}

static int set_mapping(struct arm_mmu_ptables *ptables,
		       uintptr_t virt, size_t size,
		       uint64_t desc, bool may_overwrite)
{
	uint64_t *pte, *ptes[XLAT_LAST_LEVEL + 1];
	uint64_t level_size;
	uint64_t *table = ptables->base_xlat_table;
	unsigned int level = BASE_XLAT_LEVEL;

	while (size) {
		__ASSERT(level <= XLAT_LAST_LEVEL,
			 "max translation table level exceeded\n");

		/* Locate PTE for given virtual address and page table level */
		pte = &table[XLAT_TABLE_VA_IDX(virt, level)];
		ptes[level] = pte;

		if (is_table_desc(*pte, level)) {
			/* Move to the next translation table level */
			level++;
			table = pte_desc_table(*pte);
			continue;
		}

		if (!may_overwrite && !is_free_desc(*pte)) {
			/* the entry is already allocated */
			LOG_ERR("entry already in use: "
				"level %d pte %p *pte 0x%016llx",
				level, pte, *pte);
			return -EBUSY;
		}

		level_size = 1ULL << LEVEL_TO_VA_SIZE_SHIFT(level);

		if (is_desc_superset(*pte, desc, level)) {
			/* This block already covers our range */
			level_size -= (virt & (level_size - 1));
			if (level_size > size) {
				level_size = size;
			}
			goto move_on;
		}

		if ((size < level_size) || (virt & (level_size - 1))) {
			/* Range doesn't fit, create subtable */
			table = new_table();
			if (!table) {
				return -ENOMEM;
			}
			/*
			 * If entry at current level was already populated
			 * then we need to reflect that in the new table.
			 */
			if (is_block_desc(*pte)) {
				table_usage(table, Ln_XLAT_NUM_ENTRIES);
				populate_table(table, *pte, level + 1);
			}
			/* Adjust usage count for parent table */
			if (is_free_desc(*pte)) {
				table_usage(pte, 1);
			}
			/* And link it. */
			set_pte_table_desc(pte, table, level);
			level++;
			continue;
		}

		/* Adjust usage count for corresponding table */
		if (is_free_desc(*pte)) {
			table_usage(pte, 1);
		}
		if (!desc) {
			table_usage(pte, -1);
		}
		/* Create (or erase) block/page descriptor */
		set_pte_block_desc(pte, desc, level);

		/* recursively free unused tables if any */
		while (level != BASE_XLAT_LEVEL &&
		       is_table_unused(pte)) {
			free_table(pte);
			pte = ptes[--level];
			set_pte_block_desc(pte, 0, level);
			table_usage(pte, -1);
		}

move_on:
		virt += level_size;
		desc += desc ? level_size : 0;
		size -= level_size;

		/* Range is mapped, start again for next range */
		table = ptables->base_xlat_table;
		level = BASE_XLAT_LEVEL;
	}

	return 0;
}

static uint64_t get_region_desc(uint32_t attrs)
{
	unsigned int mem_type;
	uint64_t desc = 0;

	/* NS bit for security memory access from secure state */
	desc |= (attrs & MT_NS) ? PTE_BLOCK_DESC_NS : 0;

	/*
	 * AP bits for EL0 / ELh Data access permission
	 *
	 *   AP[2:1]   ELh  EL0
	 * +--------------------+
	 *     00      RW   NA
	 *     01      RW   RW
	 *     10      RO   NA
	 *     11      RO   RO
	 */

	/* AP bits for Data access permission */
	desc |= (attrs & MT_RW) ? PTE_BLOCK_DESC_AP_RW : PTE_BLOCK_DESC_AP_RO;

	/* Mirror permissions to EL0 */
	desc |= (attrs & MT_RW_AP_ELx) ?
		 PTE_BLOCK_DESC_AP_ELx : PTE_BLOCK_DESC_AP_EL_HIGHER;

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
		if ((attrs & MT_RW) || (attrs & MT_P_EXECUTE_NEVER))
			desc |= PTE_BLOCK_DESC_PXN;

		if (((attrs & MT_RW) && (attrs & MT_RW_AP_ELx)) ||
		     (attrs & MT_U_EXECUTE_NEVER))
			desc |= PTE_BLOCK_DESC_UXN;

		if (mem_type == MT_NORMAL)
			desc |= PTE_BLOCK_DESC_INNER_SHARE;
		else
			desc |= PTE_BLOCK_DESC_OUTER_SHARE;
	}

	return desc;
}

static int add_map(struct arm_mmu_ptables *ptables, const char *name,
		   uintptr_t phys, uintptr_t virt, size_t size, uint32_t attrs)
{
	uint64_t desc = get_region_desc(attrs);
	bool may_overwrite = !(attrs & MT_NO_OVERWRITE);

	MMU_DEBUG("mmap [%s]: virt %lx phys %lx size %lx attr %llx\n",
		  name, virt, phys, size, desc);
	__ASSERT(((virt | phys | size) & (CONFIG_MMU_PAGE_SIZE - 1)) == 0,
		 "address/size are not page aligned\n");
	desc |= phys;
	return set_mapping(ptables, virt, size, desc, may_overwrite);
}

static int remove_map(struct arm_mmu_ptables *ptables, const char *name,
		      uintptr_t virt, size_t size)
{
	MMU_DEBUG("unmmap [%s]: virt %lx size %lx\n", name, virt, size);
	__ASSERT(((virt | size) & (CONFIG_MMU_PAGE_SIZE - 1)) == 0,
		 "address/size are not page aligned\n");
	return set_mapping(ptables, virt, size, 0, true);
}

/* zephyr execution regions with appropriate attributes */

struct arm_mmu_flat_range {
	char *name;
	void *start;
	void *end;
	uint32_t attrs;
};

static const struct arm_mmu_flat_range mmu_zephyr_ranges[] = {

	/* Mark the zephyr execution regions (data, bss, noinit, etc.)
	 * cacheable, read-write
	 * Note: read-write region is marked execute-never internally
	 */
	{ .name  = "zephyr_data",
	  .start = _image_ram_start,
	  .end   = _image_ram_end,
	  .attrs = MT_NORMAL | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE },

	/* Mark text segment cacheable,read only and executable */
	{ .name  = "zephyr_code",
	  .start = _image_text_start,
	  .end   = _image_text_end,
	  .attrs = MT_NORMAL | MT_P_RX_U_NA | MT_DEFAULT_SECURE_STATE },

	/* Mark rodata segment cacheable, read only and execute-never */
	{ .name  = "zephyr_rodata",
	  .start = _image_rodata_start,
	  .end   = _image_rodata_end,
	  .attrs = MT_NORMAL | MT_P_RO_U_NA | MT_DEFAULT_SECURE_STATE },
};

static inline void add_arm_mmu_flat_range(struct arm_mmu_ptables *ptables,
					  const struct arm_mmu_flat_range *range,
					  uint32_t extra_flags)
{
	uintptr_t address = (uintptr_t)range->start;
	size_t size = (uintptr_t)range->end - address;

	if (size) {
		add_map(ptables, range->name, address, address,
			size, range->attrs | extra_flags);
	}
}

static inline void add_arm_mmu_region(struct arm_mmu_ptables *ptables,
				      const struct arm_mmu_region *region,
				      uint32_t extra_flags)
{
	if (region->size || region->attrs) {
		add_map(ptables, region->name, region->base_pa, region->base_va,
			region->size, region->attrs | extra_flags);
	}
}

static void setup_page_tables(struct arm_mmu_ptables *ptables)
{
	unsigned int index;
	const struct arm_mmu_flat_range *range;
	const struct arm_mmu_region *region;
	uintptr_t max_va = 0, max_pa = 0;

	MMU_DEBUG("xlat tables:\n");
	for (index = 0; index < CONFIG_MAX_XLAT_TABLES; index++)
		MMU_DEBUG("%d: %p\n", index, xlat_tables + index * Ln_XLAT_NUM_ENTRIES);

	for (index = 0; index < mmu_config.num_regions; index++) {
		region = &mmu_config.mmu_regions[index];
		max_va = MAX(max_va, region->base_va + region->size);
		max_pa = MAX(max_pa, region->base_pa + region->size);
	}

	__ASSERT(max_va <= (1ULL << CONFIG_ARM64_VA_BITS),
		 "Maximum VA not supported\n");
	__ASSERT(max_pa <= (1ULL << CONFIG_ARM64_PA_BITS),
		 "Maximum PA not supported\n");

	/* setup translation table for zephyr execution regions */
	for (index = 0; index < ARRAY_SIZE(mmu_zephyr_ranges); index++) {
		range = &mmu_zephyr_ranges[index];
		add_arm_mmu_flat_range(ptables, range, 0);
	}

	/*
	 * Create translation tables for user provided platform regions.
	 * Those must not conflict with our default mapping.
	 */
	for (index = 0; index < mmu_config.num_regions; index++) {
		region = &mmu_config.mmu_regions[index];
		add_arm_mmu_region(ptables, region, MT_NO_OVERWRITE);
	}
}

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

static void enable_mmu_el1(struct arm_mmu_ptables *ptables, unsigned int flags)
{
	ARG_UNUSED(flags);
	uint64_t val;

	/* Set MAIR, TCR and TBBR registers */
	write_mair_el1(MEMORY_ATTRIBUTES);
	write_tcr_el1(get_tcr(1));
	write_ttbr0_el1((uint64_t)ptables->base_xlat_table);

	/* Ensure these changes are seen before MMU is enabled */
	isb();

	/* Enable the MMU and data cache */
	val = read_sctlr_el1();
	write_sctlr_el1(val | SCTLR_M_BIT | SCTLR_C_BIT);

	/* Ensure the MMU enable takes effect immediately */
	isb();

	MMU_DEBUG("MMU enabled with dcache\n");
}

/* ARM MMU Driver Initial Setup */

static struct arm_mmu_ptables kernel_ptables;

/*
 * @brief MMU default configuration
 *
 * This function provides the default configuration mechanism for the Memory
 * Management Unit (MMU).
 */
void z_arm64_mmu_init(void)
{
	unsigned int flags = 0;

	__ASSERT(CONFIG_MMU_PAGE_SIZE == KB(4),
		 "Only 4K page size is supported\n");

	__ASSERT(GET_EL(read_currentel()) == MODE_EL1,
		 "Exception level not EL1, MMU not enabled!\n");

	/* Ensure that MMU is already not enabled */
	__ASSERT((read_sctlr_el1() & SCTLR_M_BIT) == 0, "MMU is already enabled\n");

	kernel_ptables.base_xlat_table = new_table();
	setup_page_tables(&kernel_ptables);

	/* currently only EL1 is supported */
	enable_mmu_el1(&kernel_ptables, flags);
}

static int __arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	struct arm_mmu_ptables *ptables;
	uint32_t entry_flags = MT_SECURE | MT_P_RX_U_NA;

	/* Always map in the kernel page tables */
	ptables = &kernel_ptables;

	/* Translate flags argument into HW-recognized entry flags. */
	switch (flags & K_MEM_CACHE_MASK) {
	/*
	 * K_MEM_CACHE_NONE => MT_DEVICE_nGnRnE
	 *			(Device memory nGnRnE)
	 * K_MEM_CACHE_WB   => MT_NORMAL
	 *			(Normal memory Outer WB + Inner WB)
	 * K_MEM_CACHE_WT   => MT_NORMAL_WT
	 *			(Normal memory Outer WT + Inner WT)
	 */
	case K_MEM_CACHE_NONE:
		entry_flags |= MT_DEVICE_nGnRnE;
		break;
	case K_MEM_CACHE_WT:
		entry_flags |= MT_NORMAL_WT;
		break;
	case K_MEM_CACHE_WB:
		entry_flags |= MT_NORMAL;
		break;
	default:
		return -ENOTSUP;
	}

	if ((flags & K_MEM_PERM_RW) != 0U) {
		entry_flags |= MT_RW;
	}

	if ((flags & K_MEM_PERM_EXEC) == 0U) {
		entry_flags |= MT_P_EXECUTE_NEVER;
	}

	if ((flags & K_MEM_PERM_USER) != 0U) {
		return -ENOTSUP;
	}

	return add_map(ptables, "generic", phys, (uintptr_t)virt, size, entry_flags);
}

void arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	int ret = __arch_mem_map(virt, phys, size, flags);

	if (ret) {
		LOG_ERR("__arch_mem_map() returned %d", ret);
		k_panic();
	}
}

void arch_mem_unmap(void *addr, size_t size)
{
	int ret = remove_map(&kernel_ptables, "generic", (uintptr_t)addr, size);

	if (ret) {
		LOG_ERR("remove_map() returned %d", ret);
	}
}
