/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/xtensa/arch_inlines.h>
#include <zephyr/arch/xtensa/mpu.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util_macro.h>

#include <xtensa/corebits.h>
#include <xtensa/config/core-matmap.h>
#include <xtensa/config/core-isa.h>
#include <xtensa_mpu_priv.h>

#ifdef CONFIG_USERSPACE
BUILD_ASSERT((CONFIG_PRIVILEGED_STACK_SIZE > 0) &&
	     (CONFIG_PRIVILEGED_STACK_SIZE % XCHAL_MPU_ALIGN) == 0);
#endif

extern char _heap_end[];
extern char _heap_start[];

/** MPU foreground map for kernel mode. */
static struct xtensa_mpu_map xtensa_mpu_map_fg_kernel;

/** Make sure write to the MPU region is atomic. */
static struct k_spinlock xtensa_mpu_lock;

/*
 * Additional information about the MPU maps: foreground and background
 * maps.
 *
 *
 * Some things to keep in mind:
 * - Each MPU region is described by TWO entries:
 *   [entry_a_address, entry_b_address). For contiguous memory regions,
 *   this should not much of an issue. However, disjoint memory regions
 *   "waste" another entry to describe the end of those regions.
 *   We might run out of available entries in the MPU map because of
 *   this.
 *   - The last entry is a special case as there is no more "next"
 *     entry in the map. In this case, the end of memory is
 *     the implicit boundary. In another word, the last entry
 *     describes the region between the start address of this entry
 *     and the end of memory.
 * - Current implementation has following limitations:
 *   - All enabled entries are grouped towards the end of the map.
 *     - Except the last entry which can be disabled. This is
 *       the end of the last foreground region. With a disabled
 *       entry, memory after this will use the background map
 *       for access control.
 *   - No disabled MPU entries allowed in between.
 *
 *
 * For foreground map to be valid, its entries must follow these rules:
 * - The start addresses must always be in non-descending order.
 * - The access rights and memory type fields must contain valid values.
 * - The segment field needs to be correct for each entry.
 * - MBZ fields must contain only zeroes.
 * - Although the start address occupies 27 bits of the register,
 *   it does not mean all 27 bits are usable. The macro
 *   XCHAL_MPU_ALIGN_BITS provided by the toolchain indicates
 *   that only bits of and left of this value are valid. This
 *   corresponds to the minimum segment size (MINSEGMENTSIZE)
 *   definied in the processor configuration.
 */

#ifndef CONFIG_XTENSA_MPU_ONLY_SOC_RANGES
/**
 * Static definition of all code and data memory regions of the
 * current Zephyr image. This information must be available and
 * need to be processed upon MPU initialization.
 */
static const struct xtensa_mpu_range mpu_zephyr_ranges[] = {
	/* Region for vector handlers. */
	{
		.start = (uintptr_t)XCHAL_VECBASE_RESET_VADDR,
		/*
		 * There is nothing from the Xtensa overlay about how big
		 * the vector handler region is. So we make an assumption
		 * that vecbase and .text are contiguous.
		 *
		 * SoC can override as needed if this is not the case,
		 * especially if the SoC reset/startup code relocates
		 * vecbase.
		 */
		.end   = (uintptr_t)__text_region_start,
		.access_rights = XTENSA_MPU_ACCESS_P_RX_U_RX,
		.memory_type = CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,
	},
	/*
	 * Mark the zephyr execution regions (data, bss, noinit, etc.)
	 * cacheable, read / write and non-executable
	 */
	{
		/* This includes .data, .bss and various kobject sections. */
		.start = (uintptr_t)_image_ram_start,
		.end   = (uintptr_t)_image_ram_end,
		.access_rights = XTENSA_MPU_ACCESS_P_RW_U_NA,
		.memory_type = CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,
	},
#if K_HEAP_MEM_POOL_SIZE > 0
	/* System heap memory */
	{
		.start = (uintptr_t)_heap_start,
		.end   = (uintptr_t)_heap_end,
		.access_rights = XTENSA_MPU_ACCESS_P_RW_U_NA,
		.memory_type = CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,
	},
#endif
	/* Mark text segment cacheable, read only and executable */
	{
		.start = (uintptr_t)__text_region_start,
		.end   = (uintptr_t)__text_region_end,
		.access_rights = XTENSA_MPU_ACCESS_P_RX_U_RX,
		.memory_type = CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,
	},
	/* Mark rodata segment cacheable, read only and non-executable */
	{
		.start = (uintptr_t)__rodata_region_start,
		.end   = (uintptr_t)__rodata_region_end,
		.access_rights = XTENSA_MPU_ACCESS_P_RO_U_RO,
		.memory_type = CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,
	},
};
#endif /* !CONFIG_XTENSA_MPU_ONLY_SOC_RANGES */

/**
 * Return the pointer to the entry encompassing @a addr out of an array of MPU entries.
 *
 * Returning the entry where @a addr is greater or equal to the entry's start address,
 * and where @a addr is less than the starting address of the next entry.
 *
 * @param[in]  entries Array of MPU entries.
 * @param[in]  addr Address to be matched to one background entry.
 * @param[in]  first_enabled_idx The index of the first enabled entry.
 *                               Use 0 if not sure.
 * @param[out] exact Set to true if address matches exactly.
 *                   NULL if do not care.
 * @param[out] entry_idx Set to the index of the entry array if entry is found.
 *                       NULL if do not care.
 *
 * @return Pointer to the map entry encompassing @a addr, or NULL if no such entry found.
 */
static const
struct xtensa_mpu_entry *check_addr_in_mpu_entries(const struct xtensa_mpu_entry *entries,
						   uintptr_t addr, uint8_t first_enabled_idx,
						   bool *exact, uint8_t *entry_idx)
{
	const struct xtensa_mpu_entry *ret = NULL;
	uintptr_t s_addr, e_addr;
	uint8_t idx;

	if (first_enabled_idx >= XTENSA_MPU_NUM_ENTRIES) {
		goto out_null;
	}

	if (addr < xtensa_mpu_entry_start_address_get(&entries[first_enabled_idx])) {
		/* Before the start address of very first entry. So no match. */
		goto out_null;
	}

	/* Loop through the map except the last entry (which is a special case). */
	for (idx = first_enabled_idx; idx < (XTENSA_MPU_NUM_ENTRIES - 1); idx++) {
		s_addr = xtensa_mpu_entry_start_address_get(&entries[idx]);
		e_addr = xtensa_mpu_entry_start_address_get(&entries[idx + 1]);

		if ((addr >= s_addr) && (addr < e_addr)) {
			ret = &entries[idx];
			goto out;
		}
	}

	idx = XTENSA_MPU_NUM_ENTRIES - 1;
	s_addr = xtensa_mpu_entry_start_address_get(&entries[idx]);
	if (addr >= s_addr) {
		/* Last entry encompasses the start address to end of memory. */
		ret = &entries[idx];
	}

out:
	if (ret != NULL) {
		if (exact != NULL) {
			if (addr == s_addr) {
				*exact = true;
			} else {
				*exact = false;
			}
		}

		if (entry_idx != NULL) {
			*entry_idx = idx;
		}
	}

out_null:
	return ret;
}

/**
 * Find the first enabled MPU entry.
 *
 * @param entries Array of MPU entries with XTENSA_MPU_NUM_ENTRIES elements.
 *
 * @return Index of the first enabled entry.
 * @retval XTENSA_MPU_NUM_ENTRIES if no entry is enabled.
 */
static inline uint8_t find_first_enabled_entry(const struct xtensa_mpu_entry *entries)
{
	int first_enabled_idx;

	for (first_enabled_idx = 0; first_enabled_idx < XTENSA_MPU_NUM_ENTRIES;
	     first_enabled_idx++) {
		if (entries[first_enabled_idx].as.p.enable) {
			break;
		}
	}

	return first_enabled_idx;
}

/**
 * Compare two MPU entries.
 *
 * This is used by qsort to compare two MPU entries on their ordering
 * based on starting address.
 *
 * @param a First MPU entry.
 * @param b Second MPU entry.
 *
 * @retval -1 First address is less than second address.
 * @retval  0 First address is equal to second address.
 * @retval  1 First address is great than second address.
 */
static int compare_entries(const void *a, const void *b)
{
	struct xtensa_mpu_entry *e_a = (struct xtensa_mpu_entry *)a;
	struct xtensa_mpu_entry *e_b = (struct xtensa_mpu_entry *)b;

	uintptr_t addr_a = xtensa_mpu_entry_start_address_get(e_a);
	uintptr_t addr_b = xtensa_mpu_entry_start_address_get(e_b);

	if (addr_a < addr_b) {
		return -1;
	} else if (addr_a == addr_b) {
		return 0;
	} else {
		return 1;
	}
}

/**
 * Sort the MPU entries base on starting address.
 *
 * This sorts the MPU entries in ascending order of starting address.
 * After sorting, it rewrites the segment numbers of all entries.
 */
static void sort_entries(struct xtensa_mpu_entry *entries)
{
	qsort(entries, XTENSA_MPU_NUM_ENTRIES, sizeof(entries[0]), compare_entries);

	for (uint32_t idx = 0; idx < XTENSA_MPU_NUM_ENTRIES; idx++) {
		/* Segment value must correspond to the index. */
		entries[idx].at.p.segment = idx;
	}
}

/**
 * Consolidate the MPU entries.
 *
 * This removes consecutive entries where the attributes are the same.
 *
 * @param entries Array of MPU entries with XTENSA_MPU_NUM_ENTRIES elements.
 * @param first_enabled_idx Index of first enabled entry.
 *
 * @return Index of the first enabled entry after consolidation.
 */
static uint8_t consolidate_entries(struct xtensa_mpu_entry *entries,
				   uint8_t first_enabled_idx)
{
	uint8_t new_first;
	uint8_t idx_0 = first_enabled_idx;
	uint8_t idx_1 = first_enabled_idx + 1;
	bool to_consolidate = false;

	/* For each a pair of entries... */
	while (idx_1 < XTENSA_MPU_NUM_ENTRIES) {
		struct xtensa_mpu_entry *entry_0 = &entries[idx_0];
		struct xtensa_mpu_entry *entry_1 = &entries[idx_1];
		bool mark_disable_0 = false;
		bool mark_disable_1 = false;

		if (xtensa_mpu_entries_has_same_attributes(entry_0, entry_1)) {
			/*
			 * If both entry has same attributes (access_rights and memory type),
			 * they can be consolidated into one by removing the higher indexed
			 * one.
			 */
			mark_disable_1 = true;
		} else if (xtensa_mpu_entries_has_same_address(entry_0, entry_1)) {
			/*
			 * If both entries have the same address, the higher index
			 * one always override the lower one. So remove the lower indexed
			 * one.
			 */
			mark_disable_0 = true;
		}

		/*
		 * Marking an entry as disabled here so it can be removed later.
		 *
		 * The MBZ field of the AS register is re-purposed to indicate that
		 * this is an entry to be removed.
		 */
		if (mark_disable_1) {
			/* Remove the higher indexed entry. */
			to_consolidate = true;

			entry_1->as.p.mbz = 1U;

			/* Skip ahead for next comparison. */
			idx_1++;
			continue;
		} else if (mark_disable_0) {
			/* Remove the lower indexed entry. */
			to_consolidate = true;

			entry_0->as.p.mbz = 1U;
		}

		idx_0 = idx_1;
		idx_1++;
	}

	if (to_consolidate) {
		uint8_t read_idx = XTENSA_MPU_NUM_ENTRIES - 1;
		uint8_t write_idx = XTENSA_MPU_NUM_ENTRIES;

		/* Go through the map from the end and copy enabled entries in place. */
		while (read_idx >= first_enabled_idx) {
			struct xtensa_mpu_entry *entry_rd = &entries[read_idx];

			if (entry_rd->as.p.mbz != 1U) {
				struct xtensa_mpu_entry *entry_wr;

				write_idx--;
				entry_wr = &entries[write_idx];

				*entry_wr = *entry_rd;
				entry_wr->at.p.segment = write_idx;
			}

			read_idx--;
		}

		/* New first enabled entry is where the last written entry is. */
		new_first = write_idx;

		for (idx_0 = 0; idx_0 < new_first; idx_0++) {
			struct xtensa_mpu_entry *e = &entries[idx_0];

			/* Shortcut to zero out address and enabled bit. */
			e->as.raw = 0U;

			/* Segment value must correspond to the index. */
			e->at.p.segment = idx_0;

			/* No access at all for both kernel and user modes. */
			e->at.p.access_rights =	XTENSA_MPU_ACCESS_P_NA_U_NA;

			/* Use default memory type for disabled entries. */
			e->at.p.memory_type = CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE;
		}
	} else {
		/* No need to conlidate entries. Map is same as before. */
		new_first = first_enabled_idx;
	}

	return new_first;
}

/**
 * Add a memory region to the MPU map.
 *
 * This adds a memory region to the MPU map, by setting the appropriate
 * start and end entries. This may re-use existing entries or add new
 * entries to the map.
 *
 * @param[in,out] map Pointer to MPU map.
 * @param[in] start_addr Start address of the region.
 * @param[in] end_addr End address of the region.
 * @param[in] access_rights Access rights of this region.
 * @param[in] memory_type Memory type of this region.
 * @param[out] first_idx Return index of first enabled entry if not NULL.
 *
 * @retval 0 Successful in adding the region.
 * @retval -EINVAL Invalid values in function arguments.
 */
static int mpu_map_region_add(struct xtensa_mpu_map *map,
			      uintptr_t start_addr, uintptr_t end_addr,
			      uint32_t access_rights, uint32_t memory_type,
			      uint8_t *first_idx)
{
	int ret;
	bool exact_s, exact_e;
	uint8_t idx_s, idx_e, first_enabled_idx;
	struct xtensa_mpu_entry *entry_slot_s, *entry_slot_e, prev_entry;

	struct xtensa_mpu_entry *entries = map->entries;

	if (start_addr >= end_addr) {
		ret = -EINVAL;
		goto out;
	}

	first_enabled_idx = find_first_enabled_entry(entries);
	if (first_enabled_idx >= XTENSA_MPU_NUM_ENTRIES) {

		/*
		 * If the last entry in the map is not enabled and the start
		 * address is NULL, we can assume the map has not been populated
		 * at all. This is because we group all enabled entries at
		 * the end of map.
		 */
		struct xtensa_mpu_entry *last_entry = &entries[XTENSA_MPU_NUM_ENTRIES - 1];

		if (!xtensa_mpu_entry_enable_get(last_entry) &&
		    (xtensa_mpu_entry_start_address_get(last_entry) == 0U)) {
			/* Empty table, so populate the entries as-is. */
			if (end_addr == 0xFFFFFFFFU) {
				/*
				 * Region goes to end of memory, so only need to
				 * program one entry.
				 */
				entry_slot_s = &entries[XTENSA_MPU_NUM_ENTRIES - 1];

				xtensa_mpu_entry_set(entry_slot_s, start_addr, true,
						     access_rights, memory_type);
				first_enabled_idx = XTENSA_MPU_NUM_ENTRIES - 1;
				goto end;
			} else {
				/*
				 * Populate the last two entries to indicate
				 * a memory region. Notice that the second entry
				 * is not enabled as it is merely marking the end of
				 * a region and is not the starting of another
				 * enabled MPU region.
				 */
				entry_slot_s = &entries[XTENSA_MPU_NUM_ENTRIES - 2];
				entry_slot_e = &entries[XTENSA_MPU_NUM_ENTRIES - 1];

				xtensa_mpu_entry_set(entry_slot_s, start_addr, true,
						     access_rights, memory_type);
				xtensa_mpu_entry_set(entry_slot_e, end_addr, false,
						     XTENSA_MPU_ACCESS_P_NA_U_NA,
						     CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE);
				first_enabled_idx = XTENSA_MPU_NUM_ENTRIES - 2;
				goto end;
			}

			ret = 0;
			goto out;
		}

		first_enabled_idx = consolidate_entries(entries, first_enabled_idx);

		if (first_enabled_idx >= XTENSA_MPU_NUM_ENTRIES) {
			ret = -EINVAL;
			goto out;
		}
	}

	entry_slot_s = (struct xtensa_mpu_entry *)
		       check_addr_in_mpu_entries(entries, start_addr, first_enabled_idx,
						 &exact_s, &idx_s);
	entry_slot_e = (struct xtensa_mpu_entry *)
		       check_addr_in_mpu_entries(entries, end_addr, first_enabled_idx,
						 &exact_e, &idx_e);

	__ASSERT_NO_MSG(entry_slot_s != NULL);
	__ASSERT_NO_MSG(entry_slot_e != NULL);
	__ASSERT_NO_MSG(start_addr < end_addr);

	if ((entry_slot_s == NULL) || (entry_slot_e == NULL)) {
		ret = -EINVAL;
		goto out;
	}

	/*
	 * Figure out if we need to add new slots for either addresses.
	 * If the addresses match exactly the addresses current in map,
	 * we can reuse those entries without adding new one.
	 */
	if (!exact_s || !exact_e) {
		uint8_t needed = (exact_s ? 0 : 1) + (exact_e ? 0 : 1);

		/* Check if there are enough empty slots. */
		if (first_enabled_idx < needed) {
			ret = -ENOMEM;
			goto out;
		}
	}

	/*
	 * Need to keep track of the attributes of the memory region before
	 * we start adding entries, as we will need to apply the same
	 * attributes to the "ending address" entry to preseve the attributes
	 * of existing map.
	 */
	prev_entry = *entry_slot_e;

	/*
	 * Entry for beginning of new region.
	 *
	 * - Use existing entry if start addresses are the same for existing
	 *   and incoming region. We can simply reuse the entry.
	 * - Add an entry if incoming region is within existing region.
	 */
	if (!exact_s) {
		/*
		 * Put a new entry before the first enabled entry.
		 * We will sort the entries later.
		 */
		first_enabled_idx--;

		entry_slot_s = &entries[first_enabled_idx];
	}

	xtensa_mpu_entry_set(entry_slot_s, start_addr, true, access_rights, memory_type);

	/*
	 * Entry for ending of region.
	 *
	 * - Add an entry if incoming region is within existing region.
	 * - If the end address matches exactly to existing entry, there is
	 *   no need to do anything.
	 */
	if (!exact_e) {
		/*
		 * Put a new entry before the first enabled entry.
		 * We will sort the entries later.
		 */
		first_enabled_idx--;

		entry_slot_e = &entries[first_enabled_idx];

		/*
		 * Since we are going to punch a hole in the map,
		 * we need to preserve the attribute of existing region
		 * between the end address and next entry.
		 */
		*entry_slot_e = prev_entry;
		xtensa_mpu_entry_start_address_set(entry_slot_e, end_addr);
	}

	/* Sort the entries in ascending order of starting address */
	sort_entries(entries);

	/*
	 * Need to figure out where the start and end entries are as sorting
	 * may change their positions.
	 */
	entry_slot_s = (struct xtensa_mpu_entry *)
		       check_addr_in_mpu_entries(entries, start_addr, first_enabled_idx,
						 &exact_s, &idx_s);
	entry_slot_e = (struct xtensa_mpu_entry *)
		       check_addr_in_mpu_entries(entries, end_addr, first_enabled_idx,
						 &exact_e, &idx_e);

	/* Both must be exact match. */
	__ASSERT_NO_MSG(exact_s);
	__ASSERT_NO_MSG(exact_e);

	if (end_addr == 0xFFFFFFFFU) {
		/*
		 * If end_addr = 0xFFFFFFFFU, entry_slot_e and idx_e both
		 * point to the last slot. Because the incoming region goes
		 * to the end of memory, we simply cheat by including
		 * the last entry by incrementing idx_e so the loop to
		 * update entries will change the attribute of last entry
		 * in map.
		 */
		idx_e++;
	}

	/*
	 * Any existing entries between the "newly" popluated start and
	 * end entries must bear the same attributes. So modify them
	 * here.
	 */
	for (int idx = idx_s + 1; idx < idx_e; idx++) {
		xtensa_mpu_entry_attributes_set(&entries[idx], access_rights, memory_type);
	}

end:
	if (first_idx != NULL) {
		*first_idx = first_enabled_idx;
	}

	ret = 0;

out:
	return ret;
}

/**
 * Write the MPU map to hardware.
 *
 * @param map Pointer to foreground MPU map.
 */
#ifdef CONFIG_USERSPACE
/* With userspace enabled, the pointer to per memory domain MPU map is stashed
 * inside the thread struct. If we still only take struct xtensa_mpu_map as
 * argument, a wrapper function is needed. To avoid the cost associated with
 * calling that wrapper function, takes thread pointer directly as argument
 * when userspace is enabled. Not to mention that writing the map to hardware
 * is already a costly operation per context switch. So every little bit helps.
 */
void xtensa_mpu_map_write(struct k_thread *thread)
#else
void xtensa_mpu_map_write(struct xtensa_mpu_map *map)
#endif
{
	int entry;
	k_spinlock_key_t key;

	key = k_spin_lock(&xtensa_mpu_lock);

#ifdef CONFIG_USERSPACE
	struct xtensa_mpu_map *map = thread->arch.mpu_map;
#endif

	/*
	 * Clear MPU entries first, then write MPU entries in reverse order.
	 *
	 * Remember that the boundary of each memory region is marked by
	 * two consecutive entries, and that the addresses of all entries
	 * must not be in descending order (i.e. equal or increasing).
	 * To ensure this, we clear out the entries first then write them
	 * in reverse order. This avoids any intermediate invalid
	 * configuration with regard to ordering.
	 */
	for (entry = 0; entry < XTENSA_MPU_NUM_ENTRIES; entry++) {
		__asm__ volatile("wptlb %0, %1\n\t" : : "a"(entry), "a"(0));
	}

	for (entry = XTENSA_MPU_NUM_ENTRIES - 1; entry >= 0; entry--) {
		__asm__ volatile("wptlb %0, %1\n\t"
				 : : "a"(map->entries[entry].at), "a"(map->entries[entry].as));
	}

	k_spin_unlock(&xtensa_mpu_lock, key);
}

/**
 * Perform necessary steps to enable MPU.
 */
void xtensa_mpu_init(void)
{
	unsigned int entry;
	uint8_t first_enabled_idx;

	/* Disable all foreground segments before we start configuration. */
	xtensa_mpu_mpuenb_write(0);

	/*
	 * Clear the foreground MPU map so we can populate it later with valid entries.
	 * Note that we still need to make sure the map is valid, and cannot be totally
	 * zeroed.
	 */
	for (entry = 0; entry < XTENSA_MPU_NUM_ENTRIES; entry++) {
		/* Make sure to zero out everything as a start, especially the MBZ fields. */
		struct xtensa_mpu_entry ent = {0};

		/* Segment value must correspond to the index. */
		ent.at.p.segment = entry;

		/* No access at all for both kernel and user modes. */
		ent.at.p.access_rights = XTENSA_MPU_ACCESS_P_NA_U_NA;

		/* Use default memory type for disabled entries. */
		ent.at.p.memory_type = CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE;

		xtensa_mpu_map_fg_kernel.entries[entry] = ent;
	}

#ifndef CONFIG_XTENSA_MPU_ONLY_SOC_RANGES
	/*
	 * Add necessary MPU entries for the memory regions of base Zephyr image.
	 */
	for (entry = 0; entry < ARRAY_SIZE(mpu_zephyr_ranges); entry++) {
		const struct xtensa_mpu_range *range = &mpu_zephyr_ranges[entry];

		int ret = mpu_map_region_add(&xtensa_mpu_map_fg_kernel,
					     range->start, range->end,
					     range->access_rights, range->memory_type,
					     &first_enabled_idx);

		ARG_UNUSED(ret);
		__ASSERT(ret == 0, "Unable to add region [0x%08x, 0x%08x): %d",
				   (unsigned int)range->start,
				   (unsigned int)range->end,
				   ret);
	}
#endif /* !CONFIG_XTENSA_MPU_ONLY_SOC_RANGES */

	/*
	 * Now for the entries for memory regions needed by SoC.
	 */
	for (entry = 0; entry < xtensa_soc_mpu_ranges_num; entry++) {
		const struct xtensa_mpu_range *range = &xtensa_soc_mpu_ranges[entry];

		int ret = mpu_map_region_add(&xtensa_mpu_map_fg_kernel,
					     range->start, range->end,
					     range->access_rights, range->memory_type,
					     &first_enabled_idx);

		ARG_UNUSED(ret);
		__ASSERT(ret == 0, "Unable to add region [0x%08x, 0x%08x): %d",
				   (unsigned int)range->start,
				   (unsigned int)range->end,
				   ret);
	}

	/* Consolidate entries so we have a compact map at boot. */
	consolidate_entries(xtensa_mpu_map_fg_kernel.entries, first_enabled_idx);

	/* Write the map into hardware. There is no turning back now. */
#ifdef CONFIG_USERSPACE
	struct k_thread dummy_map_thread;

	dummy_map_thread.arch.mpu_map = &xtensa_mpu_map_fg_kernel;
	xtensa_mpu_map_write(&dummy_map_thread);
#else
	xtensa_mpu_map_write(&xtensa_mpu_map_fg_kernel);
#endif
}

#ifdef CONFIG_USERSPACE
int arch_mem_domain_init(struct k_mem_domain *domain)
{
	domain->arch.mpu_map = xtensa_mpu_map_fg_kernel;

	return 0;
}

int arch_mem_domain_max_partitions_get(void)
{
	/*
	 * Due to each memory region requiring 2 MPU entries to describe,
	 * it is hard to figure out how many partitions are available.
	 * For example, if all those partitions are contiguous, it only
	 * needs 2 entries (1 if the end of region already has an entry).
	 * If they are all disjoint, it will need (2 * n) entries to
	 * describe all of them. So just use CONFIG_MAX_DOMAIN_PARTITIONS
	 * here and let the application set this instead.
	 */
	return CONFIG_MAX_DOMAIN_PARTITIONS;
}

int arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				     uint32_t partition_id)
{
	int ret;
	uint32_t perm;
	struct k_thread *cur_thread;
	struct xtensa_mpu_map *map = &domain->arch.mpu_map;
	struct k_mem_partition *partition = &domain->partitions[partition_id];
	uintptr_t end_addr = partition->start + partition->size;

	if (end_addr <= partition->start) {
		ret = -EINVAL;
		goto out;
	}

	/*
	 * This is simply to get rid of the user permissions and retain
	 * whatever the kernel permissions are. So that we won't be
	 * setting the memory region permission incorrectly, for example,
	 * marking read only region writable.
	 *
	 * Note that Zephyr does not do RWX partitions so we can treat it
	 * as invalid.
	 */
	switch (partition->attr) {
	case XTENSA_MPU_ACCESS_P_RO_U_NA:
		__fallthrough;
	case XTENSA_MPU_ACCESS_P_RX_U_NA:
		__fallthrough;
	case XTENSA_MPU_ACCESS_P_RO_U_RO:
		__fallthrough;
	case XTENSA_MPU_ACCESS_P_RX_U_RX:
		perm = XTENSA_MPU_ACCESS_P_RO_U_NA;
		break;

	case XTENSA_MPU_ACCESS_P_RW_U_NA:
		__fallthrough;
	case XTENSA_MPU_ACCESS_P_RWX_U_NA:
		__fallthrough;
	case XTENSA_MPU_ACCESS_P_RW_U_RWX:
		__fallthrough;
	case XTENSA_MPU_ACCESS_P_RW_U_RO:
		__fallthrough;
	case XTENSA_MPU_ACCESS_P_RWX_U_RX:
		__fallthrough;
	case XTENSA_MPU_ACCESS_P_RW_U_RW:
		__fallthrough;
	case XTENSA_MPU_ACCESS_P_RWX_U_RWX:
		perm = XTENSA_MPU_ACCESS_P_RW_U_NA;
		break;

	default:
		/* _P_X_U_NA is not a valid permission for userspace, so ignore.
		 * _P_NA_U_X becomes _P_NA_U_NA when removing user permissions.
		 * _P_WO_U_WO has not kernel only counterpart so just force no access.
		 * If we get here with _P_NA_P_NA, there is something seriously
		 * wrong with the userspace and/or application code.
		 */
		perm = XTENSA_MPU_ACCESS_P_NA_U_NA;
		break;
	}

	/*
	 * Reset the memory region attributes by simply "adding"
	 * a region with default attributes. If entries already
	 * exist for the region, the corresponding entries will
	 * be updated with the default attributes. Or new entries
	 * will be added to carve a hole in existing regions.
	 */
	ret = mpu_map_region_add(map, partition->start, end_addr,
				 perm,
				 CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,
				 NULL);

	/*
	 * Need to update hardware MPU regions if we are removing
	 * partition from the domain of the current running thread.
	 */
	cur_thread = _current_cpu->current;
	if (cur_thread->mem_domain_info.mem_domain == domain) {
		xtensa_mpu_map_write(cur_thread);
	}

out:
	return ret;
}

int arch_mem_domain_partition_add(struct k_mem_domain *domain,
				  uint32_t partition_id)
{
	int ret;
	struct k_thread *cur_thread;
	struct xtensa_mpu_map *map = &domain->arch.mpu_map;
	struct k_mem_partition *partition = &domain->partitions[partition_id];
	uintptr_t end_addr = partition->start + partition->size;

	if (end_addr <= partition->start) {
		ret = -EINVAL;
		goto out;
	}

	ret = mpu_map_region_add(map, partition->start, end_addr,
				 (uint8_t)partition->attr,
				 CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,
				 NULL);

	/*
	 * Need to update hardware MPU regions if we are removing
	 * partition from the domain of the current running thread.
	 *
	 * Note that this function can be called with dummy thread
	 * at boot so we need to avoid writing MPU regions to
	 * hardware.
	 */
	cur_thread = _current_cpu->current;
	if (((cur_thread->base.thread_state & _THREAD_DUMMY) != _THREAD_DUMMY) &&
	    (cur_thread->mem_domain_info.mem_domain == domain)) {
		xtensa_mpu_map_write(cur_thread);
	}

out:
	return ret;
}

int arch_mem_domain_thread_add(struct k_thread *thread)
{
	int ret = 0;

	/* New memory domain we are being added to */
	struct k_mem_domain *domain = thread->mem_domain_info.mem_domain;

	/*
	 * this is only set for threads that were migrating from some other
	 * memory domain; new threads this is NULL.
	 */
	struct xtensa_mpu_map *old_map = thread->arch.mpu_map;

	bool is_user = (thread->base.user_options & K_USER) != 0;
	bool is_migration = (old_map != NULL) && is_user;

	uintptr_t stack_end_addr = thread->stack_info.start + thread->stack_info.size;

	if (stack_end_addr < thread->stack_info.start) {
		/* Account for wrapping around back to 0. */
		stack_end_addr = 0xFFFFFFFFU;
	}

	/*
	 * Allow USER access to the thread's stack in its new domain if
	 * we are migrating. If we are not migrating this is done in
	 * xtensa_user_stack_perms().
	 */
	if (is_migration) {
		/* Add stack to new domain's MPU map. */
		ret = mpu_map_region_add(&domain->arch.mpu_map,
					 thread->stack_info.start, stack_end_addr,
					 XTENSA_MPU_ACCESS_P_RW_U_RW,
					 CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,
					 NULL);

		/* Probably this fails due to no more available slots in MPU map. */
		__ASSERT_NO_MSG(ret == 0);
	}

	thread->arch.mpu_map = &domain->arch.mpu_map;

	/*
	 * Remove thread stack from old memory domain if we are
	 * migrating away from old memory domain. This is done
	 * by simply remove USER access from the region.
	 */
	if (is_migration) {
		/*
		 * Remove stack from old MPU map by...
		 * "adding" a new memory region to the map
		 * as this carves a hole in the existing map.
		 */
		ret = mpu_map_region_add(old_map,
					 thread->stack_info.start, stack_end_addr,
					 XTENSA_MPU_ACCESS_P_RW_U_NA,
					 CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,
					 NULL);
	}

	/*
	 * Need to switch to new MPU map if this is the current
	 * running thread.
	 */
	if (thread == _current_cpu->current) {
		xtensa_mpu_map_write(thread);
	}

	return ret;
}

int arch_mem_domain_thread_remove(struct k_thread *thread)
{
	uintptr_t stack_end_addr;
	int ret;

	struct k_mem_domain *domain = thread->mem_domain_info.mem_domain;

	if ((thread->base.user_options & K_USER) == 0) {
		ret = 0;
		goto out;
	}

	if ((thread->base.thread_state & _THREAD_DEAD) == 0) {
		/* Thread is migrating to another memory domain and not
		 * exiting for good; we weren't called from
		 * z_thread_abort().  Resetting the stack region will
		 * take place in the forthcoming thread_add() call.
		 */
		ret = 0;
		goto out;
	}

	stack_end_addr = thread->stack_info.start + thread->stack_info.size;
	if (stack_end_addr < thread->stack_info.start) {
		/* Account for wrapping around back to 0. */
		stack_end_addr = 0xFFFFFFFFU;
	}

	/*
	 * Restore permissions on the thread's stack area since it is no
	 * longer a member of the domain.
	 */
	ret = mpu_map_region_add(&domain->arch.mpu_map,
				 thread->stack_info.start, stack_end_addr,
				 XTENSA_MPU_ACCESS_P_RW_U_NA,
				 CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,
				 NULL);

	xtensa_mpu_map_write(thread);

out:
	return ret;
}

int arch_buffer_validate(const void *addr, size_t size, int write)
{
	uintptr_t aligned_addr;
	size_t aligned_size, addr_offset;
	int ret = 0;

	/* addr/size arbitrary, fix this up into an aligned region */
	aligned_addr = ROUND_DOWN((uintptr_t)addr, XCHAL_MPU_ALIGN);
	addr_offset = (uintptr_t)addr - aligned_addr;
	aligned_size = ROUND_UP(size + addr_offset, XCHAL_MPU_ALIGN);

	for (size_t offset = 0; offset < aligned_size;
	     offset += XCHAL_MPU_ALIGN) {
		uint32_t probed = xtensa_pptlb_probe(aligned_addr + offset);

		if ((probed & XTENSA_MPU_PROBE_VALID_ENTRY_MASK) == 0U) {
			/* There is no foreground or background entry associated
			 * with the region.
			 */
			ret = -EPERM;
			goto out;
		}

		uint8_t access_rights = (probed & XTENSA_MPU_PPTLB_ACCESS_RIGHTS_MASK)
					>> XTENSA_MPU_PPTLB_ACCESS_RIGHTS_SHIFT;

		if (write) {
			/* Need to check write permission. */
			switch (access_rights) {
			case XTENSA_MPU_ACCESS_P_WO_U_WO:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RW_U_RWX:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RW_U_RW:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RWX_U_RWX:
				/* These permissions are okay. */
				break;
			default:
				ret = -EPERM;
				goto out;
			}
		} else {
			/* Only check read permission. */
			switch (access_rights) {
			case XTENSA_MPU_ACCESS_P_RW_U_RWX:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RW_U_RO:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RWX_U_RX:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RO_U_RO:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RX_U_RX:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RW_U_RW:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RWX_U_RWX:
				/* These permissions are okay. */
				break;
			default:
				ret = -EPERM;
				goto out;
			}
		}
	}

out:
	return ret;
}

bool xtensa_mem_kernel_has_access(void *addr, size_t size, int write)
{
	uintptr_t aligned_addr;
	size_t aligned_size, addr_offset;
	bool ret = true;

	/* addr/size arbitrary, fix this up into an aligned region */
	aligned_addr = ROUND_DOWN((uintptr_t)addr, XCHAL_MPU_ALIGN);
	addr_offset = (uintptr_t)addr - aligned_addr;
	aligned_size = ROUND_UP(size + addr_offset, XCHAL_MPU_ALIGN);

	for (size_t offset = 0; offset < aligned_size;
	     offset += XCHAL_MPU_ALIGN) {
		uint32_t probed = xtensa_pptlb_probe(aligned_addr + offset);

		if ((probed & XTENSA_MPU_PROBE_VALID_ENTRY_MASK) == 0U) {
			/* There is no foreground or background entry associated
			 * with the region.
			 */
			ret = false;
			goto out;
		}

		uint8_t access_rights = (probed & XTENSA_MPU_PPTLB_ACCESS_RIGHTS_MASK)
					>> XTENSA_MPU_PPTLB_ACCESS_RIGHTS_SHIFT;


		if (write != 0) {
			/* Need to check write permission. */
			switch (access_rights) {
			case XTENSA_MPU_ACCESS_P_RW_U_NA:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RWX_U_NA:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_WO_U_WO:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RW_U_RWX:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RW_U_RO:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RWX_U_RX:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RW_U_RW:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RWX_U_RWX:
				/* These permissions are okay. */
				break;
			default:
				ret = false;
				goto out;
			}
		} else {
			/* Only check read permission. */
			switch (access_rights) {
			case XTENSA_MPU_ACCESS_P_RO_U_NA:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RX_U_NA:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RW_U_NA:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RWX_U_NA:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RW_U_RWX:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RW_U_RO:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RWX_U_RX:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RO_U_RO:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RX_U_RX:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RW_U_RW:
				__fallthrough;
			case XTENSA_MPU_ACCESS_P_RWX_U_RWX:
				/* These permissions are okay. */
				break;
			default:
				ret = false;
				goto out;
			}
		}
	}

out:
	return ret;
}


void xtensa_user_stack_perms(struct k_thread *thread)
{
	int ret;

	uintptr_t stack_end_addr = thread->stack_info.start + thread->stack_info.size;

	if (stack_end_addr < thread->stack_info.start) {
		/* Account for wrapping around back to 0. */
		stack_end_addr = 0xFFFFFFFFU;
	}

	(void)memset((void *)thread->stack_info.start,
		     (IS_ENABLED(CONFIG_INIT_STACKS)) ? 0xAA : 0x00,
		     thread->stack_info.size - thread->stack_info.delta);

	/* Add stack to new domain's MPU map. */
	ret = mpu_map_region_add(thread->arch.mpu_map,
				 thread->stack_info.start, stack_end_addr,
				 XTENSA_MPU_ACCESS_P_RW_U_RW,
				 CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,
				 NULL);

	xtensa_mpu_map_write(thread);

	/* Probably this fails due to no more available slots in MPU map. */
	ARG_UNUSED(ret);
	__ASSERT_NO_MSG(ret == 0);
}

#endif /* CONFIG_USERSPACE */
