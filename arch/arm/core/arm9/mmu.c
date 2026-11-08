/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv5 MMU setup
 *
 * Builds a flat (identity) mapping of the whole 4 GB address space out of 1 MB
 * sections. Sections the SoC has not described are mapped as device memory,
 * which is the safe default for anything that might be a peripheral.
 *
 * All mappings live in domain 0, which is configured as "manager", so section
 * access permissions are not checked. There is no userspace support in this
 * port, so nothing would be gained by enforcing them.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arm/arm9/mmu.h>
#include <zephyr/arch/arm/arm9/lib_helpers.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/linker/linker-defs.h>
#include <string.h>

#define L1_ENTRIES      4096U
#define L1_SECTION_SIZE (1024U * 1024U)
#define L2_ENTRIES      256U
#define L2_PAGE_SIZE    4096U

/* Level 1 descriptor types */
#define L1_TYPE_FAULT   0x0U
#define L1_TYPE_COARSE  0x1U
#define L1_TYPE_SECTION 0x2U

/* Level 2 descriptor types */
#define L2_TYPE_SMALL_PAGE 0x2U

#define L1_SECTION_AP_RW (3U << 10)
/* Bit 4 of a level 1 descriptor is should-be-one, except on XScale. */
#ifdef CONFIG_CPU_XSCALE
#define L1_SECTION_SBO 0U
#else
#define L1_SECTION_SBO BIT(4)
#endif
#define L1_CACHEABLE        BIT(3)
#define L1_BUFFERABLE       BIT(2)
#define L1_TYPE_MASK        0x3U
#define L1_COARSE_BASE_MASK 0xfffffc00U

#define L2_SMALL_PAGE_AP_RW 0x00000ff0U /* AP0..AP3 = 0b11 */
#define L2_CACHEABLE        BIT(3)
#define L2_BUFFERABLE       BIT(2)

/* Domain 0 = manager, all other domains = no access */
#define DACR_INIT 0x00000003U

static uint32_t l1_table[L1_ENTRIES] __aligned(16 * 1024);

#ifdef CONFIG_ARMV5_HIGH_VECTORS
static uint32_t l2_vector_table[L2_ENTRIES] __aligned(1024);
static uint8_t vector_page[4096] __aligned(4096);
#endif

static uint32_t section_attrs(uint32_t attrs)
{
	switch (attrs) {
	case ARMV5_MMU_MEM_CACHED:
		return L1_CACHEABLE | L1_BUFFERABLE;
	case ARMV5_MMU_MEM_BUFFERED:
		return L1_BUFFERABLE;
	default:
		return 0U;
	}
}

static void map_range(uintptr_t base, size_t size, uint32_t attrs)
{
	uintptr_t addr = ROUND_DOWN(base, L1_SECTION_SIZE);
	uintptr_t end = ROUND_UP(base + size, L1_SECTION_SIZE);
	uint32_t cb = section_attrs(attrs);

	for (; addr < end; addr += L1_SECTION_SIZE) {
		l1_table[addr >> 20] = (addr & 0xfff00000U) | L1_SECTION_AP_RW | L1_SECTION_SBO |
				       cb | L1_TYPE_SECTION;

		/* Wrapped past the top of the address space. */
		if (addr + L1_SECTION_SIZE < addr) {
			break;
		}
	}
}

#ifdef CONFIG_ARMV5_HIGH_VECTORS
static void map_vector_page(void)
{
	uint32_t l1_idx = VECTOR_BASE_HIGH >> 20;
	uint32_t l2_idx = (VECTOR_BASE_HIGH >> 12) & (L2_ENTRIES - 1);

	memset(l2_vector_table, 0, sizeof(l2_vector_table));

	l2_vector_table[l2_idx] = ((uint32_t)vector_page & 0xfffff000U) | L2_SMALL_PAGE_AP_RW |
				  L2_CACHEABLE | L2_BUFFERABLE | L2_TYPE_SMALL_PAGE;

	l1_table[l1_idx] =
		((uint32_t)l2_vector_table & 0xfffffc00U) | L1_SECTION_SBO | L1_TYPE_COARSE;
}
#endif /* CONFIG_ARMV5_HIGH_VECTORS */

#ifdef CONFIG_NOCACHE_MEMORY
static uint32_t l2_tables[CONFIG_ARMV5_MMU_NUM_L2_TABLES][L2_ENTRIES] __aligned(1024);

/*
 * Return the coarse page table of the 1 MB section holding addr, replacing
 * a section mapping with 256 small pages of the same attributes first.
 * The C and B bits sit at the same place in both descriptor formats.
 */
static uint32_t *section_to_pages(uintptr_t addr)
{
	static unsigned int l2_tables_used;
	uint32_t *l1 = &l1_table[addr >> 20];
	uint32_t base = addr & 0xfff00000U;
	uint32_t cb;
	uint32_t *l2;

	if ((*l1 & L1_TYPE_MASK) == L1_TYPE_COARSE) {
		return (uint32_t *)(*l1 & L1_COARSE_BASE_MASK);
	}

	__ASSERT(l2_tables_used < CONFIG_ARMV5_MMU_NUM_L2_TABLES,
		 "CONFIG_ARMV5_MMU_NUM_L2_TABLES too small");

	l2 = l2_tables[l2_tables_used++];
	cb = *l1 & (L1_CACHEABLE | L1_BUFFERABLE);

	for (uint32_t i = 0; i < L2_ENTRIES; i++) {
		l2[i] = (base + (i * L2_PAGE_SIZE)) | L2_SMALL_PAGE_AP_RW | cb | L2_TYPE_SMALL_PAGE;
	}

	*l1 = ((uint32_t)l2 & L1_COARSE_BASE_MASK) | L1_SECTION_SBO | L1_TYPE_COARSE;

	return l2;
}

/* Change the attributes of the 4 kB pages covering [base, base + size). */
static void map_pages(uintptr_t base, size_t size, uint32_t attrs)
{
	uint32_t cb = section_attrs(attrs);

	for (uintptr_t addr = ROUND_DOWN(base, L2_PAGE_SIZE); addr < base + size;
	     addr += L2_PAGE_SIZE) {
		uint32_t *pte = &section_to_pages(addr)[(addr >> 12) & (L2_ENTRIES - 1U)];

		*pte = (*pte & ~(L2_CACHEABLE | L2_BUFFERABLE)) | cb;
	}
}
#endif /* CONFIG_NOCACHE_MEMORY */

void z_armv5_mmu_init(void)
{
	/* Everything is device memory until the SoC says otherwise. */
	for (uint32_t i = 0; i < L1_ENTRIES; i++) {
		l1_table[i] = (i << 20) | L1_SECTION_AP_RW | L1_SECTION_SBO | L1_TYPE_SECTION;
	}

	for (unsigned int i = 0; i < soc_mmu_config.num_regions; i++) {
		const struct armv5_mmu_region *r = &soc_mmu_config.regions[i];

		map_range(r->base, r->size, r->attrs);
	}

#ifdef CONFIG_NOCACHE_MEMORY
	map_pages((uintptr_t)_nocache_ram_start,
		  (uintptr_t)_nocache_ram_end - (uintptr_t)_nocache_ram_start,
		  ARMV5_MMU_MEM_BUFFERED);
#endif

#ifdef CONFIG_ARMV5_HIGH_VECTORS
	map_vector_page();
#endif

	/* Publish the tables before pointing the walker at them. */
	__DSB();

	__set_DACR(DACR_INIT);
	__set_TTBR0((uint32_t)l1_table);

	/* Invalidate the TLBs and the caches; their contents predate the map. */
	MCR15(c8, 0, c7, 0, 0);
	MCR15(c7, 0, c7, 0, 0);
	__DSB();

	__set_SCTLR(__get_SCTLR() | SCTLR_M_BIT);
	__ISB();
}
