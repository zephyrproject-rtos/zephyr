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

#define L1_ENTRIES      4096U
#define L1_SECTION_SIZE (1024U * 1024U)

/* Level 1 descriptor types */
#define L1_TYPE_FAULT   0x0U
#define L1_TYPE_SECTION 0x2U

#define L1_SECTION_AP_RW (3U << 10)
/* Bit 4 of a level 1 descriptor is should-be-one. */
#define L1_SECTION_SBO   BIT(4)
#define L1_CACHEABLE     BIT(3)
#define L1_BUFFERABLE    BIT(2)

/* Domain 0 = manager, all other domains = no access */
#define DACR_INIT 0x00000003U

static uint32_t l1_table[L1_ENTRIES] __aligned(16 * 1024);

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
