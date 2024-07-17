/*
 * Copyright 2019 Broadcom
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * Copyright (c) 2021 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm/demand_paging.h>
#include <kernel_arch_func.h>
#include <kernel_arch_interface.h>
#include <kernel_internal.h>
#include <zephyr/logging/log.h>
#include <zephyr/arch/arm64/cpu.h>
#include <zephyr/arch/arm64/lib_helpers.h>
#include <zephyr/arch/arm64/mm.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>
#include <mmu.h>

#include "mmu.h"
#include "paging.h"

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

static uint64_t xlat_tables[CONFIG_MAX_XLAT_TABLES * Ln_XLAT_NUM_ENTRIES]
		__aligned(Ln_XLAT_NUM_ENTRIES * sizeof(uint64_t));
static int xlat_use_count[CONFIG_MAX_XLAT_TABLES];
static struct k_spinlock xlat_lock;

/* Usage count value range */
#define XLAT_PTE_COUNT_MASK	GENMASK(15, 0)
#define XLAT_REF_COUNT_UNIT	BIT(16)

/* Returns a reference to a free table */
static uint64_t *new_table(void)
{
	uint64_t *table;
	unsigned int i;

	/* Look for a free table. */
	for (i = 0U; i < CONFIG_MAX_XLAT_TABLES; i++) {
		if (xlat_use_count[i] == 0) {
			table = &xlat_tables[i * Ln_XLAT_NUM_ENTRIES];
			xlat_use_count[i] = XLAT_REF_COUNT_UNIT;
			MMU_DEBUG("allocating table [%d]%p\n", i, table);
			return table;
		}
	}

	LOG_ERR("CONFIG_MAX_XLAT_TABLES, too small");
	return NULL;
}

static inline unsigned int table_index(uint64_t *pte)
{
	unsigned int i = (pte - xlat_tables) / Ln_XLAT_NUM_ENTRIES;

	__ASSERT(i < CONFIG_MAX_XLAT_TABLES, "table %p out of range", pte);
	return i;
}

/* Adjusts usage count and returns current count. */
static int table_usage(uint64_t *table, int adjustment)
{
	unsigned int i = table_index(table);
	int prev_count = xlat_use_count[i];
	int new_count = prev_count + adjustment;

	/* be reasonable not to always create a debug flood */
	if ((IS_ENABLED(DUMP_PTE) && adjustment != 0) || new_count == 0) {
		MMU_DEBUG("table [%d]%p: usage %#x -> %#x\n", i, table, prev_count, new_count);
	}

	__ASSERT(new_count >= 0,
		 "table use count underflow");
	__ASSERT(new_count == 0 || new_count >= XLAT_REF_COUNT_UNIT,
		 "table in use with no reference to it");
	__ASSERT((new_count & XLAT_PTE_COUNT_MASK) <= Ln_XLAT_NUM_ENTRIES,
		 "table PTE count overflow");

	xlat_use_count[i] = new_count;
	return new_count;
}

static inline void inc_table_ref(uint64_t *table)
{
	table_usage(table, XLAT_REF_COUNT_UNIT);
}

static inline void dec_table_ref(uint64_t *table)
{
	int ref_unit = XLAT_REF_COUNT_UNIT;

	table_usage(table, -ref_unit);
}

static inline bool is_table_unused(uint64_t *table)
{
	return (table_usage(table, 0) & XLAT_PTE_COUNT_MASK) == 0;
}

static inline bool is_table_single_referenced(uint64_t *table)
{
	return table_usage(table, 0) < (2 * XLAT_REF_COUNT_UNIT);
}

#ifdef CONFIG_TEST
/* Hooks to let test code peek at table states */

int arm64_mmu_nb_free_tables(void)
{
	int count = 0;

	for (int i = 0; i < CONFIG_MAX_XLAT_TABLES; i++) {
		if (xlat_use_count[i] == 0) {
			count++;
		}
	}

	return count;
}

int arm64_mmu_tables_total_usage(void)
{
	int count = 0;

	for (int i = 0; i < CONFIG_MAX_XLAT_TABLES; i++) {
		count += xlat_use_count[i];
	}

	return count;
}

#endif /* CONFIG_TEST */

static inline bool is_free_desc(uint64_t desc)
{
	return desc == 0;
}

static inline bool is_inval_desc(uint64_t desc)
{
	/* invalid descriptors aren't necessarily free */
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
	uint64_t address = desc & PTE_PHYSADDR_MASK;

	/* tables use a 1:1 physical:virtual mapping */
	return (uint64_t *)address;
}

static inline bool is_desc_block_aligned(uint64_t desc, unsigned int level_size)
{
	bool aligned = (desc & PTE_PHYSADDR_MASK & (level_size - 1)) == 0;

	if (!aligned) {
		MMU_DEBUG("misaligned desc 0x%016llx for block size 0x%x\n",
			  desc, level_size);
	}

	return aligned;
}

static inline bool is_desc_superset(uint64_t desc1, uint64_t desc2,
				    unsigned int level)
{
	uint64_t mask = DESC_ATTRS_MASK | GENMASK64(47, LEVEL_TO_VA_SIZE_SHIFT(level));

	return (desc1 & mask) == (desc2 & mask);
}

#if DUMP_PTE
static void debug_show_pte(uint64_t *pte, unsigned int level)
{
	MMU_DEBUG("%.*s", level * 2U, ". . . ");
	MMU_DEBUG("[%d]%p: ", table_index(pte), pte);

	if (is_free_desc(*pte)) {
		MMU_DEBUG("---\n");
		return;
	}

	MMU_DEBUG("0x%016llx ", *pte);

	if (is_table_desc(*pte, level)) {
		uint64_t *table = pte_desc_table(*pte);

		MMU_DEBUG("[Table] [%d]%p\n", table_index(table), table);
		return;
	}

	if (is_block_desc(*pte)) {
		MMU_DEBUG("[Block] ");
	} else if (!is_inval_desc(*pte)) {
		MMU_DEBUG("[Page] ");
	} else {
		MMU_DEBUG("[paged-out] ");
	}

	uint8_t mem_type = (*pte >> 2) & MT_TYPE_MASK;

	MMU_DEBUG((mem_type == MT_NORMAL) ? "MEM" :
		  ((mem_type == MT_NORMAL_NC) ? "NC" : "DEV"));
	MMU_DEBUG((*pte & PTE_BLOCK_DESC_AP_RO) ? "-RO" : "-RW");
	MMU_DEBUG((*pte & PTE_BLOCK_DESC_NS) ? "-NS" : "-S");
	MMU_DEBUG((*pte & PTE_BLOCK_DESC_AP_ELx) ? "-ELx" : "-ELh");
	MMU_DEBUG((*pte & PTE_BLOCK_DESC_PXN) ? "-PXN" : "-PX");
	MMU_DEBUG((*pte & PTE_BLOCK_DESC_UXN) ? "-UXN" : "-UX");
	MMU_DEBUG((*pte & PTE_SW_WRITABLE) ? "-WRITABLE" : "");
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
	if (level != XLAT_LAST_LEVEL) {
		desc |= PTE_BLOCK_DESC;
	} else if (!IS_ENABLED(CONFIG_DEMAND_PAGING) || (desc & PTE_BLOCK_DESC_AF) != 0) {
		desc |= PTE_PAGE_DESC;
	} else {
		/*
		 * Demand paging configured and AF unset: leave the descriptor
		 * type to "invalid" as in arch_mem_page_out().
		 */
	}
	*pte = desc;
	debug_show_pte(pte, level);
}

static uint64_t *expand_to_table(uint64_t *pte, unsigned int level)
{
	uint64_t *table;

	__ASSERT(level < XLAT_LAST_LEVEL, "can't expand last level");

	table = new_table();
	if (!table) {
		return NULL;
	}

	if (!is_free_desc(*pte)) {
		/*
		 * If entry at current level was already populated
		 * then we need to reflect that in the new table.
		 */
		uint64_t desc = *pte;
		unsigned int i, stride_shift;

		MMU_DEBUG("expanding PTE 0x%016llx into table [%d]%p\n",
			  desc, table_index(table), table);
		__ASSERT(is_block_desc(desc), "");

		if (level + 1 == XLAT_LAST_LEVEL) {
			desc |= PTE_PAGE_DESC;
		}

		stride_shift = LEVEL_TO_VA_SIZE_SHIFT(level + 1);
		for (i = 0U; i < Ln_XLAT_NUM_ENTRIES; i++) {
			table[i] = desc | (i << stride_shift);
		}
		table_usage(table, Ln_XLAT_NUM_ENTRIES);
	} else {
		/*
		 * Adjust usage count for parent table's entry
		 * that will no longer be free.
		 */
		table_usage(pte, 1);
	}

	/* Link the new table in place of the pte it replaces */
	set_pte_table_desc(pte, table, level);

	return table;
}

static int set_mapping(uint64_t *top_table, uintptr_t virt, size_t size,
		       uint64_t desc, bool may_overwrite)
{
	uint64_t *table = top_table;
	uint64_t *pte;
	uint64_t level_size;
	unsigned int level = BASE_XLAT_LEVEL;

	while (size) {
		__ASSERT(level <= XLAT_LAST_LEVEL,
			 "max translation table level exceeded\n");

		/* Locate PTE for given virtual address and page table level */
		pte = &table[XLAT_TABLE_VA_IDX(virt, level)];

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

		if ((size < level_size) || (virt & (level_size - 1)) ||
		    !is_desc_block_aligned(desc, level_size)) {
			/* Range doesn't fit, create subtable */
			table = expand_to_table(pte, level);
			if (!table) {
				return -ENOMEM;
			}
			level++;
			continue;
		}

		/* Adjust usage count for corresponding table */
		if (is_free_desc(*pte)) {
			table_usage(pte, 1);
		}
		/* Create block/page descriptor */
		set_pte_block_desc(pte, desc, level);

move_on:
		virt += level_size;
		desc += level_size;
		size -= level_size;

		/* Range is mapped, start again for next range */
		table = top_table;
		level = BASE_XLAT_LEVEL;
	}

	return 0;
}

static void del_mapping(uint64_t *table, uintptr_t virt, size_t size,
			unsigned int level)
{
	size_t step, level_size = 1ULL << LEVEL_TO_VA_SIZE_SHIFT(level);
	uint64_t *pte, *subtable;

	for ( ; size; virt += step, size -= step) {
		step = level_size - (virt & (level_size - 1));
		if (step > size) {
			step = size;
		}
		pte = &table[XLAT_TABLE_VA_IDX(virt, level)];

		if (is_free_desc(*pte)) {
			continue;
		}

		if (step != level_size && is_block_desc(*pte)) {
			/* need to split this block mapping */
			expand_to_table(pte, level);
		}

		if (is_table_desc(*pte, level)) {
			subtable = pte_desc_table(*pte);
			del_mapping(subtable, virt, step, level + 1);
			if (!is_table_unused(subtable)) {
				continue;
			}
			dec_table_ref(subtable);
		}

		/* free this entry */
		*pte = 0;
		table_usage(pte, -1);
	}
}

#ifdef CONFIG_USERSPACE

static uint64_t *dup_table(uint64_t *src_table, unsigned int level)
{
	uint64_t *dst_table = new_table();
	int i, usage_count = 0;

	if (!dst_table) {
		return NULL;
	}

	MMU_DEBUG("dup (level %d) [%d]%p to [%d]%p\n", level,
		  table_index(src_table), src_table,
		  table_index(dst_table), dst_table);

	for (i = 0; i < Ln_XLAT_NUM_ENTRIES; i++) {
		/*
		 * After the table duplication, each table can be independently
		 *  updated. Thus, entries may become non-global.
		 * To keep the invariants very simple, we thus force the non-global
		 *  bit on duplication. Moreover, there is no process to revert this
		 *  (e.g. in `globalize_table`). Could be improved in future work.
		 */
		if (!is_free_desc(src_table[i]) && !is_table_desc(src_table[i], level)) {
			src_table[i] |= PTE_BLOCK_DESC_NG;
		}

		dst_table[i] = src_table[i];
		if (is_table_desc(dst_table[i], level)) {
			inc_table_ref(pte_desc_table(dst_table[i]));
		}
		if (!is_free_desc(dst_table[i])) {
			usage_count++;
		}
	}
	table_usage(dst_table, usage_count);

	return dst_table;
}

static int privatize_table(uint64_t *dst_table, uint64_t *src_table,
			   uintptr_t virt, size_t size, unsigned int level)
{
	size_t step, level_size = 1ULL << LEVEL_TO_VA_SIZE_SHIFT(level);
	unsigned int i;
	int ret;

	for ( ; size; virt += step, size -= step) {
		step = level_size - (virt & (level_size - 1));
		if (step > size) {
			step = size;
		}
		i = XLAT_TABLE_VA_IDX(virt, level);

		if (!is_table_desc(dst_table[i], level) ||
		    !is_table_desc(src_table[i], level)) {
			/* this entry is already private */
			continue;
		}

		uint64_t *dst_subtable = pte_desc_table(dst_table[i]);
		uint64_t *src_subtable = pte_desc_table(src_table[i]);

		if (dst_subtable == src_subtable) {
			/* need to make a private copy of this table */
			dst_subtable = dup_table(src_subtable, level + 1);
			if (!dst_subtable) {
				return -ENOMEM;
			}
			set_pte_table_desc(&dst_table[i], dst_subtable, level);
			dec_table_ref(src_subtable);
		}

		ret = privatize_table(dst_subtable, src_subtable,
				      virt, step, level + 1);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

/*
 * Make the given virtual address range private in dst_pt with regards to
 * src_pt. By "private" this means that corresponding page tables in dst_pt
 * will be duplicated so not to share the same table(s) with src_pt.
 * If corresponding page tables in dst_pt are already distinct from src_pt
 * then nothing is done. This allows for subsequent mapping changes in that
 * range to affect only dst_pt.
 */
static int privatize_page_range(struct arm_mmu_ptables *dst_pt,
				struct arm_mmu_ptables *src_pt,
				uintptr_t virt_start, size_t size,
				const char *name)
{
	k_spinlock_key_t key;
	int ret;

	MMU_DEBUG("privatize [%s]: virt %lx size %lx\n",
		  name, virt_start, size);

	key = k_spin_lock(&xlat_lock);

	ret = privatize_table(dst_pt->base_xlat_table, src_pt->base_xlat_table,
			      virt_start, size, BASE_XLAT_LEVEL);

	k_spin_unlock(&xlat_lock, key);
	return ret;
}

static void discard_table(uint64_t *table, unsigned int level)
{
	unsigned int i;
	int free_count = 0;

	for (i = 0U; i < Ln_XLAT_NUM_ENTRIES; i++) {
		if (is_table_desc(table[i], level)) {
			uint64_t *subtable = pte_desc_table(table[i]);

			if (is_table_single_referenced(subtable)) {
				discard_table(subtable, level + 1);
			}
			dec_table_ref(subtable);
		}
		if (!is_free_desc(table[i])) {
			table[i] = 0U;
			free_count++;
		}
	}
	table_usage(table, -free_count);
}

static int globalize_table(uint64_t *dst_table, uint64_t *src_table,
			   uintptr_t virt, size_t size, unsigned int level)
{
	size_t step, level_size = 1ULL << LEVEL_TO_VA_SIZE_SHIFT(level);
	unsigned int i;
	int ret;

	for ( ; size; virt += step, size -= step) {
		step = level_size - (virt & (level_size - 1));
		if (step > size) {
			step = size;
		}
		i = XLAT_TABLE_VA_IDX(virt, level);

		if (dst_table[i] == src_table[i]) {
			/* already identical to global table */
			continue;
		}

		if (is_free_desc(src_table[i]) &&
		    is_table_desc(dst_table[i], level)) {
			uint64_t *subtable = pte_desc_table(dst_table[i]);

			del_mapping(subtable, virt, step, level + 1);
			if (is_table_unused(subtable)) {
				/* unreference the empty table */
				dst_table[i] = 0;
				table_usage(dst_table, -1);
				dec_table_ref(subtable);
			}
			continue;
		}

		if (step != level_size) {
			/* boundary falls in the middle of this pte */
			__ASSERT(is_table_desc(src_table[i], level),
				 "can't have partial block pte here");
			if (!is_table_desc(dst_table[i], level)) {
				/* we need more fine grained boundaries */
				if (!expand_to_table(&dst_table[i], level)) {
					return -ENOMEM;
				}
			}
			ret = globalize_table(pte_desc_table(dst_table[i]),
					      pte_desc_table(src_table[i]),
					      virt, step, level + 1);
			if (ret) {
				return ret;
			}
			continue;
		}

		/* we discard current pte and replace with global one */

		uint64_t *old_table = is_table_desc(dst_table[i], level) ?
					pte_desc_table(dst_table[i]) : NULL;

		if (is_free_desc(dst_table[i])) {
			table_usage(dst_table, 1);
		}
		if (is_free_desc(src_table[i])) {
			table_usage(dst_table, -1);
		}
		if (is_table_desc(src_table[i], level)) {
			inc_table_ref(pte_desc_table(src_table[i]));
		}
		dst_table[i] = src_table[i];
		debug_show_pte(&dst_table[i], level);

		if (old_table) {
			/* we can discard the whole branch */
			discard_table(old_table, level + 1);
			dec_table_ref(old_table);
		}
	}

	return 0;
}

/*
 * Globalize the given virtual address range in dst_pt from src_pt. We make
 * it global by sharing as much page table content from src_pt as possible,
 * including page tables themselves, and corresponding private tables in
 * dst_pt are then discarded. If page tables in the given range are already
 * shared then nothing is done. If page table sharing is not possible then
 * page table entries in dst_pt are synchronized with those from src_pt.
 */
static int globalize_page_range(struct arm_mmu_ptables *dst_pt,
				struct arm_mmu_ptables *src_pt,
				uintptr_t virt_start, size_t size,
				const char *name)
{
	k_spinlock_key_t key;
	int ret;

	MMU_DEBUG("globalize [%s]: virt %lx size %lx\n",
		  name, virt_start, size);

	key = k_spin_lock(&xlat_lock);

	ret = globalize_table(dst_pt->base_xlat_table, src_pt->base_xlat_table,
			      virt_start, size, BASE_XLAT_LEVEL);

	k_spin_unlock(&xlat_lock, key);
	return ret;
}

#endif /* CONFIG_USERSPACE */

static uint64_t get_region_desc(uint32_t attrs)
{
	unsigned int mem_type;
	uint64_t desc = 0U;

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
	desc |= (IS_ENABLED(CONFIG_DEMAND_PAGING) && (attrs & MT_RW)) ?
		PTE_SW_WRITABLE : 0;

	/* Mirror permissions to EL0 */
	desc |= (attrs & MT_RW_AP_ELx) ?
		 PTE_BLOCK_DESC_AP_ELx : PTE_BLOCK_DESC_AP_EL_HIGHER;

	/* the access flag */
	desc |= PTE_BLOCK_DESC_AF;
	if (IS_ENABLED(CONFIG_DEMAND_PAGING) && (attrs & MT_PAGED_OUT) != 0) {
		/* set it up for demand paging like arch_mem_page_out() */
		desc &= ~PTE_BLOCK_DESC_AF;
		desc |= PTE_BLOCK_DESC_AP_RO;
	}

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
		if ((attrs & MT_RW) || (attrs & MT_P_EXECUTE_NEVER)) {
			desc |= PTE_BLOCK_DESC_PXN;
		}

		if (((attrs & MT_RW) && (attrs & MT_RW_AP_ELx)) ||
		     (attrs & MT_U_EXECUTE_NEVER)) {
			desc |= PTE_BLOCK_DESC_UXN;
		}

		if (mem_type == MT_NORMAL) {
			desc |= PTE_BLOCK_DESC_INNER_SHARE;
		} else {
			desc |= PTE_BLOCK_DESC_OUTER_SHARE;
		}
	}

	/* non-Global bit */
	if (attrs & MT_NG) {
		desc |= PTE_BLOCK_DESC_NG;
	}

	return desc;
}

static int __add_map(struct arm_mmu_ptables *ptables, const char *name,
		     uintptr_t phys, uintptr_t virt, size_t size, uint32_t attrs)
{
	uint64_t desc = get_region_desc(attrs);
	bool may_overwrite = !(attrs & MT_NO_OVERWRITE);

	MMU_DEBUG("mmap [%s]: virt %lx phys %lx size %lx attr %llx %s overwrite\n",
		  name, virt, phys, size, desc,
		  may_overwrite ? "may" : "no");
	__ASSERT(((virt | phys | size) & (CONFIG_MMU_PAGE_SIZE - 1)) == 0,
		 "address/size are not page aligned\n");
	desc |= phys;
	return set_mapping(ptables->base_xlat_table, virt, size, desc, may_overwrite);
}

static int add_map(struct arm_mmu_ptables *ptables, const char *name,
		   uintptr_t phys, uintptr_t virt, size_t size, uint32_t attrs)
{
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&xlat_lock);
	ret = __add_map(ptables, name, phys, virt, size, attrs);
	k_spin_unlock(&xlat_lock, key);
	return ret;
}

static void remove_map(struct arm_mmu_ptables *ptables, const char *name,
		       uintptr_t virt, size_t size)
{
	k_spinlock_key_t key;

	MMU_DEBUG("unmmap [%s]: virt %lx size %lx\n", name, virt, size);
	__ASSERT(((virt | size) & (CONFIG_MMU_PAGE_SIZE - 1)) == 0,
		 "address/size are not page aligned\n");

	key = k_spin_lock(&xlat_lock);
	del_mapping(ptables->base_xlat_table, virt, size, BASE_XLAT_LEVEL);
	k_spin_unlock(&xlat_lock, key);
}

static void invalidate_tlb_all(void)
{
	__asm__ volatile (
	"dsb ishst; tlbi vmalle1; dsb ish; isb"
	: : : "memory");
}

static inline void invalidate_tlb_page(uintptr_t virt)
{
	/* to be refined */
	invalidate_tlb_all();
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
	  .start = __text_region_start,
	  .end   = __text_region_end,
	  .attrs = MT_NORMAL | MT_P_RX_U_RX | MT_DEFAULT_SECURE_STATE },

	/* Mark rodata segment cacheable, read only and execute-never */
	{ .name  = "zephyr_rodata",
	  .start = __rodata_region_start,
	  .end   = __rodata_region_end,
	  .attrs = MT_NORMAL | MT_P_RO_U_RO | MT_DEFAULT_SECURE_STATE },

#ifdef CONFIG_NOCACHE_MEMORY
	/* Mark nocache segment noncachable, read-write and execute-never */
	{ .name  = "nocache_data",
	  .start = _nocache_ram_start,
	  .end   = _nocache_ram_end,
	  .attrs = MT_NORMAL_NC | MT_P_RW_U_RW | MT_DEFAULT_SECURE_STATE },
#endif
};

static inline void add_arm_mmu_flat_range(struct arm_mmu_ptables *ptables,
					  const struct arm_mmu_flat_range *range,
					  uint32_t extra_flags)
{
	uintptr_t address = (uintptr_t)range->start;
	size_t size = (uintptr_t)range->end - address;

	if (size) {
		/* MMU not yet active: must use unlocked version */
		__add_map(ptables, range->name, address, address,
			  size, range->attrs | extra_flags);
	}
}

static inline void add_arm_mmu_region(struct arm_mmu_ptables *ptables,
				      const struct arm_mmu_region *region,
				      uint32_t extra_flags)
{
	if (region->size || region->attrs) {
		/* MMU not yet active: must use unlocked version */
		__add_map(ptables, region->name, region->base_pa, region->base_va,
			  region->size, region->attrs | extra_flags);
	}
}

static inline void inv_dcache_after_map_helper(void *virt, size_t size, uint32_t attrs)
{
	/*
	 * DC IVAC instruction requires write access permission to the VA,
	 * otherwise it can generate a permission fault
	 */
	if ((attrs & MT_RW) != MT_RW) {
		return;
	}

	if (MT_TYPE(attrs) == MT_NORMAL || MT_TYPE(attrs) == MT_NORMAL_WT) {
		sys_cache_data_invd_range(virt, size);
	}
}

static void setup_page_tables(struct arm_mmu_ptables *ptables)
{
	unsigned int index;
	const struct arm_mmu_flat_range *range;
	const struct arm_mmu_region *region;
	uintptr_t max_va = 0, max_pa = 0;

	MMU_DEBUG("xlat tables:\n");
	for (index = 0U; index < CONFIG_MAX_XLAT_TABLES; index++) {
		MMU_DEBUG("%d: %p\n", index, xlat_tables + index * Ln_XLAT_NUM_ENTRIES);
	}

	for (index = 0U; index < mmu_config.num_regions; index++) {
		region = &mmu_config.mmu_regions[index];
		max_va = MAX(max_va, region->base_va + region->size);
		max_pa = MAX(max_pa, region->base_pa + region->size);
	}

	__ASSERT(max_va <= (1ULL << CONFIG_ARM64_VA_BITS),
		 "Maximum VA not supported\n");
	__ASSERT(max_pa <= (1ULL << CONFIG_ARM64_PA_BITS),
		 "Maximum PA not supported\n");

	/* setup translation table for zephyr execution regions */
	for (index = 0U; index < ARRAY_SIZE(mmu_zephyr_ranges); index++) {
		range = &mmu_zephyr_ranges[index];
		add_arm_mmu_flat_range(ptables, range, 0);
	}

	/*
	 * Create translation tables for user provided platform regions.
	 * Those must not conflict with our default mapping.
	 */
	for (index = 0U; index < mmu_config.num_regions; index++) {
		region = &mmu_config.mmu_regions[index];
		add_arm_mmu_region(ptables, region, MT_NO_OVERWRITE);
	}

	invalidate_tlb_all();

	for (index = 0U; index < ARRAY_SIZE(mmu_zephyr_ranges); index++) {
		size_t size;

		range = &mmu_zephyr_ranges[index];
		size = POINTER_TO_UINT(range->end) - POINTER_TO_UINT(range->start);
		inv_dcache_after_map_helper(range->start, size, range->attrs);
	}

	for (index = 0U; index < mmu_config.num_regions; index++) {
		region = &mmu_config.mmu_regions[index];
		inv_dcache_after_map_helper(UINT_TO_POINTER(region->base_va), region->size,
					    region->attrs);
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
	} else {
		tcr = (tcr_ps_bits << TCR_EL3_PS_SHIFT);
	}

	tcr |= TCR_T0SZ(va_bits);

	/*
	 * Translation table walk is cacheable, inner/outer WBWA and
	 * inner shareable.  Due to Cortex-A57 erratum #822227 we must
	 * set TG1[1] = 4KB.
	 */
	tcr |= TCR_TG1_4K | TCR_TG0_4K | TCR_SHARED_INNER |
	       TCR_ORGN_WBWA | TCR_IRGN_WBWA;

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
	barrier_isync_fence_full();

	/* Enable the MMU and data cache */
	val = read_sctlr_el1();
	write_sctlr_el1(val | SCTLR_M_BIT | SCTLR_C_BIT);

	/* Ensure the MMU enable takes effect immediately */
	barrier_isync_fence_full();

	MMU_DEBUG("MMU enabled with dcache\n");
}

/* ARM MMU Driver Initial Setup */

static struct arm_mmu_ptables kernel_ptables;
#ifdef CONFIG_USERSPACE
static sys_slist_t domain_list;
#endif

/*
 * @brief MMU default configuration
 *
 * This function provides the default configuration mechanism for the Memory
 * Management Unit (MMU).
 */
void z_arm64_mm_init(bool is_primary_core)
{
	unsigned int flags = 0U;

	__ASSERT(CONFIG_MMU_PAGE_SIZE == KB(4),
		 "Only 4K page size is supported\n");

	__ASSERT(GET_EL(read_currentel()) == MODE_EL1,
		 "Exception level not EL1, MMU not enabled!\n");

	/* Ensure that MMU is already not enabled */
	__ASSERT((read_sctlr_el1() & SCTLR_M_BIT) == 0, "MMU is already enabled\n");

	/*
	 * Only booting core setup up the page tables.
	 */
	if (is_primary_core) {
		kernel_ptables.base_xlat_table = new_table();
		setup_page_tables(&kernel_ptables);
	}

	/* currently only EL1 is supported */
	enable_mmu_el1(&kernel_ptables, flags);
}

static void sync_domains(uintptr_t virt, size_t size, const char *name)
{
#ifdef CONFIG_USERSPACE
	sys_snode_t *node;
	struct arch_mem_domain *domain;
	struct arm_mmu_ptables *domain_ptables;
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&z_mem_domain_lock);
	SYS_SLIST_FOR_EACH_NODE(&domain_list, node) {
		domain = CONTAINER_OF(node, struct arch_mem_domain, node);
		domain_ptables = &domain->ptables;
		ret = globalize_page_range(domain_ptables, &kernel_ptables,
					   virt, size, name);
		if (ret) {
			LOG_ERR("globalize_page_range() returned %d", ret);
		}
	}
	k_spin_unlock(&z_mem_domain_lock, key);
#endif
}

static int __arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	struct arm_mmu_ptables *ptables;
	uint32_t entry_flags = MT_DEFAULT_SECURE_STATE | MT_P_RX_U_NA | MT_NO_OVERWRITE;

	/* Always map in the kernel page tables */
	ptables = &kernel_ptables;

	/* Translate flags argument into HW-recognized entry flags. */
	switch (flags & K_MEM_CACHE_MASK) {
	/*
	 * K_MEM_CACHE_NONE, K_MEM_ARM_DEVICE_nGnRnE => MT_DEVICE_nGnRnE
	 *			(Device memory nGnRnE)
	 * K_MEM_ARM_DEVICE_nGnRE => MT_DEVICE_nGnRE
	 *			(Device memory nGnRE)
	 * K_MEM_ARM_DEVICE_GRE => MT_DEVICE_GRE
	 *			(Device memory GRE)
	 * K_MEM_ARM_NORMAL_NC   => MT_NORMAL_NC
	 *			(Normal memory Non-cacheable)
	 * K_MEM_CACHE_WB   => MT_NORMAL
	 *			(Normal memory Outer WB + Inner WB)
	 * K_MEM_CACHE_WT   => MT_NORMAL_WT
	 *			(Normal memory Outer WT + Inner WT)
	 */
	case K_MEM_CACHE_NONE:
	/* K_MEM_CACHE_NONE equal to K_MEM_ARM_DEVICE_nGnRnE */
	/* case K_MEM_ARM_DEVICE_nGnRnE: */
		entry_flags |= MT_DEVICE_nGnRnE;
		break;
	case K_MEM_ARM_DEVICE_nGnRE:
		entry_flags |= MT_DEVICE_nGnRE;
		break;
	case K_MEM_ARM_DEVICE_GRE:
		entry_flags |= MT_DEVICE_GRE;
		break;
	case K_MEM_ARM_NORMAL_NC:
		entry_flags |= MT_NORMAL_NC;
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
		entry_flags |= MT_RW_AP_ELx;
	}

	if (IS_ENABLED(CONFIG_DEMAND_PAGING) && (flags & K_MEM_MAP_UNPAGED) != 0) {
		entry_flags |= MT_PAGED_OUT;
	}

	return add_map(ptables, "generic", phys, (uintptr_t)virt, size, entry_flags);
}

void arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	int ret = __arch_mem_map(virt, phys, size, flags);

	if (ret) {
		LOG_ERR("__arch_mem_map() returned %d", ret);
		k_panic();
	} else {
		uint32_t mem_flags = flags & K_MEM_CACHE_MASK;

		sync_domains((uintptr_t)virt, size, "mem_map");
		invalidate_tlb_all();

		switch (mem_flags) {
		case K_MEM_CACHE_WB:
		case K_MEM_CACHE_WT:
			mem_flags = (mem_flags == K_MEM_CACHE_WB) ? MT_NORMAL : MT_NORMAL_WT;
			mem_flags |= (flags & K_MEM_PERM_RW) ? MT_RW : 0;
			inv_dcache_after_map_helper(virt, size, mem_flags);
		default:
			break;
		}
	}
}

void arch_mem_unmap(void *addr, size_t size)
{
	remove_map(&kernel_ptables, "generic", (uintptr_t)addr, size);
	sync_domains((uintptr_t)addr, size, "mem_unmap");
	invalidate_tlb_all();
}

int arch_page_phys_get(void *virt, uintptr_t *phys)
{
	uint64_t par;
	int key;

	key = arch_irq_lock();
	__asm__ volatile ("at S1E1R, %0" : : "r" (virt));
	barrier_isync_fence_full();
	par = read_par_el1();
	arch_irq_unlock(key);

	if (par & BIT(0)) {
		return -EFAULT;
	}

	if (phys) {
		*phys = par & GENMASK64(47, 12);
	}
	return 0;
}

size_t arch_virt_region_align(uintptr_t phys, size_t size)
{
	size_t alignment = CONFIG_MMU_PAGE_SIZE;
	size_t level_size;
	int level;

	for (level = XLAT_LAST_LEVEL; level >= BASE_XLAT_LEVEL; level--) {
		level_size = 1 << LEVEL_TO_VA_SIZE_SHIFT(level);

		if (size < level_size) {
			break;
		}

		if ((phys & (level_size - 1))) {
			break;
		}

		alignment = level_size;
	}

	return alignment;
}

#ifdef CONFIG_USERSPACE

static uint16_t next_asid = 1;

static uint16_t get_asid(uint64_t ttbr0)
{
	return ttbr0 >> TTBR_ASID_SHIFT;
}

static void z_arm64_swap_ptables(struct k_thread *incoming);

int arch_mem_domain_max_partitions_get(void)
{
	return CONFIG_MAX_DOMAIN_PARTITIONS;
}

int arch_mem_domain_init(struct k_mem_domain *domain)
{
	struct arm_mmu_ptables *domain_ptables = &domain->arch.ptables;
	k_spinlock_key_t key;
	uint16_t asid;

	MMU_DEBUG("%s\n", __func__);

	key = k_spin_lock(&xlat_lock);

	/*
	 * Pick a new ASID. We use round-robin
	 * Note: `next_asid` is an uint16_t and `VM_ASID_BITS` could
	 *  be up to 16, hence `next_asid` might overflow to 0 below.
	 */
	asid = next_asid++;
	if ((next_asid >= (1UL << VM_ASID_BITS)) || (next_asid == 0)) {
		next_asid = 1;
	}

	domain_ptables->base_xlat_table =
		dup_table(kernel_ptables.base_xlat_table, BASE_XLAT_LEVEL);
	k_spin_unlock(&xlat_lock, key);
	if (!domain_ptables->base_xlat_table) {
		return -ENOMEM;
	}

	domain_ptables->ttbr0 =	(((uint64_t)asid) << TTBR_ASID_SHIFT) |
		((uint64_t)(uintptr_t)domain_ptables->base_xlat_table);

	sys_slist_append(&domain_list, &domain->arch.node);
	return 0;
}

static int private_map(struct arm_mmu_ptables *ptables, const char *name,
		       uintptr_t phys, uintptr_t virt, size_t size, uint32_t attrs)
{
	int ret;

	ret = privatize_page_range(ptables, &kernel_ptables, virt, size, name);
	__ASSERT(ret == 0, "privatize_page_range() returned %d", ret);
	ret = add_map(ptables, name, phys, virt, size, attrs | MT_NG);
	__ASSERT(ret == 0, "add_map() returned %d", ret);
	invalidate_tlb_all();

	inv_dcache_after_map_helper(UINT_TO_POINTER(virt), size, attrs);
	return ret;
}

static int reset_map(struct arm_mmu_ptables *ptables, const char *name,
		     uintptr_t addr, size_t size)
{
	int ret;

	ret = globalize_page_range(ptables, &kernel_ptables, addr, size, name);
	__ASSERT(ret == 0, "globalize_page_range() returned %d", ret);
	invalidate_tlb_all();

	return ret;
}

int arch_mem_domain_partition_add(struct k_mem_domain *domain,
				  uint32_t partition_id)
{
	struct arm_mmu_ptables *domain_ptables = &domain->arch.ptables;
	struct k_mem_partition *ptn = &domain->partitions[partition_id];

	return private_map(domain_ptables, "partition", ptn->start, ptn->start,
			   ptn->size, ptn->attr.attrs | MT_NORMAL);
}

int arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				     uint32_t partition_id)
{
	struct arm_mmu_ptables *domain_ptables = &domain->arch.ptables;
	struct k_mem_partition *ptn = &domain->partitions[partition_id];

	return reset_map(domain_ptables, "partition removal",
			 ptn->start, ptn->size);
}

static int map_thread_stack(struct k_thread *thread,
			    struct arm_mmu_ptables *ptables)
{
	return private_map(ptables, "thread_stack", thread->stack_info.start,
			    thread->stack_info.start, thread->stack_info.size,
			    MT_P_RW_U_RW | MT_NORMAL);
}

int arch_mem_domain_thread_add(struct k_thread *thread)
{
	struct arm_mmu_ptables *old_ptables, *domain_ptables;
	struct k_mem_domain *domain;
	bool is_user, is_migration;
	int ret = 0;

	domain = thread->mem_domain_info.mem_domain;
	domain_ptables = &domain->arch.ptables;
	old_ptables = thread->arch.ptables;

	is_user = (thread->base.user_options & K_USER) != 0;
	is_migration = (old_ptables != NULL) && is_user;

	if (is_migration) {
		ret = map_thread_stack(thread, domain_ptables);
	}

	thread->arch.ptables = domain_ptables;
	if (thread == _current) {
		z_arm64_swap_ptables(thread);
	} else {
#ifdef CONFIG_SMP
		/* the thread could be running on another CPU right now */
		z_arm64_mem_cfg_ipi();
#endif
	}

	if (is_migration) {
		ret = reset_map(old_ptables, __func__, thread->stack_info.start,
				thread->stack_info.size);
	}

	return ret;
}

int arch_mem_domain_thread_remove(struct k_thread *thread)
{
	struct arm_mmu_ptables *domain_ptables;
	struct k_mem_domain *domain;

	domain = thread->mem_domain_info.mem_domain;
	domain_ptables = &domain->arch.ptables;

	if ((thread->base.user_options & K_USER) == 0) {
		return 0;
	}

	if ((thread->base.thread_state & _THREAD_DEAD) == 0) {
		return 0;
	}

	return reset_map(domain_ptables, __func__, thread->stack_info.start,
			 thread->stack_info.size);
}

static void z_arm64_swap_ptables(struct k_thread *incoming)
{
	struct arm_mmu_ptables *ptables = incoming->arch.ptables;
	uint64_t curr_ttbr0 = read_ttbr0_el1();
	uint64_t new_ttbr0 = ptables->ttbr0;

	if (curr_ttbr0 == new_ttbr0) {
		return; /* Already the right tables */
	}

	MMU_DEBUG("TTBR0 switch from %#llx to %#llx\n", curr_ttbr0, new_ttbr0);
	z_arm64_set_ttbr0(new_ttbr0);

	if (get_asid(curr_ttbr0) == get_asid(new_ttbr0)) {
		invalidate_tlb_all();
	}
}

void z_arm64_thread_mem_domains_init(struct k_thread *incoming)
{
	struct arm_mmu_ptables *ptables;

	if ((incoming->base.user_options & K_USER) == 0) {
		return;
	}

	ptables = incoming->arch.ptables;

	/* Map the thread stack */
	map_thread_stack(incoming, ptables);

	z_arm64_swap_ptables(incoming);
}

void z_arm64_swap_mem_domains(struct k_thread *incoming)
{
	z_arm64_swap_ptables(incoming);
}

#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_DEMAND_PAGING

static uint64_t *get_pte_location(struct arm_mmu_ptables *ptables,
				  uintptr_t virt)
{
	uint64_t *pte;
	uint64_t *table = ptables->base_xlat_table;
	unsigned int level = BASE_XLAT_LEVEL;

	for (;;) {
		pte = &table[XLAT_TABLE_VA_IDX(virt, level)];
		if (level == XLAT_LAST_LEVEL) {
			return pte;
		}

		if (is_table_desc(*pte, level)) {
			level++;
			table = pte_desc_table(*pte);
			continue;
		}

		/* anything else is unexpected */
		return NULL;
	}
}

void arch_mem_page_out(void *addr, uintptr_t location)
{
	uintptr_t virt = (uintptr_t)addr;
	uint64_t *pte = get_pte_location(&kernel_ptables, virt);
	uint64_t desc;

	__ASSERT(pte != NULL, "");
	desc = *pte;

	/* mark the entry invalid to the hardware */
	desc &= ~PTE_DESC_TYPE_MASK;
	desc |= PTE_INVALID_DESC;

	/* store the location token in place of the physical address */
	__ASSERT((location & ~PTE_PHYSADDR_MASK) == 0, "");
	desc &= ~PTE_PHYSADDR_MASK;
	desc |= location;

	/*
	 * The location token may be 0. Make sure the whole descriptor
	 * doesn't end up being zero as this would be seen as a free entry.
	 */
	desc |= PTE_BLOCK_DESC_AP_RO;

	*pte = desc;
	MMU_DEBUG("page_out: virt=%#lx location=%#lx\n", virt, location);
	debug_show_pte(pte, XLAT_LAST_LEVEL);

	sync_domains(virt, CONFIG_MMU_PAGE_SIZE, "page_out");
	invalidate_tlb_page(virt);
}

void arch_mem_page_in(void *addr, uintptr_t phys)
{
	uintptr_t virt = (uintptr_t)addr;
	uint64_t *pte = get_pte_location(&kernel_ptables, virt);
	uint64_t desc;

	__ASSERT((phys & ~PTE_PHYSADDR_MASK) == 0, "");

	__ASSERT(pte != NULL, "");
	desc = *pte;
	__ASSERT(!is_free_desc(desc), "");

	/* mark the entry valid again to the hardware */
	desc &= ~PTE_DESC_TYPE_MASK;
	desc |= PTE_PAGE_DESC;

	/* store the physical address */
	desc &= ~PTE_PHYSADDR_MASK;
	desc |= phys;

	/* mark as clean */
	desc |= PTE_BLOCK_DESC_AP_RO;

	/* and make it initially unaccessible to track unaccessed pages */
	desc &= ~PTE_BLOCK_DESC_AF;

	*pte = desc;
	MMU_DEBUG("page_in: virt=%#lx phys=%#lx\n", virt, phys);
	debug_show_pte(pte, XLAT_LAST_LEVEL);

	sync_domains(virt, CONFIG_MMU_PAGE_SIZE, "page_in");
	invalidate_tlb_page(virt);
}

enum arch_page_location arch_page_location_get(void *addr, uintptr_t *location)
{
	uintptr_t virt = (uintptr_t)addr;
	uint64_t *pte = get_pte_location(&kernel_ptables, virt);
	uint64_t desc;
	enum arch_page_location status;

	if (!pte) {
		return ARCH_PAGE_LOCATION_BAD;
	}
	desc = *pte;
	if (is_free_desc(desc)) {
		return ARCH_PAGE_LOCATION_BAD;
	}

	switch (desc & PTE_DESC_TYPE_MASK) {
	case PTE_PAGE_DESC:
		status = ARCH_PAGE_LOCATION_PAGED_IN;
		break;
	case PTE_INVALID_DESC:
		status = ARCH_PAGE_LOCATION_PAGED_OUT;
		break;
	default:
		return ARCH_PAGE_LOCATION_BAD;
	}

	*location = desc & PTE_PHYSADDR_MASK;
	return status;
}

uintptr_t arch_page_info_get(void *addr, uintptr_t *phys, bool clear_accessed)
{
	uintptr_t virt = (uintptr_t)addr;
	uint64_t *pte = get_pte_location(&kernel_ptables, virt);
	uint64_t desc;
	uintptr_t status = 0;

	if (!pte) {
		return ARCH_DATA_PAGE_NOT_MAPPED;
	}
	desc = *pte;
	if (is_free_desc(desc)) {
		return ARCH_DATA_PAGE_NOT_MAPPED;
	}

	switch (desc & PTE_DESC_TYPE_MASK) {
	case PTE_PAGE_DESC:
		status |= ARCH_DATA_PAGE_LOADED;
		break;
	case PTE_INVALID_DESC:
		/* page not loaded */
		break;
	default:
		return ARCH_DATA_PAGE_NOT_MAPPED;
	}

	if (phys) {
		*phys = desc & PTE_PHYSADDR_MASK;
	}

	if ((status & ARCH_DATA_PAGE_LOADED) == 0) {
		return status;
	}

	if ((desc & PTE_BLOCK_DESC_AF) != 0) {
		status |= ARCH_DATA_PAGE_ACCESSED;
	}

	if ((desc & PTE_BLOCK_DESC_AP_RO) == 0) {
		status |= ARCH_DATA_PAGE_DIRTY;
	}

	if (clear_accessed) {
		desc &= ~PTE_BLOCK_DESC_AF;
		*pte = desc;
		MMU_DEBUG("page_info: virt=%#lx (clearing AF)\n", virt);
		debug_show_pte(pte, XLAT_LAST_LEVEL);
		sync_domains(virt, CONFIG_MMU_PAGE_SIZE, "unaccessible");
		invalidate_tlb_page(virt);
	}

	return status;
}

#define MT_SCRATCH (MT_NORMAL | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE)

void arch_mem_scratch(uintptr_t phys)
{
	uintptr_t virt = (uintptr_t)K_MEM_SCRATCH_PAGE;
	size_t size = CONFIG_MMU_PAGE_SIZE;
	int ret = add_map(&kernel_ptables, "scratch", phys, virt, size, MT_SCRATCH);

	if (ret) {
		LOG_ERR("add_map() returned %d", ret);
	} else {
		sync_domains(virt, size, "scratch");
		invalidate_tlb_page(virt);
	}
}

static bool do_mem_page_fault(struct arch_esf *esf, uintptr_t virt)
{
	/*
	 * The k_mem_page_fault() code expects to be called with IRQs enabled
	 * if the fault happened in a context where IRQs were enabled.
	 */
	if (arch_irq_unlocked(esf->spsr)) {
		enable_irq();
	}

	bool ok = k_mem_page_fault((void *)virt);

	disable_irq();
	return ok;
}

/* Called from the fault handler. Returns true if the fault is resolved. */
bool z_arm64_do_demand_paging(struct arch_esf *esf, uint64_t esr, uint64_t far)
{
	uintptr_t virt = far;
	uint64_t *pte, desc;
	uintptr_t phys;

	/* filter relevant exceptions */
	switch (GET_ESR_EC(esr)) {
	case 0x21: /* insn abort from current EL */
	case 0x25: /* data abort from current EL */
		break;
	default:
		return false;
	}

	/* make sure the fault happened in the expected range */
	if (!IN_RANGE(virt,
		      (uintptr_t)K_MEM_VIRT_RAM_START,
		      ((uintptr_t)K_MEM_VIRT_RAM_END - 1))) {
		return false;
	}

	virt = ROUND_DOWN(virt, CONFIG_MMU_PAGE_SIZE);

	pte = get_pte_location(&kernel_ptables, virt);
	if (!pte) {
		/* page mapping doesn't exist, let the core code do its thing */
		return do_mem_page_fault(esf, virt);
	}
	desc = *pte;
	if ((desc & PTE_DESC_TYPE_MASK) != PTE_PAGE_DESC) {
		/* page is not loaded/mapped */
		return do_mem_page_fault(esf, virt);
	}

	/*
	 * From this point, we expect only 2 cases:
	 *
	 * 1) the Access Flag was not set so we set it marking the page
	 *    as accessed;
	 *
	 * 2) the page was read-only and a write occurred so we clear the
	 *    RO flag marking the page dirty.
	 *
	 * We bail out on anything else.
	 *
	 * Fault status codes for Data aborts (DFSC):
	 *  0b0010LL	Access flag fault
	 *  0b0011LL	Permission fault
	 */
	uint32_t dfsc = GET_ESR_ISS(esr) & GENMASK(5, 0);
	bool write = (GET_ESR_ISS(esr) & BIT(6)) != 0; /* WnR */

	if (dfsc == (0b001000 | XLAT_LAST_LEVEL) &&
	    (desc & PTE_BLOCK_DESC_AF) == 0) {
		/* page is being accessed: set the access flag */
		desc |= PTE_BLOCK_DESC_AF;
		if (write) {
			if ((desc & PTE_SW_WRITABLE) == 0) {
				/* we don't actually have write permission */
				return false;
			}
			/*
			 * Let's avoid another fault immediately after
			 * returning by making the page read-write right away
			 * effectively marking it "dirty" as well.
			 */
			desc &= ~PTE_BLOCK_DESC_AP_RO;
		}
		*pte = desc;
		sync_domains(virt, CONFIG_MMU_PAGE_SIZE, "accessed");
		/* no TLB inval needed after setting AF */

		/* tell the eviction algorithm about it */
		phys = desc & PTE_PHYSADDR_MASK;
		k_mem_paging_eviction_accessed(phys);
		return true;
	}

	if (dfsc == (0b001100 | XLAT_LAST_LEVEL) && write &&
	    (desc & PTE_BLOCK_DESC_AP_RO) != 0 &&
	    (desc & PTE_SW_WRITABLE) != 0) {
		/* make it "dirty" i.e. read-write */
		desc &= ~PTE_BLOCK_DESC_AP_RO;
		*pte = desc;
		sync_domains(virt, CONFIG_MMU_PAGE_SIZE, "dirtied");
		invalidate_tlb_page(virt);

		/* this also counts as an access refresh */
		phys = desc & PTE_PHYSADDR_MASK;
		k_mem_paging_eviction_accessed(phys);
		return true;
	}

	return false;
}

#endif /* CONFIG_DEMAND_PAGING */
