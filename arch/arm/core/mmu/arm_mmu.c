/*
 * ARMv7 MMU support
 *
 * This implementation supports the Short-descriptor translation
 * table format. The standard page size is 4 kB, 1 MB sections
 * are only used for mapping the code and data of the Zephyr image.
 * Secure mode and PL1 is always assumed. LPAE and PXN extensions
 * as well as TEX remapping are not supported. The AP[2:1] plus
 * Access flag permissions model is used, as the AP[2:0] model is
 * deprecated. As the AP[2:1] model can only disable write access,
 * the read permission flag is always implied.
 *
 * Reference documentation:
 * ARM Architecture Reference Manual, ARMv7-A and ARMv7-R edition,
 * ARM document ID DDI0406C Rev. d, March 2018
 *
 * Copyright (c) 2021 Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/sys/barrier.h>

#include <cmsis_core.h>

#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include "arm_mmu_priv.h"

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* Level 1 page table: always required, must be 16k-aligned */
static struct arm_mmu_l1_page_table
	l1_page_table __aligned(KB(16)) = {0};
/*
 * Array of level 2 page tables with 4k granularity:
 * each table covers a range of 1 MB, the number of L2 tables
 * is configurable.
 */
static struct arm_mmu_l2_page_table
	l2_page_tables[CONFIG_ARM_MMU_NUM_L2_TABLES] __aligned(KB(1)) = {0};
/*
 * For each level 2 page table, a separate dataset tracks
 * if the respective table is in use, if so, to which 1 MB
 * virtual address range it is assigned, and how many entries,
 * each mapping a 4 kB page, it currently contains.
 */
static struct arm_mmu_l2_page_table_status
	l2_page_tables_status[CONFIG_ARM_MMU_NUM_L2_TABLES] = {0};

/* Available L2 tables count & next free index for an L2 table request */
static uint32_t arm_mmu_l2_tables_free = CONFIG_ARM_MMU_NUM_L2_TABLES;
static uint32_t arm_mmu_l2_next_free_table;

/*
 * Static definition of all code & data memory regions of the
 * current Zephyr image. This information must be available &
 * processed upon MMU initialization.
 */
static const struct arm_mmu_flat_range mmu_zephyr_ranges[] = {
	/*
	 * Mark the zephyr execution regions (data, bss, noinit, etc.)
	 * cacheable, read / write and non-executable
	 */
	{ .name  = "zephyr_data",
	  .start = (uint32_t)_image_ram_start,
	  .end   = (uint32_t)_image_ram_end,
	  .attrs = MT_NORMAL | MATTR_SHARED |
		   MPERM_R | MPERM_W |
		   MATTR_CACHE_OUTER_WB_WA | MATTR_CACHE_INNER_WB_WA},

	/* Mark text segment cacheable, read only and executable */
	{ .name  = "zephyr_code",
	  .start = (uint32_t)__text_region_start,
	  .end   = (uint32_t)__text_region_end,
	  .attrs = MT_NORMAL | MATTR_SHARED |
	  /* The code needs to have write permission in order for
	   * software breakpoints (which modify instructions) to work
	   */
#if defined(CONFIG_GDBSTUB)
		   MPERM_R | MPERM_X | MPERM_W |
#else
		   MPERM_R | MPERM_X |
#endif
		   MATTR_CACHE_OUTER_WB_nWA | MATTR_CACHE_INNER_WB_nWA |
		   MATTR_MAY_MAP_L1_SECTION},

	/* Mark rodata segment cacheable, read only and non-executable */
	{ .name  = "zephyr_rodata",
	  .start = (uint32_t)__rodata_region_start,
	  .end   = (uint32_t)__rodata_region_end,
	  .attrs = MT_NORMAL | MATTR_SHARED |
		   MPERM_R |
		   MATTR_CACHE_OUTER_WB_nWA | MATTR_CACHE_INNER_WB_nWA |
		   MATTR_MAY_MAP_L1_SECTION},
#ifdef CONFIG_NOCACHE_MEMORY
	/* Mark nocache segment read / write and non-executable */
	{ .name  = "nocache",
	  .start = (uint32_t)_nocache_ram_start,
	  .end   = (uint32_t)_nocache_ram_end,
	  .attrs = MT_STRONGLY_ORDERED |
		   MPERM_R | MPERM_W},
#endif
};

static void arm_mmu_l2_map_page(uint32_t va, uint32_t pa,
				struct arm_mmu_perms_attrs perms_attrs);

/**
 * @brief Invalidates the TLB
 * Helper function which invalidates the entire TLB. This action
 * is performed whenever the MMU is (re-)enabled or changes to the
 * page tables are made at run-time, as the TLB might contain entries
 * which are no longer valid once the changes are applied.
 */
static void invalidate_tlb_all(void)
{
	__set_TLBIALL(0); /* 0 = opc2 = invalidate entire TLB */
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

/**
 * @brief Returns a free level 2 page table
 * Initializes and returns the next free L2 page table whenever
 * a page is to be mapped in a 1 MB virtual address range that
 * is not yet covered by a level 2 page table.
 *
 * @param va 32-bit virtual address to be mapped.
 * @retval pointer to the L2 table now assigned to the 1 MB
 *         address range the target virtual address is in.
 */
static struct arm_mmu_l2_page_table *arm_mmu_assign_l2_table(uint32_t va)
{
	struct arm_mmu_l2_page_table *l2_page_table;

	__ASSERT(arm_mmu_l2_tables_free > 0,
		 "Cannot set up L2 page table for VA 0x%08X: "
		 "no more free L2 page tables available\n",
		 va);
	__ASSERT(l2_page_tables_status[arm_mmu_l2_next_free_table].entries == 0,
		 "Cannot set up L2 page table for VA 0x%08X: "
		 "expected empty L2 table at index [%u], but the "
		 "entries value is %u\n",
		 va, arm_mmu_l2_next_free_table,
		 l2_page_tables_status[arm_mmu_l2_next_free_table].entries);

	/*
	 * Store in the status dataset of the L2 table to be returned
	 * which 1 MB virtual address range it is being assigned to.
	 * Set the current page table entry count to 0.
	 */
	l2_page_tables_status[arm_mmu_l2_next_free_table].l1_index =
		((va >> ARM_MMU_PTE_L1_INDEX_PA_SHIFT) & ARM_MMU_PTE_L1_INDEX_MASK);
	l2_page_tables_status[arm_mmu_l2_next_free_table].entries = 0;
	l2_page_table = &l2_page_tables[arm_mmu_l2_next_free_table];

	/*
	 * Decrement the available L2 page table count. As long as at
	 * least one more L2 table is available afterwards, update the
	 * L2 next free table index. If we're about to return the last
	 * available L2 table, calculating a next free table index is
	 * impossible.
	 */
	--arm_mmu_l2_tables_free;
	if (arm_mmu_l2_tables_free > 0)	{
		do {
			arm_mmu_l2_next_free_table = (arm_mmu_l2_next_free_table + 1) %
						      CONFIG_ARM_MMU_NUM_L2_TABLES;
		} while (l2_page_tables_status[arm_mmu_l2_next_free_table].entries != 0);
	}

	return l2_page_table;
}

/**
 * @brief Releases a level 2 page table
 * Releases a level 2 page table, marking it as no longer in use.
 * From that point on, it can be re-used for mappings in another
 * 1 MB virtual address range. This function is called whenever
 * it is determined during an unmap call at run-time that the page
 * table entry count in the respective page table has reached 0.
 *
 * @param l2_page_table Pointer to L2 page table to be released.
 */
static void arm_mmu_release_l2_table(struct arm_mmu_l2_page_table *l2_page_table)
{
	uint32_t l2_page_table_index = ARM_MMU_L2_PT_INDEX(l2_page_table);

	l2_page_tables_status[l2_page_table_index].l1_index = 0;
	if (arm_mmu_l2_tables_free == 0) {
		arm_mmu_l2_next_free_table = l2_page_table_index;
	}
	++arm_mmu_l2_tables_free;
}

/**
 * @brief Increments the page table entry counter of a L2 page table
 * Increments the page table entry counter of a level 2 page table.
 * Contains a check to ensure that no attempts are made to set up
 * more page table entries than the table can hold.
 *
 * @param l2_page_table Pointer to the L2 page table whose entry
 *                      counter shall be incremented.
 */
static void arm_mmu_inc_l2_table_entries(struct arm_mmu_l2_page_table *l2_page_table)
{
	uint32_t l2_page_table_index = ARM_MMU_L2_PT_INDEX(l2_page_table);

	__ASSERT(l2_page_tables_status[l2_page_table_index].entries < ARM_MMU_PT_L2_NUM_ENTRIES,
		 "Cannot increment entry count of the L2 page table at index "
		 "[%u] / addr %p / ref L1[%u]: maximum entry count already reached",
		 l2_page_table_index, l2_page_table,
		 l2_page_tables_status[l2_page_table_index].l1_index);

	++l2_page_tables_status[l2_page_table_index].entries;
}

/**
 * @brief Decrements the page table entry counter of a L2 page table
 * Decrements the page table entry counter of a level 2 page table.
 * Contains a check to ensure that no attempts are made to remove
 * entries from the respective table that aren't actually there.
 *
 * @param l2_page_table Pointer to the L2 page table whose entry
 *                      counter shall be decremented.
 */
static void arm_mmu_dec_l2_table_entries(struct arm_mmu_l2_page_table *l2_page_table)
{
	uint32_t l2_page_table_index = ARM_MMU_L2_PT_INDEX(l2_page_table);

	__ASSERT(l2_page_tables_status[l2_page_table_index].entries > 0,
		 "Cannot decrement entry count of the L2 page table at index "
		 "[%u] / addr %p / ref L1[%u]: entry count is already zero",
		 l2_page_table_index, l2_page_table,
		 l2_page_tables_status[l2_page_table_index].l1_index);

	if (--l2_page_tables_status[l2_page_table_index].entries == 0) {
		arm_mmu_release_l2_table(l2_page_table);
	}
}

/**
 * @brief Converts memory attributes and permissions to MMU format
 * Converts memory attributes and permissions as used in the boot-
 * time memory mapping configuration data array (MT_..., MATTR_...,
 * MPERM_...) to the equivalent bit (field) values used in the MMU's
 * L1 and L2 page table entries. Contains plausibility checks.
 *
 * @param attrs type/attribute/permissions flags word obtained from
 *              an entry of the mmu_config mapping data array.
 * @retval A struct containing the information from the input flags
 *         word converted to the bits / bit fields used in L1 and
 *         L2 page table entries.
 */
static struct arm_mmu_perms_attrs arm_mmu_convert_attr_flags(uint32_t attrs)
{
	struct arm_mmu_perms_attrs perms_attrs = {0};

	__ASSERT(((attrs & MT_MASK) > 0),
		 "Cannot convert attrs word to PTE control bits: no "
		 "memory type specified");
	__ASSERT(!((attrs & MPERM_W) && !(attrs & MPERM_R)),
		 "attrs must not define write permission without read "
		 "permission");
	__ASSERT(!((attrs & MPERM_W) && (attrs & MPERM_X)),
		 "attrs must not define executable memory with write "
		 "permission");

	/*
	 * The translation of the memory type / permissions / attributes
	 * flags in the attrs word to the TEX, C, B, S and AP bits of the
	 * target PTE is based on the reference manual:
	 * TEX, C, B, S: Table B3-10, chap. B3.8.2, p. B3-1363f.
	 * AP          : Table B3-6,  chap. B3.7.1, p. B3-1353.
	 * Device / strongly ordered memory is always assigned to a domain
	 * other than that used for normal memory. Assuming that userspace
	 * support utilizing the MMU is eventually implemented, a single
	 * modification of the DACR register when entering/leaving unprivi-
	 * leged mode could be used in order to enable/disable all device
	 * memory access without having to modify any PTs/PTEs.
	 */

	if (attrs & MT_STRONGLY_ORDERED) {
		/* Strongly ordered is always shareable, S bit is ignored */
		perms_attrs.tex        = 0;
		perms_attrs.cacheable  = 0;
		perms_attrs.bufferable = 0;
		perms_attrs.shared     = 0;
		perms_attrs.domain     = ARM_MMU_DOMAIN_DEVICE;
	} else if (attrs & MT_DEVICE) {
		/*
		 * Shareability of device memory is determined by TEX, C, B.
		 * The S bit is ignored. C is always 0 for device memory.
		 */
		perms_attrs.shared    = 0;
		perms_attrs.cacheable = 0;
		perms_attrs.domain    = ARM_MMU_DOMAIN_DEVICE;

		/*
		 * ARM deprecates the marking of Device memory with a
		 * shareability attribute other than Outer Shareable
		 * or Shareable. This means ARM strongly recommends
		 * that Device memory is never assigned a shareability
		 * attribute of Non-shareable or Inner Shareable.
		 */
		perms_attrs.tex        = 0;
		perms_attrs.bufferable = 1;
	} else if (attrs & MT_NORMAL) {
		/*
		 * TEX[2] is always 1. TEX[1:0] contain the outer cache attri-
		 * butes encoding, C and B contain the inner cache attributes
		 * encoding.
		 */
		perms_attrs.tex |= ARM_MMU_TEX2_CACHEABLE_MEMORY;
		perms_attrs.domain = ARM_MMU_DOMAIN_OS;

		/* For normal memory, shareability depends on the S bit */
		if (attrs & MATTR_SHARED) {
			perms_attrs.shared = 1;
		}

		if (attrs & MATTR_CACHE_OUTER_WB_WA) {
			perms_attrs.tex |= ARM_MMU_TEX_CACHE_ATTRS_WB_WA;
		} else if (attrs & MATTR_CACHE_OUTER_WT_nWA) {
			perms_attrs.tex |= ARM_MMU_TEX_CACHE_ATTRS_WT_nWA;
		} else if (attrs & MATTR_CACHE_OUTER_WB_nWA) {
			perms_attrs.tex |= ARM_MMU_TEX_CACHE_ATTRS_WB_nWA;
		}

		if (attrs & MATTR_CACHE_INNER_WB_WA) {
			perms_attrs.cacheable  = ARM_MMU_C_CACHE_ATTRS_WB_WA;
			perms_attrs.bufferable = ARM_MMU_B_CACHE_ATTRS_WB_WA;
		} else if (attrs & MATTR_CACHE_INNER_WT_nWA) {
			perms_attrs.cacheable  = ARM_MMU_C_CACHE_ATTRS_WT_nWA;
			perms_attrs.bufferable = ARM_MMU_B_CACHE_ATTRS_WT_nWA;
		} else if (attrs & MATTR_CACHE_INNER_WB_nWA) {
			perms_attrs.cacheable  = ARM_MMU_C_CACHE_ATTRS_WB_nWA;
			perms_attrs.bufferable = ARM_MMU_B_CACHE_ATTRS_WB_nWA;
		}
	}

	if (attrs & MATTR_NON_SECURE) {
		perms_attrs.non_sec = 1;
	}
	if (attrs & MATTR_NON_GLOBAL) {
		perms_attrs.not_global = 1;
	}

	/*
	 * Up next is the consideration of the case that a PTE shall be configured
	 * for a page that shall not be accessible at all (e.g. guard pages), and
	 * therefore has neither read nor write permissions. In the AP[2:1] access
	 * permission specification model, the only way to indicate this is to
	 * actually mask out the PTE's identifier bits, as otherwise, read permission
	 * is always granted for any valid PTE, it can't be revoked explicitly,
	 * unlike the write permission.
	 */
	if (!((attrs & MPERM_R) || (attrs & MPERM_W))) {
		perms_attrs.id_mask = 0x0;
	} else {
		perms_attrs.id_mask = 0x3;
	}
	if (!(attrs & MPERM_W)) {
		perms_attrs.acc_perms |= ARM_MMU_PERMS_AP2_DISABLE_WR;
	}
	if (attrs & MPERM_UNPRIVILEGED) {
		perms_attrs.acc_perms |= ARM_MMU_PERMS_AP1_ENABLE_PL0;
	}
	if (!(attrs & MPERM_X)) {
		perms_attrs.exec_never = 1;
	}

	return perms_attrs;
}

/**
 * @brief Maps a 1 MB memory range via a level 1 page table entry
 * Maps a 1 MB memory range using a level 1 page table entry of type
 * 'section'. This type of entry saves a level 2 page table, but has
 * two pre-conditions: the memory area to be mapped must contain at
 * least 1 MB of contiguous memory, starting at an address with suit-
 * able alignment. This mapping method should only be used for map-
 * pings for which it is unlikely that the attributes of those mappings
 * will mappings will change at run-time (e.g. code sections will al-
 * ways be read-only and executable). Should the case occur that the
 * permissions or attributes of a subset of a 1 MB section entry shall
 * be re-configured at run-time, a L1 section entry will be broken
 * down into 4k segments using a L2 table with identical attributes
 * before any modifications are performed for the subset of the affec-
 * ted 1 MB range. This comes with an undeterministic performance
 * penalty at the time of re-configuration, therefore, any mappings
 * for which L1 section entries are a valid option, shall be marked in
 * their declaration with the MATTR_MAY_MAP_L1_SECTION flag.
 *
 * @param va 32-bit target virtual address to be mapped.
 * @param pa 32-bit physical address to be mapped.
 * @param perms_attrs Permission and attribute bits in the format
 *                    used in the MMU's L1 page table entries.
 */
static void arm_mmu_l1_map_section(uint32_t va, uint32_t pa,
				   struct arm_mmu_perms_attrs perms_attrs)
{
	uint32_t l1_index = (va >> ARM_MMU_PTE_L1_INDEX_PA_SHIFT) &
			    ARM_MMU_PTE_L1_INDEX_MASK;

	__ASSERT(l1_page_table.entries[l1_index].undefined.id == ARM_MMU_PTE_ID_INVALID,
		 "Unexpected non-zero L1 PTE ID %u for VA 0x%08X / PA 0x%08X",
		 l1_page_table.entries[l1_index].undefined.id,
		 va, pa);

	l1_page_table.entries[l1_index].l1_section_1m.id =
		(ARM_MMU_PTE_ID_SECTION & perms_attrs.id_mask);
	l1_page_table.entries[l1_index].l1_section_1m.bufferable = perms_attrs.bufferable;
	l1_page_table.entries[l1_index].l1_section_1m.cacheable = perms_attrs.cacheable;
	l1_page_table.entries[l1_index].l1_section_1m.exec_never = perms_attrs.exec_never;
	l1_page_table.entries[l1_index].l1_section_1m.domain = perms_attrs.domain;
	l1_page_table.entries[l1_index].l1_section_1m.impl_def = 0;
	l1_page_table.entries[l1_index].l1_section_1m.acc_perms10 =
		((perms_attrs.acc_perms & 0x1) << 1) | 0x1;
	l1_page_table.entries[l1_index].l1_section_1m.tex = perms_attrs.tex;
	l1_page_table.entries[l1_index].l1_section_1m.acc_perms2 =
		(perms_attrs.acc_perms >> 1) & 0x1;
	l1_page_table.entries[l1_index].l1_section_1m.shared = perms_attrs.shared;
	l1_page_table.entries[l1_index].l1_section_1m.not_global = perms_attrs.not_global;
	l1_page_table.entries[l1_index].l1_section_1m.zero = 0;
	l1_page_table.entries[l1_index].l1_section_1m.non_sec = perms_attrs.non_sec;
	l1_page_table.entries[l1_index].l1_section_1m.base_address =
		(pa >> ARM_MMU_PTE_L1_INDEX_PA_SHIFT);
}

/**
 * @brief Converts a L1 1 MB section mapping to a full L2 table
 * When this function is called, something has happened that shouldn't
 * happen for the sake of run-time performance and determinism: the
 * attributes and/or permissions of a subset of a 1 MB memory range
 * currently represented by a level 1 page table entry of type 'section'
 * shall be modified so that they differ from the rest of the 1 MB
 * range's attributes/permissions. Therefore, the single L1 page table
 * entry has to be broken down to the full 256 4k-wide entries of a
 * L2 page table with identical properties so that afterwards, the
 * modification of the subset can be performed with a 4k granularity.
 * The risk at this point is that all L2 tables are already in use,
 * which will result in an assertion failure in the first contained
 * #arm_mmu_l2_map_page() call.
 * @warning While the conversion is being performed, interrupts are
 *          locked globally and the MMU is disabled (the required
 *          Zephyr code & data are still accessible in this state as
 *          those are identity mapped). Expect non-deterministic be-
 *          haviour / interrupt latencies while the conversion is in
 *          progress!
 *
 * @param va 32-bit virtual address within the 1 MB range that shall
 *           be converted from L1 1 MB section mapping to L2 4 kB page
 *           mappings.
 * @param l2_page_table Pointer to an empty L2 page table allocated
 *                      for the purpose of replacing the L1 section
 *                      mapping.
 */
static void arm_mmu_remap_l1_section_to_l2_table(uint32_t va,
						 struct arm_mmu_l2_page_table *l2_page_table)
{
	struct arm_mmu_perms_attrs perms_attrs = {0};
	uint32_t l1_index = (va >> ARM_MMU_PTE_L1_INDEX_PA_SHIFT) &
			    ARM_MMU_PTE_L1_INDEX_MASK;
	uint32_t rem_size = MB(1);
	uint32_t reg_val;
	int lock_key;

	/*
	 * Extract the permissions and attributes from the current 1 MB section entry.
	 * This data will be carried over to the resulting L2 page table.
	 */

	perms_attrs.acc_perms = (l1_page_table.entries[l1_index].l1_section_1m.acc_perms2 << 1) |
		((l1_page_table.entries[l1_index].l1_section_1m.acc_perms10 >> 1) & 0x1);
	perms_attrs.bufferable = l1_page_table.entries[l1_index].l1_section_1m.bufferable;
	perms_attrs.cacheable = l1_page_table.entries[l1_index].l1_section_1m.cacheable;
	perms_attrs.domain = l1_page_table.entries[l1_index].l1_section_1m.domain;
	perms_attrs.id_mask = (l1_page_table.entries[l1_index].l1_section_1m.id ==
			      ARM_MMU_PTE_ID_INVALID) ? 0x0 : 0x3;
	perms_attrs.not_global = l1_page_table.entries[l1_index].l1_section_1m.not_global;
	perms_attrs.non_sec = l1_page_table.entries[l1_index].l1_section_1m.non_sec;
	perms_attrs.shared = l1_page_table.entries[l1_index].l1_section_1m.shared;
	perms_attrs.tex = l1_page_table.entries[l1_index].l1_section_1m.tex;
	perms_attrs.exec_never = l1_page_table.entries[l1_index].l1_section_1m.exec_never;

	/*
	 * Disable interrupts - no interrupts shall occur before the L2 table has
	 * been set up in place of the former L1 section entry.
	 */

	lock_key = arch_irq_lock();

	/*
	 * Disable the MMU. The L1 PTE array and the L2 PT array may actually be
	 * covered by the L1 PTE we're about to replace, so access to this data
	 * must remain functional during the entire remap process. Yet, the only
	 * memory areas for which L1 1 MB section entries are even considered are
	 * those belonging to the Zephyr image. Those areas are *always* identity
	 * mapped, so the MMU can be turned off and the relevant data will still
	 * be available.
	 */

	reg_val = __get_SCTLR();
	__set_SCTLR(reg_val & (~ARM_MMU_SCTLR_MMU_ENABLE_BIT));

	/*
	 * Clear the entire L1 PTE & re-configure it as a L2 PT reference
	 * -> already sets the correct values for: zero0, zero1, impl_def.
	 */
	l1_page_table.entries[l1_index].word = 0;

	l1_page_table.entries[l1_index].l2_page_table_ref.id = ARM_MMU_PTE_ID_L2_PT;
	l1_page_table.entries[l1_index].l2_page_table_ref.domain = perms_attrs.domain;
	l1_page_table.entries[l1_index].l2_page_table_ref.non_sec = perms_attrs.non_sec;
	l1_page_table.entries[l1_index].l2_page_table_ref.l2_page_table_address =
		(((uint32_t)l2_page_table >> ARM_MMU_PT_L2_ADDR_SHIFT) &
		ARM_MMU_PT_L2_ADDR_MASK);

	/* Align the target VA to the base address of the section we're converting */
	va &= ~(MB(1) - 1);
	while (rem_size > 0) {
		arm_mmu_l2_map_page(va, va, perms_attrs);
		rem_size -= KB(4);
		va += KB(4);
	}

	/* Remap complete, re-enable the MMU, unlock the interrupts. */

	invalidate_tlb_all();
	__set_SCTLR(reg_val);

	arch_irq_unlock(lock_key);
}

/**
 * @brief Maps a 4 kB memory page using a L2 page table entry
 * Maps a single 4 kB page of memory from the specified physical
 * address to the specified virtual address, using the provided
 * attributes and permissions which have already been converted
 * from the system's format provided to arch_mem_map() to the
 * bits / bit masks used in the L2 page table entry.
 *
 * @param va 32-bit target virtual address.
 * @param pa 32-bit physical address.
 * @param perms_attrs Permission and attribute bits in the format
 *                    used in the MMU's L2 page table entries.
 */
static void arm_mmu_l2_map_page(uint32_t va, uint32_t pa,
				struct arm_mmu_perms_attrs perms_attrs)
{
	struct arm_mmu_l2_page_table *l2_page_table = NULL;
	uint32_t l1_index = (va >> ARM_MMU_PTE_L1_INDEX_PA_SHIFT) &
			    ARM_MMU_PTE_L1_INDEX_MASK;
	uint32_t l2_index = (va >> ARM_MMU_PTE_L2_INDEX_PA_SHIFT) &
			    ARM_MMU_PTE_L2_INDEX_MASK;

	/*
	 * Use the calculated L1 index in order to determine if a L2 page
	 * table is required in order to complete the current mapping.
	 * -> See below for an explanation of the possible scenarios.
	 */

	if (l1_page_table.entries[l1_index].undefined.id == ARM_MMU_PTE_ID_INVALID ||
	    (l1_page_table.entries[l1_index].undefined.id & ARM_MMU_PTE_ID_SECTION) != 0) {
		l2_page_table = arm_mmu_assign_l2_table(pa);
		__ASSERT(l2_page_table != NULL,
			 "Unexpected L2 page table NULL pointer for VA 0x%08X",
			 va);
	}

	/*
	 * Check what is currently present at the corresponding L1 table entry.
	 * The following scenarios are possible:
	 * 1) The L1 PTE's ID bits are zero, as is the rest of the entry.
	 *    In this case, the L1 PTE is currently unused. A new L2 PT to
	 *    refer to in this entry has already been allocated above.
	 * 2) The L1 PTE's ID bits indicate a L2 PT reference entry (01).
	 *    The corresponding L2 PT's address will be resolved using this
	 *    entry.
	 * 3) The L1 PTE's ID bits may or may not be zero, and the rest of
	 *    the descriptor contains some non-zero data. This always indicates
	 *    an existing 1 MB section entry in this place. Checking only the
	 *    ID bits wouldn't be enough, as the only way to indicate a section
	 *    with neither R nor W permissions is to set the ID bits to 00 in
	 *    the AP[2:1] permissions model. As we're now about to map a single
	 *    page overlapping with the 1 MB section, the section has to be
	 *    converted into a L2 table. Afterwards, the current page mapping
	 *    can be added/modified.
	 */

	if (l1_page_table.entries[l1_index].word == 0) {
		/* The matching L1 PT entry is currently unused */
		l1_page_table.entries[l1_index].l2_page_table_ref.id = ARM_MMU_PTE_ID_L2_PT;
		l1_page_table.entries[l1_index].l2_page_table_ref.zero0 = 0;
		l1_page_table.entries[l1_index].l2_page_table_ref.zero1 = 0;
		l1_page_table.entries[l1_index].l2_page_table_ref.impl_def = 0;
		l1_page_table.entries[l1_index].l2_page_table_ref.domain = 0; /* TODO */
		l1_page_table.entries[l1_index].l2_page_table_ref.non_sec =
			perms_attrs.non_sec;
		l1_page_table.entries[l1_index].l2_page_table_ref.l2_page_table_address =
			(((uint32_t)l2_page_table >> ARM_MMU_PT_L2_ADDR_SHIFT) &
			ARM_MMU_PT_L2_ADDR_MASK);
	} else if (l1_page_table.entries[l1_index].undefined.id == ARM_MMU_PTE_ID_L2_PT) {
		/* The matching L1 PT entry already points to a L2 PT */
		l2_page_table = (struct arm_mmu_l2_page_table *)
				((l1_page_table.entries[l1_index].word &
				(ARM_MMU_PT_L2_ADDR_MASK << ARM_MMU_PT_L2_ADDR_SHIFT)));
		/*
		 * The only configuration bit contained in the L2 PT entry is the
		 * NS bit. Set it according to the attributes passed to this function,
		 * warn if there is a mismatch between the current page's NS attribute
		 * value and the value currently contained in the L2 PT entry.
		 */
		if (l1_page_table.entries[l1_index].l2_page_table_ref.non_sec !=
		    perms_attrs.non_sec) {
			LOG_WRN("NS bit mismatch in L2 PT reference at L1 index [%u], "
				"re-configuring from %u to %u",
				l1_index,
				l1_page_table.entries[l1_index].l2_page_table_ref.non_sec,
				perms_attrs.non_sec);
			l1_page_table.entries[l1_index].l2_page_table_ref.non_sec =
				perms_attrs.non_sec;
		}
	} else if (l1_page_table.entries[l1_index].undefined.reserved != 0) {
		/*
		 * The matching L1 PT entry currently holds a 1 MB section entry
		 * in order to save a L2 table (as it's neither completely blank
		 * nor a L2 PT reference), but now we have to map an overlapping
		 * 4 kB page, so the section entry must be converted to a L2 table
		 * first before the individual L2 entry for the page to be mapped is
		 * accessed. A blank L2 PT has already been assigned above.
		 */
		arm_mmu_remap_l1_section_to_l2_table(va, l2_page_table);
	}

	/*
	 * If the matching L2 PTE is blank, increment the number of used entries
	 * in the L2 table. If the L2 PTE already contains some data, we're re-
	 * placing the entry's data instead, the used entry count remains unchanged.
	 * Once again, checking the ID bits might be misleading if the PTE declares
	 * a page which has neither R nor W permissions.
	 */
	if (l2_page_table->entries[l2_index].word == 0) {
		arm_mmu_inc_l2_table_entries(l2_page_table);
	}

	l2_page_table->entries[l2_index].l2_page_4k.id =
		(ARM_MMU_PTE_ID_SMALL_PAGE & perms_attrs.id_mask);
	l2_page_table->entries[l2_index].l2_page_4k.id |= perms_attrs.exec_never; /* XN in [0] */
	l2_page_table->entries[l2_index].l2_page_4k.bufferable = perms_attrs.bufferable;
	l2_page_table->entries[l2_index].l2_page_4k.cacheable = perms_attrs.cacheable;
	l2_page_table->entries[l2_index].l2_page_4k.acc_perms10 =
		((perms_attrs.acc_perms & 0x1) << 1) | 0x1;
	l2_page_table->entries[l2_index].l2_page_4k.tex = perms_attrs.tex;
	l2_page_table->entries[l2_index].l2_page_4k.acc_perms2 =
		((perms_attrs.acc_perms >> 1) & 0x1);
	l2_page_table->entries[l2_index].l2_page_4k.shared = perms_attrs.shared;
	l2_page_table->entries[l2_index].l2_page_4k.not_global = perms_attrs.not_global;
	l2_page_table->entries[l2_index].l2_page_4k.pa_base =
		((pa >> ARM_MMU_PTE_L2_SMALL_PAGE_ADDR_SHIFT) &
		ARM_MMU_PTE_L2_SMALL_PAGE_ADDR_MASK);
}

/**
 * @brief Unmaps a 4 kB memory page by clearing its L2 page table entry
 * Unmaps a single 4 kB page of memory from the specified virtual
 * address by clearing its respective L2 page table entry.
 *
 * @param va 32-bit virtual address to be unmapped.
 */
static void arm_mmu_l2_unmap_page(uint32_t va)
{
	struct arm_mmu_l2_page_table *l2_page_table;
	uint32_t l1_index = (va >> ARM_MMU_PTE_L1_INDEX_PA_SHIFT) &
			    ARM_MMU_PTE_L1_INDEX_MASK;
	uint32_t l2_index = (va >> ARM_MMU_PTE_L2_INDEX_PA_SHIFT) &
			    ARM_MMU_PTE_L2_INDEX_MASK;

	if (l1_page_table.entries[l1_index].undefined.id != ARM_MMU_PTE_ID_L2_PT) {
		/*
		 * No L2 PT currently exists for the given VA - this should be
		 * tolerated without an error, just as in the case that while
		 * a L2 PT exists, the corresponding PTE is blank - see explanation
		 * below, the same applies here.
		 */
		return;
	}

	l2_page_table = (struct arm_mmu_l2_page_table *)
			((l1_page_table.entries[l1_index].word &
			(ARM_MMU_PT_L2_ADDR_MASK << ARM_MMU_PT_L2_ADDR_SHIFT)));

	if (l2_page_table->entries[l2_index].word == 0) {
		/*
		 * We're supposed to unmap a page at the given VA, but there currently
		 * isn't anything mapped at this address, the L2 PTE is blank.
		 * -> This is normal if a memory area is being mapped via k_mem_map,
		 * which contains two calls to arch_mem_unmap (which effectively end up
		 * here) in order to unmap the leading and trailing guard pages.
		 * Therefore, it has to be expected that unmap calls are made for unmapped
		 * memory which hasn't been in use before.
		 * -> Just return, don't decrement the entry counter of the corresponding
		 * L2 page table, as we're not actually clearing any PTEs.
		 */
		return;
	}

	if ((l2_page_table->entries[l2_index].undefined.id & ARM_MMU_PTE_ID_SMALL_PAGE) !=
			ARM_MMU_PTE_ID_SMALL_PAGE) {
		LOG_ERR("Cannot unmap virtual memory at 0x%08X: invalid "
			"page table entry type in level 2 page table at "
			"L1 index [%u], L2 index [%u]", va, l1_index, l2_index);
		return;
	}

	l2_page_table->entries[l2_index].word = 0;

	arm_mmu_dec_l2_table_entries(l2_page_table);
}

/**
 * @brief MMU boot-time initialization function
 * Initializes the MMU at boot time. Sets up the page tables and
 * applies any specified memory mappings for either the different
 * sections of the Zephyr binary image, or for device memory as
 * specified at the SoC level.
 *
 * @retval Always 0, errors are handled by assertions.
 */
int z_arm_mmu_init(void)
{
	uint32_t mem_range;
	uint32_t pa;
	uint32_t va;
	uint32_t attrs;
	uint32_t pt_attrs = 0;
	uint32_t rem_size;
	uint32_t reg_val = 0;
	struct arm_mmu_perms_attrs perms_attrs;

	__ASSERT(KB(4) == CONFIG_MMU_PAGE_SIZE,
		 "MMU_PAGE_SIZE value %u is invalid, only 4 kB pages are supported\n",
		 CONFIG_MMU_PAGE_SIZE);

	/* Set up the memory regions pre-defined by the image */
	for (mem_range = 0; mem_range < ARRAY_SIZE(mmu_zephyr_ranges); mem_range++) {
		pa          = mmu_zephyr_ranges[mem_range].start;
		rem_size    = mmu_zephyr_ranges[mem_range].end - pa;
		attrs       = mmu_zephyr_ranges[mem_range].attrs;
		perms_attrs = arm_mmu_convert_attr_flags(attrs);

		/*
		 * Check if the L1 page table is within the region currently
		 * being mapped. If so, store the permissions and attributes
		 * of the current section. This information is required when
		 * writing to the TTBR0 register.
		 */
		if (((uint32_t)&l1_page_table >= pa) &&
				((uint32_t)&l1_page_table < (pa + rem_size))) {
			pt_attrs = attrs;
		}

		while (rem_size > 0) {
			if (rem_size >= MB(1) && (pa & 0xFFFFF) == 0 &&
			    (attrs & MATTR_MAY_MAP_L1_SECTION)) {
				/*
				 * Remaining area size > 1 MB & matching alignment
				 * -> map a 1 MB section instead of individual 4 kB
				 * pages with identical configuration.
				 */
				arm_mmu_l1_map_section(pa, pa, perms_attrs);
				rem_size -= MB(1);
				pa += MB(1);
			} else {
				arm_mmu_l2_map_page(pa, pa, perms_attrs);
				rem_size -= (rem_size >= KB(4)) ? KB(4) : rem_size;
				pa += KB(4);
			}
		}
	}

	/* Set up the memory regions defined at the SoC level */
	for (mem_range = 0; mem_range < mmu_config.num_regions; mem_range++) {
		pa          = (uint32_t)(mmu_config.mmu_regions[mem_range].base_pa);
		va          = (uint32_t)(mmu_config.mmu_regions[mem_range].base_va);
		rem_size    = (uint32_t)(mmu_config.mmu_regions[mem_range].size);
		attrs       = mmu_config.mmu_regions[mem_range].attrs;
		perms_attrs = arm_mmu_convert_attr_flags(attrs);

		while (rem_size > 0) {
			arm_mmu_l2_map_page(va, pa, perms_attrs);
			rem_size -= (rem_size >= KB(4)) ? KB(4) : rem_size;
			va += KB(4);
			pa += KB(4);
		}
	}

	/* Clear TTBR1 */
	__asm__ volatile("mcr p15, 0, %0, c2, c0, 1" : : "r"(reg_val));

	/* Write TTBCR: EAE, security not yet relevant, N[2:0] = 0 */
	__asm__ volatile("mcr p15, 0, %0, c2, c0, 2"
			     : : "r"(reg_val));

	/* Write TTBR0 */
	reg_val = ((uint32_t)&l1_page_table.entries[0] & ~0x3FFF);

	/*
	 * Set IRGN, RGN, S in TTBR0 based on the configuration of the
	 * memory area the actual page tables are located in.
	 */
	if (pt_attrs & MATTR_SHARED) {
		reg_val |= ARM_MMU_TTBR_SHAREABLE_BIT;
	}

	if (pt_attrs & MATTR_CACHE_OUTER_WB_WA) {
		reg_val |= (ARM_MMU_TTBR_RGN_OUTER_WB_WA_CACHEABLE <<
			    ARM_MMU_TTBR_RGN_SHIFT);
	} else if (pt_attrs & MATTR_CACHE_OUTER_WT_nWA) {
		reg_val |= (ARM_MMU_TTBR_RGN_OUTER_WT_CACHEABLE <<
			    ARM_MMU_TTBR_RGN_SHIFT);
	} else if (pt_attrs & MATTR_CACHE_OUTER_WB_nWA) {
		reg_val |= (ARM_MMU_TTBR_RGN_OUTER_WB_nWA_CACHEABLE <<
			    ARM_MMU_TTBR_RGN_SHIFT);
	}

	if (pt_attrs & MATTR_CACHE_INNER_WB_WA) {
		reg_val |= ARM_MMU_TTBR_IRGN0_BIT_MP_EXT_ONLY;
	} else if (pt_attrs & MATTR_CACHE_INNER_WT_nWA) {
		reg_val |= ARM_MMU_TTBR_IRGN1_BIT_MP_EXT_ONLY;
	} else if (pt_attrs & MATTR_CACHE_INNER_WB_nWA) {
		reg_val |= ARM_MMU_TTBR_IRGN0_BIT_MP_EXT_ONLY;
		reg_val |= ARM_MMU_TTBR_IRGN1_BIT_MP_EXT_ONLY;
	}

	__set_TTBR0(reg_val);

	/* Write DACR -> all domains to client = 01b. */
	reg_val = ARM_MMU_DACR_ALL_DOMAINS_CLIENT;
	__set_DACR(reg_val);

	invalidate_tlb_all();

	/* Enable the MMU and Cache in SCTLR */
	reg_val  = __get_SCTLR();
	reg_val |= ARM_MMU_SCTLR_AFE_BIT;
	reg_val |= ARM_MMU_SCTLR_ICACHE_ENABLE_BIT;
	reg_val |= ARM_MMU_SCTLR_DCACHE_ENABLE_BIT;
	reg_val |= ARM_MMU_SCTLR_MMU_ENABLE_BIT;
	__set_SCTLR(reg_val);

	return 0;
}

/**
 * @brief ARMv7-specific implementation of memory mapping at run-time
 * Maps memory according to the parameters provided by the caller
 * at run-time.
 *
 * @param virt 32-bit target virtual address.
 * @param phys 32-bit physical address.
 * @param size Size (in bytes) of the memory area to map.
 * @param flags Memory attributes & permissions. Comp. K_MEM_...
 *              flags in kernel/mm.h.
 * @retval 0 on success, -EINVAL if an invalid parameter is detected.
 */
static int __arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	uint32_t va = (uint32_t)virt;
	uint32_t pa = (uint32_t)phys;
	uint32_t rem_size = (uint32_t)size;
	uint32_t conv_flags = MPERM_R;
	struct arm_mmu_perms_attrs perms_attrs;
	int key;

	if (size == 0) {
		LOG_ERR("Cannot map physical memory at 0x%08X: invalid "
			"zero size", (uint32_t)phys);
		return -EINVAL;
	}

	switch (flags & K_MEM_CACHE_MASK) {

	case K_MEM_CACHE_NONE:
	default:
		conv_flags |= MT_DEVICE;
		break;
	case K_MEM_CACHE_WB:
		conv_flags |= MT_NORMAL;
		conv_flags |= MATTR_SHARED;
		if (flags & K_MEM_PERM_RW) {
			conv_flags |= MATTR_CACHE_OUTER_WB_WA;
			conv_flags |= MATTR_CACHE_INNER_WB_WA;
		} else {
			conv_flags |= MATTR_CACHE_OUTER_WB_nWA;
			conv_flags |= MATTR_CACHE_INNER_WB_nWA;
		}
		break;
	case K_MEM_CACHE_WT:
		conv_flags |= MT_NORMAL;
		conv_flags |= MATTR_SHARED;
		conv_flags |= MATTR_CACHE_OUTER_WT_nWA;
		conv_flags |= MATTR_CACHE_INNER_WT_nWA;
		break;

	}

	if (flags & K_MEM_PERM_RW) {
		conv_flags |= MPERM_W;
	}
	if (flags & K_MEM_PERM_EXEC) {
		conv_flags |= MPERM_X;
	}

	perms_attrs = arm_mmu_convert_attr_flags(conv_flags);

	key = arch_irq_lock();

	while (rem_size > 0) {
		arm_mmu_l2_map_page(va, pa, perms_attrs);
		rem_size -= (rem_size >= KB(4)) ? KB(4) : rem_size;
		va += KB(4);
		pa += KB(4);
	}

	arch_irq_unlock(key);

	return 0;
}

/**
 * @brief Arch-specific wrapper function for memory mapping at run-time
 * Maps memory according to the parameters provided by the caller
 * at run-time. This function wraps the ARMv7 MMU specific implementation
 * #__arch_mem_map() for the upper layers of the memory management.
 * If the map operation fails, a kernel panic will be triggered.
 *
 * @param virt 32-bit target virtual address.
 * @param phys 32-bit physical address.
 * @param size Size (in bytes) of the memory area to map.
 * @param flags Memory attributes & permissions. Comp. K_MEM_...
 *              flags in kernel/mm.h.
 */
void arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	int ret = __arch_mem_map(virt, phys, size, flags);

	if (ret) {
		LOG_ERR("__arch_mem_map() returned %d", ret);
		k_panic();
	} else {
		invalidate_tlb_all();
	}
}

/**
 * @brief ARMv7-specific implementation of memory unmapping at run-time
 * Unmaps memory according to the parameters provided by the caller
 * at run-time.
 *
 * @param addr 32-bit virtual address to unmap.
 * @param size Size (in bytes) of the memory area to unmap.
 * @retval 0 on success, -EINVAL if an invalid parameter is detected.
 */
static int __arch_mem_unmap(void *addr, size_t size)
{
	uint32_t va = (uint32_t)addr;
	uint32_t rem_size = (uint32_t)size;
	int key;

	if (addr == NULL) {
		LOG_ERR("Cannot unmap virtual memory: invalid NULL pointer");
		return -EINVAL;
	}

	if (size == 0) {
		LOG_ERR("Cannot unmap virtual memory at 0x%08X: invalid "
			"zero size", (uint32_t)addr);
		return -EINVAL;
	}

	key = arch_irq_lock();

	while (rem_size > 0) {
		arm_mmu_l2_unmap_page(va);
		rem_size -= (rem_size >= KB(4)) ? KB(4) : rem_size;
		va += KB(4);
	}

	arch_irq_unlock(key);

	return 0;
}

/**
 * @brief Arch-specific wrapper function for memory unmapping at run-time
 * Unmaps memory according to the parameters provided by the caller
 * at run-time. This function wraps the ARMv7 MMU specific implementation
 * #__arch_mem_unmap() for the upper layers of the memory management.
 *
 * @param addr 32-bit virtual address to unmap.
 * @param size Size (in bytes) of the memory area to unmap.
 */
void arch_mem_unmap(void *addr, size_t size)
{
	int ret = __arch_mem_unmap(addr, size);

	if (ret) {
		LOG_ERR("__arch_mem_unmap() returned %d", ret);
	} else {
		invalidate_tlb_all();
	}
}

/**
 * @brief Arch-specific virtual-to-physical address resolver function
 * ARMv7 MMU specific implementation of a function that resolves the
 * physical address corresponding to the given virtual address.
 *
 * @param virt 32-bit target virtual address to resolve.
 * @param phys Pointer to a variable to which the resolved physical
 *             address will be written. May be NULL if this information
 *             is not actually required by the caller.
 * @retval 0 if the physical address corresponding to the specified
 *         virtual address could be resolved successfully, -EFAULT
 *         if the specified virtual address is not currently mapped.
 */
int arch_page_phys_get(void *virt, uintptr_t *phys)
{
	uint32_t l1_index = ((uint32_t)virt >> ARM_MMU_PTE_L1_INDEX_PA_SHIFT) &
			    ARM_MMU_PTE_L1_INDEX_MASK;
	uint32_t l2_index = ((uint32_t)virt >> ARM_MMU_PTE_L2_INDEX_PA_SHIFT) &
			    ARM_MMU_PTE_L2_INDEX_MASK;
	struct arm_mmu_l2_page_table *l2_page_table;

	uint32_t pa_resolved = 0;
	uint32_t l2_pt_resolved;

	int rc = 0;
	int key;

	key = arch_irq_lock();

	if (l1_page_table.entries[l1_index].undefined.id == ARM_MMU_PTE_ID_SECTION) {
		/*
		 * If the virtual address points to a level 1 PTE whose ID bits
		 * identify it as a 1 MB section entry rather than a level 2 PT
		 * entry, the given VA belongs to a memory region used by the
		 * Zephyr image itself - it is only for those static regions that
		 * L1 Section entries are used to save L2 tables if a sufficient-
		 * ly large block of memory is specified. The memory regions be-
		 * longing to the Zephyr image are identity mapped -> just return
		 * the value of the VA as the value of the PA.
		 */
		pa_resolved = (uint32_t)virt;
	} else if (l1_page_table.entries[l1_index].undefined.id == ARM_MMU_PTE_ID_L2_PT) {
		/*
		 * The VA points to a level 1 PTE which re-directs to a level 2
		 * PT. -> Assemble the level 2 PT pointer and resolve the PA for
		 * the specified VA from there.
		 */
		l2_pt_resolved =
			l1_page_table.entries[l1_index].l2_page_table_ref.l2_page_table_address;
		l2_pt_resolved <<= ARM_MMU_PT_L2_ADDR_SHIFT;
		l2_page_table = (struct arm_mmu_l2_page_table *)l2_pt_resolved;

		/*
		 * Check if the PTE for the specified VA is actually in use before
		 * assembling & returning the corresponding PA. k_mem_unmap will
		 * call this function for the leading & trailing guard pages when
		 * unmapping a VA. As those guard pages were explicitly unmapped
		 * when the VA was originally mapped, their L2 PTEs will be empty.
		 * In that case, the return code of this function must not be 0.
		 */
		if (l2_page_table->entries[l2_index].word == 0) {
			rc = -EFAULT;
		}

		pa_resolved = l2_page_table->entries[l2_index].l2_page_4k.pa_base;
		pa_resolved <<= ARM_MMU_PTE_L2_SMALL_PAGE_ADDR_SHIFT;
		pa_resolved |= ((uint32_t)virt & ARM_MMU_ADDR_BELOW_PAGE_GRAN_MASK);
	} else {
		/* The level 1 PTE is invalid -> the specified VA is not mapped */
		rc = -EFAULT;
	}

	arch_irq_unlock(key);

	if (phys) {
		*phys = (uintptr_t)pa_resolved;
	}
	return rc;
}
