/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include "smmu_page_map.h"
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(smmu_page_map, LOG_LEVEL_DBG);

#define pmap_load(table) (*table)
// #define pmap_store(table, entry) (*table = entry)

static ALWAYS_INLINE void pmap_store(uint64_t *table, uint64_t entry)
{
	*table = entry;
	LOG_DBG("%p <- 0x%llx", table, entry);
}

static ALWAYS_INLINE void pmap_clear(uint64_t *table)
{
	*table = 0;
	LOG_DBG("%p <- 0", table);
}

/**
 * Return the lowest valid pde for a given virtual address.
 *
 */
static pd_entry_t *page_map_pde(struct page_map *pmap, mem_addr_t va, int *level)
{
	pd_entry_t *lp, *l_ret, ln, desc;
	int lvl;

	lp = pmap->base_xlat_table;
	l_ret = NULL;
	for (lvl = pmap->base_xlat_level; lvl < PAGE_Ln_N - 1; lvl++) {

		lp += smmu_xlat_index(va, lvl);
		ln = pmap_load(lp);
		desc = FIELD_GET(ATTR_DESCR_TYPE_M, ln);
		if (desc != SMMU_Ln_TABLE) {
			*level = lvl - 1;
			return l_ret;
		}
		l_ret = lp;
		lp = (pd_entry_t *)(ln & ADDR_Ln_NLTA);
	}

	*level = lvl - 1;
	return l_ret;
}

/**
 * Returns the lowest valid pte table entry for a given virtual address.
 * If there are no valid entries return NULL and set the level to the first
 * invalid level.
 */
static pt_entry_t *page_map_pte(struct page_map *pmap, mem_addr_t va, int *level)
{
	pt_entry_t *lp, *l_ret, ln, desc;
	int lvl;

	lp = pmap->base_xlat_table;
	l_ret = NULL;

	for (lvl = pmap->base_xlat_level; lvl < PAGE_Ln_N; lvl++) {
		lp += smmu_xlat_index(va, lvl);
		ln = pmap_load(lp);
		desc = FIELD_GET(ATTR_DESCR_TYPE_M, ln);
		if (desc != SMMU_Ln_TABLE) {
			*level = lvl;
			return NULL;
		}
		l_ret = lp;
		lp = (pt_entry_t *)(ln & ADDR_Ln_NLTA);
	}

	*level = lvl - 1;
	return l_ret;
}

static pd_entry_t *page_map_add_table_desc(pd_entry_t *ln, int level, mem_addr_t va)
{
	pd_entry_t *lnp = NULL, *ln_next, lx;

	__ASSERT((0 <= level && level <= 2), "Wrong level(%d) was given.", level);

	lnp = ln + smmu_xlat_index(va, level);
	__ASSERT((*lnp & ATTR_DESCR_VALID_B) == 0, "%d -level page table descritpor (%p) is valid",
		 level, lnp);
	ln_next = page_table_alloc_empty();
	if (ln_next == NULL) {
		LOG_ERR("Allocate empty page table failed");
		return NULL;
	}

	lx = (pd_entry_t)ln_next;
	lx |= SMMU_L0_TABLE;

	*lnp = lx;
	return ln_next;
}

int page_map_init(struct page_map *pmap, uint16_t va_size)
{
	pd_entry_t *entry;

	__ASSERT(va_size <= VA_MAX_SIZE,
		 "va_size (%d) is out of range (" STRINGIFY(VA_MAX_SIZE) "bits)");

	k_mutex_init(&pmap->mux);

	entry = (pd_entry_t *)page_table_alloc_empty();
	if (entry == NULL) {
		return -ENOSPC;
	}

	pmap->base_xlat_table = entry;
	pmap->base_xlat_level = SMMU_GET_BASE_XLAT_LEVEL(va_size);
	pmap->paddr = (mem_addr_t)entry;
	pmap->va_size = va_size;

	return 0;
}

int page_map_release(struct page_map *pmap)
{
	return -ENOSYS;
}

int page_map_smmu_add(struct page_map *pmap, mem_addr_t va, mem_addr_t pa, int32_t extra_attrs)
{
	pd_entry_t *pde;
	pd_entry_t new_l3;
	pd_entry_t *lp;
	pt_entry_t *l3;
	int level;
	int ret = 0;

	__ASSERT(va < (1UL << VA_MAX_SIZE), "Wrong virtual address (%lx)", va);
	__ASSERT(FIELD_GET(PAGE_M, pa) == 0, "Wrong physical address (%lx)", pa);

	va = trunc_page(va);
	new_l3 = (pt_entry_t)(pa | ATTR_DEFAULT | ATTR_S1_IDX(MEMATTR_NORMAL) | SMMU_L3_PAGE);
	if ((extra_attrs & SMMU_ATTRS_READ_ONLY) == 1) {
		new_l3 |= ATTR_S1_AP(ATTR_S1_AP_RO);
	}
	new_l3 |= ATTR_S1_XN;
	new_l3 |= ATTR_S1_AP(ATTR_S1_AP_USER_RW);
	new_l3 |= ATTR_S1_nG;

	LOG_DBG("pmap: %.16lx -> %.16lx, attr: 0x%llx", va, pa, new_l3);

	k_mutex_lock(&pmap->mux, K_FOREVER);
	pde = page_map_pde(pmap, va, &level);
	lp = pde == NULL ? pmap->base_xlat_table : pde;

	if (level < 2) {
		for (; level < PAGE_Ln_N - 2; level++) {
			lp = page_map_add_table_desc(lp, level + 1, va);
		}
	} else {
		lp = (pt_entry_t *)(pmap_load(lp) & ADDR_Ln_NLTA);
	}
	l3 = lp + smmu_l3_index(va);
	pmap_store(l3, new_l3);

	k_mutex_unlock(&pmap->mux);
	return ret;
}

int page_map_smmu_remove(struct page_map *pmap, mem_addr_t va)
{
	pt_entry_t *pte;
	int lvl;
	int ret;

	k_mutex_lock(&pmap->mux, K_FOREVER);

	pte = page_map_pte(pmap, va, &lvl);
	__ASSERT(lvl == 3, "Invalid SMMU pagetable level: %d != 3", lvl);

	if (pte == NULL) {
		ret = -ENOENT;
	} else {
		pmap_clear(pte);
		ret = 0;
	}

	return 0;
}
