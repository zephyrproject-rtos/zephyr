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
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util_macro.h>

#include <xtensa/corebits.h>
#include <xtensa/config/core-matmap.h>
#include <xtensa/config/core-isa.h>
#include <xtensa_mpu_priv.h>

#ifdef CONFIG_USERSPACE
BUILD_ASSERT((CONFIG_PRIVILEGED_STACK_SIZE > 0) &&
	     (CONFIG_PRIVILEGED_STACK_SIZE % XCHAL_MPU_ALIGN) == 0);
#endif

/* End of memory address */
#define END_OF_MEMORY_ADDR (0xFFFFFFFFU)

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
 *   defined in the processor configuration.
 */

/**
 * Find the memory type of a region according to xtensa_mpu_mem_type_ranges[].
 *
 * @param[in] start Start address of the region.
 * @param[in] end End address of the region.
 */
static uint32_t find_memory_type(const uintptr_t start, const uintptr_t end)
{
	uint16_t type = XTENSA_MPU_DEFAULT_MEMORY_TYPE;

	/* Need to go through the whole list as it is possible that later
	 * entry in the array overrides previous entries, similar to how
	 * we process the array at boot.
	 */
	for (unsigned int i = 0; i < xtensa_mpu_mem_type_ranges_num; i++) {
		const struct xtensa_mpu_mem_type_region *region = &xtensa_mpu_mem_type_ranges[i];

		if (IN_RANGE(start, region->start, region->end) &&
		    IN_RANGE(end, region->start, region->end)) {
			type = (uint32_t)region->memory_type;
		}
	}

	return type;
}

/**
 * Return the pointer to the entry matching exactly @addr out of an array of MPU entries.
 *
 * @param[in]  entries Array of MPU entries.
 * @param[in]  addr Address to be matched to one background entry.
 * @param[in]  first_enabled_idx The index of the first enabled entry.
 *                               Use 0 if not sure.
 * @param[out] entry_idx Set to the index of the entry array if entry is found.
 *
 * @return Pointer to the map entry encompassing @a addr, or NULL if no such entry found.
 */
static struct xtensa_mpu_entry *find_exact_entry(struct xtensa_mpu_entry *entries, uintptr_t addr,
						 uint8_t first_enabled_idx, uint8_t *entry_idx)
{
	unsigned int idx;
	uintptr_t entry_addr;
	struct xtensa_mpu_entry *ret = NULL;

	/* Loop through the map to find exact address match. */
	for (idx = first_enabled_idx; idx < XTENSA_MPU_NUM_ENTRIES; idx++) {
		entry_addr = xtensa_mpu_entry_start_address_get(&entries[idx]);

		if (addr == entry_addr) {
			ret = &entries[idx];
			*entry_idx = idx;
			break;
		}
	}

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
static void sort_entries(struct xtensa_mpu_entry *entries, uint8_t first_enabled_idx)
{
	qsort(&entries[first_enabled_idx], (XTENSA_MPU_NUM_ENTRIES - first_enabled_idx),
	      sizeof(entries[0]), compare_entries);

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
 * @param[in] entries Array of MPU entries with XTENSA_MPU_NUM_ENTRIES elements.
 * @param[in,out] first_enabled_idx Index of first enabled entry. The new index
 *                                  is written into this.
 *
 * @retval true If any consolidation was done.
 * @retval false If no consolidation was done.
 */
static bool consolidate_entries(struct xtensa_mpu_entry *entries, uint8_t *first_enabled_idx)
{
	uint8_t new_first = *first_enabled_idx;
	uint8_t idx_0 = *first_enabled_idx;
	uint8_t idx_1 = idx_0 + 1;
	bool has_consolidated = false;

	/* For each a pair of entries... */
	while (idx_1 < XTENSA_MPU_NUM_ENTRIES) {
		struct xtensa_mpu_entry *entry_0 = &entries[idx_0];
		struct xtensa_mpu_entry *entry_1 = &entries[idx_1];
		struct xtensa_mpu_entry *entry_to_be_removed;

		if (xtensa_mpu_entries_has_same_attributes(entry_0, entry_1)) {
			/*
			 * If both entry has same attributes (access_rights and memory type),
			 * they can be consolidated into one by removing the higher indexed
			 * one.
			 */
			entry_to_be_removed = entry_1;
		} else if (xtensa_mpu_entries_has_same_address(entry_0, entry_1)) {
			/*
			 * If both entries have the same address, the higher index
			 * one always override the lower one. So remove the lower indexed
			 * one.
			 */
			entry_to_be_removed = entry_0;
		} else {
			/* Nothing to remove, So move on to next pair. */
			idx_0 = idx_1;
			idx_1++;

			continue;
		}

		/*
		 * We now have an entry to remove.
		 */

		/* Zero out everything so it will bubble up in the map. */
		entry_to_be_removed->as.raw = 0;
		entry_to_be_removed->at.raw = 0;

		/* Use default access rights for disabled entries. */
		entry_to_be_removed->at.p.access_rights = XTENSA_MPU_DEFAULT_ACCESS_RIGHTS;

		/* Use default memory type for disabled entries. */
		entry_to_be_removed->at.p.memory_type = XTENSA_MPU_DEFAULT_MEMORY_TYPE;

		sort_entries(entries, new_first);

		/* One fewer enabled entry. */
		new_first++;

		/* Restart from first enabled entry as there may be new opportunity to
		 * further compact the map.
		 */
		idx_0 = new_first;
		idx_1 = idx_0 + 1;

		has_consolidated = true;
	}

	/* Update the newly first enabled entry index if we have consolidated entries. */
	if (has_consolidated) {
		*first_enabled_idx = new_first;
	}

	return has_consolidated;
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
 * @param[out] first_idx Return index of first enabled entry if not NULL.
 *
 * @retval 0 Successful in adding the region.
 * @retval -EINVAL Invalid values in function arguments.
 */
static int mpu_map_region_add(struct xtensa_mpu_map *map,
			      uintptr_t start_addr, uintptr_t end_addr,
			      uint32_t access_rights, uint8_t *first_idx)
{
	int ret;
	uint8_t idx_s, first_enabled_idx;
	uint8_t idx_e = 0;
	uint32_t memory_type;
	struct xtensa_mpu_entry *entry_slot_s, *entry_slot_e = NULL;

	struct xtensa_mpu_entry *entries = map->entries;

	if (start_addr >= end_addr) {
		ret = -EINVAL;
		goto out;
	}

	memory_type = find_memory_type(start_addr, end_addr);

	first_enabled_idx = find_first_enabled_entry(entries);
	if (first_enabled_idx >= XTENSA_MPU_NUM_ENTRIES) {
		/*
		 * Map is empty and has not been populated. We can simply
		 * put the entries at the end of map.
		 */

		first_enabled_idx = XTENSA_MPU_NUM_ENTRIES;

		/*
		 * If end address is the end of memory, we don't need an entry as
		 * the last entry in map covers through the end of memory.
		 */
		if (end_addr != END_OF_MEMORY_ADDR) {
			first_enabled_idx--;

			entry_slot_e = &entries[first_enabled_idx];

			/*
			 * Note that the ending entry is not enabled as it is merely
			 * marking the end of a region and is not the starting of
			 * another enabled MPU region.
			 */
			xtensa_mpu_entry_set(entry_slot_e, end_addr, false,
					     XTENSA_MPU_DEFAULT_ACCESS_RIGHTS,
					     XTENSA_MPU_DEFAULT_MEMORY_TYPE);
		}

		/* Add the starting entry. */
		first_enabled_idx--;

		entry_slot_s = &entries[first_enabled_idx];

		xtensa_mpu_entry_set(entry_slot_s, start_addr, true, access_rights, memory_type);

		/*
		 * Since we started with an empty map, adding a region is simple
		 * and we can bail at this point.
		 */
		goto end;
	}

	do {
		/*
		 * Figure out if we need to add new slots for either addresses.
		 * If the addresses match exactly the addresses current in map,
		 * we can reuse those entries without adding new one.
		 * If not, we need to find some free slots to insert new entries.
		 */

		entry_slot_s = find_exact_entry(entries, start_addr, first_enabled_idx, &idx_s);

		if (end_addr != END_OF_MEMORY_ADDR) {
			/*
			 * No need to add an entry for end of memory as it is covered by the last
			 * entry.
			 */
			entry_slot_e = find_exact_entry(entries, end_addr, first_enabled_idx,
							&idx_e);
		}

		/* One or both have no exact match. Need to find free slots. */
		if ((entry_slot_s == NULL) || (entry_slot_e == NULL)) {
			uint8_t needed = ((entry_slot_s == NULL) ? 1 : 0) +
					 ((entry_slot_e == NULL) ? 1 : 0);

			/* Check if there are enough empty slots. */
			if (first_enabled_idx < needed) {
				bool success;

				/* Try compacting the MPU map to free up some slots. */
				success = consolidate_entries(entries, &first_enabled_idx);
				if (success) {
					/* Try to find free slots after map is consolidated. */
					continue;
				}

				/*
				 * If we still do not have enough slots after compacting
				 * the MPU map. We bail.
				 */
				ret = -ENOSPC;
				goto out;
			}
		}

		/* We have enough information and space to proceed. */
		break;
	} while (true);

	/*
	 * Entry for ending of the region.
	 *
	 * If the end address matches one of the entry (entry_slot_e != NULL), we resue
	 * this existing entry.
	 *
	 * If there is no exact address match in the map (entry_slot_e == NULL), we need to
	 * add a new entry in the map.
	 *
	 * Note that we need to preserve access rights and memory type of existing MPU map
	 * which will be done below.
	 *
	 * If the ending address is the end of memory, we don't have to worry about it as
	 * the last entry in the map covers through the end of map. Though we need to make
	 * sure the last entry is enabled and has the correct attributes, which will also
	 * be done below.
	 */
	if ((end_addr != END_OF_MEMORY_ADDR) && (entry_slot_e == NULL)) {
		struct xtensa_mpu_entry *prev_entry;

		/*
		 * Put a new entry before the first enabled entry.
		 * We will sort the entries later.
		 */
		first_enabled_idx--;

		entry_slot_e = &entries[first_enabled_idx];

		/*
		 * Access rights and memory type will be populated later
		 * as they need to preserve existing attributes. But be safe
		 * and use default for now.
		 */
		xtensa_mpu_entry_set(entry_slot_e, end_addr, false,
				     XTENSA_MPU_DEFAULT_ACCESS_RIGHTS,
				     XTENSA_MPU_DEFAULT_MEMORY_TYPE);

		/* Sort the list with newly added entry and find its position again. */
		sort_entries(entries, first_enabled_idx);
		entry_slot_e = find_exact_entry(entries, end_addr, first_enabled_idx, &idx_e);

		if (unlikely(entry_slot_e == NULL)) {
			ret = -EFAULT;
			goto out;
		}

		if (entry_slot_s != NULL) {
			/* Starting slot has moved too. So find it again. */
			entry_slot_s = find_exact_entry(entries, start_addr, first_enabled_idx,
							&idx_s);

			if (unlikely(entry_slot_s == NULL)) {
				ret = -EFAULT;
				goto out;
			}
		}

		/*
		 * Preserve access rights and memory type of existing map if there is
		 * an existing entry before the newly added one. If the new one is
		 * at the beginning of the map, there is no existing attributes to
		 * preserve so we use the default attributes set above via
		 * xtensa_mpu_entry_set().
		 */
		if (idx_e != first_enabled_idx) {
			prev_entry = &entries[idx_e - 1];

			xtensa_mpu_entry_enable_set(entry_slot_e,
					xtensa_mpu_entry_enable_get(prev_entry));
			xtensa_mpu_entry_access_rights_set(entry_slot_e,
					xtensa_mpu_entry_access_rights_get(prev_entry));
			xtensa_mpu_entry_memory_type_set(entry_slot_e,
					xtensa_mpu_entry_memory_type_get(prev_entry));
		}
	}

	/*
	 * Entry for beginning of the region.
	 *
	 * If the start address matches one of the entry (entry_slot_s != NULL), we resue
	 * this existing entry.
	 *
	 * If there is no exact address match in the map (entry_slot_s == NULL), we need to
	 * add a new entry in the map.
	 */
	if (entry_slot_s == NULL) {
		/*
		 * Put a new entry before the first enabled entry.
		 * We will sort the entries later.
		 */
		first_enabled_idx--;

		entry_slot_s = &entries[first_enabled_idx];

		/* Set the attributes of the new entry */
		xtensa_mpu_entry_set(entry_slot_s, start_addr, true, access_rights, memory_type);

		/* Sort the list with newly added entry and find its position again. */
		sort_entries(entries, first_enabled_idx);
		entry_slot_s = find_exact_entry(entries, start_addr, first_enabled_idx, &idx_s);

		if (unlikely(entry_slot_s == NULL)) {
			ret = -EFAULT;
			goto out;
		}
	} else {
		/* Update attributes if there is an existing entry. */
		xtensa_mpu_entry_set(entry_slot_s, start_addr, true, access_rights, memory_type);
	}

	if (end_addr == END_OF_MEMORY_ADDR) {
		/*
		 * If end_addr = 0xFFFFFFFFU, the incoming region goes to the end of memory.
		 * We set idx_e to beyond the entries so the loop below will go through
		 * all entries after the starting one to the end of map.
		 */
		idx_e = XCHAL_MPU_ENTRIES;
	}

	/*
	 * Any existing entries between the "newly" popluated start and
	 * end entries must bear the same attributes and enabled.
	 * So modify them here.
	 */
	for (int idx = idx_s + 1; idx < idx_e; idx++) {
		xtensa_mpu_entry_attributes_set(&entries[idx], access_rights, memory_type);
		xtensa_mpu_entry_enable_set(&entries[idx], true);
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
static inline void mpu_map_write(struct xtensa_mpu_map *map)
{
	int entry;
	k_spinlock_key_t key;

	key = k_spin_lock(&xtensa_mpu_lock);

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

#ifdef CONFIG_USERSPACE
/**
 * Write the MPU map associated with the thread.
 *
 * @param thread Pointer to thread with MPU map.
 */
void xtensa_mpu_thread_map_write(struct k_thread *thread)
{
	struct xtensa_mpu_map *map = thread->arch.mpu_map;

	mpu_map_write(map);
}
#endif /* CONFIG_USERSPACE */

/**
 * Perform necessary steps to enable MPU.
 */
void xtensa_mpu_init(void)
{
	unsigned int entry;
	uint8_t first_enabled_idx = XTENSA_MPU_NUM_ENTRIES;

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

		/* Use default access rights for disabled entries. */
		ent.at.p.access_rights = XTENSA_MPU_DEFAULT_ACCESS_RIGHTS;

		/* Use default memory type for disabled entries. */
		ent.at.p.memory_type = XTENSA_MPU_DEFAULT_MEMORY_TYPE;

		xtensa_mpu_map_fg_kernel.entries[entry] = ent;
	}

	/*
	 * Add necessary MPU entries for the memory regions of Zephyr image.
	 */
	for (entry = 0; entry < (unsigned int)xtensa_mpu_ranges_num; entry++) {
		const struct xtensa_mpu_range *range = &xtensa_mpu_ranges[entry];

		int ret = mpu_map_region_add(&xtensa_mpu_map_fg_kernel,
					     range->start, range->end,
					     range->access_rights, &first_enabled_idx);

		ARG_UNUSED(ret);
		__ASSERT(ret == 0, "Unable to add region [0x%08x, 0x%08x): %d",
				   (unsigned int)range->start,
				   (unsigned int)range->end,
				   ret);
	}

	/* Consolidate entries so we have a compact map at boot. */
	(void)consolidate_entries(xtensa_mpu_map_fg_kernel.entries, &first_enabled_idx);

	/* Write the map into hardware. There is no turning back now. */
	mpu_map_write(&xtensa_mpu_map_fg_kernel);
}

#ifdef CONFIG_USERSPACE
/**
 * Find the permission of a region at boot according to xtensa_mpu_ranges[].
 *
 * @param[in] start Start address of the region.
 * @param[in] end End address of the region.
 */
static uint32_t find_boot_permission(const uintptr_t start, const uintptr_t end)
{
	/* Default is kernel only RW, same as background map. */
	uint32_t perms = XTENSA_MPU_ACCESS_P_RW_U_NA;

	/* Need to go through the whole list as it is possible that later
	 * entry in the array overrides previous entries, similar to how
	 * we process the array at boot.
	 */
	for (int i = 0; i < xtensa_mpu_ranges_num; i++) {
		const struct xtensa_mpu_range *region = &xtensa_mpu_ranges[i];

		if (IN_RANGE(start, region->start, region->end) &&
		    IN_RANGE(end, region->start, region->end)) {
			perms = region->access_rights;
		}
	}

	return perms;
}

/**
 * Restore permissions of a memory region in the MPU map.
 *
 * This restores the permissions of a memory region back to what is
 * programmed at boot.
 *
 * @param[in,out] map Pointer to MPU map.
 * @param[in] start_addr Start address of the region.
 * @param[in] end_addr End address of the region.
 * @param[in] access_rights Access rights of this region.
 *
 * @retval 0 Successful in adding the region.
 * @retval -EINVAL Invalid values in function arguments.
 */
static int mpu_map_region_restore(struct xtensa_mpu_map *map,
				  uintptr_t start_addr, uintptr_t end_addr)
{
	uint32_t access_rights = find_boot_permission(start_addr, end_addr);

	return mpu_map_region_add(map, start_addr, end_addr, access_rights, NULL);
}

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
	ret = mpu_map_region_add(map, partition->start, end_addr, perm, NULL);

	/*
	 * Need to update hardware MPU regions if we are removing
	 * partition from the domain of the current running thread.
	 */
	cur_thread = _current_cpu->current;
	if (cur_thread->mem_domain_info.mem_domain == domain) {
		xtensa_mpu_thread_map_write(cur_thread);
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
				 (uint8_t)partition->attr, NULL);

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
		xtensa_mpu_thread_map_write(cur_thread);
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

	thread->arch.mpu_map = &domain->arch.mpu_map;

	/*
	 * If we are migrating from one memory domain to another, we need to
	 * allow USER access to the thread's stack in its new domain and remove
	 * access from the old domain.
	 *
	 * If we are not migrating, adding stack access permission is done in
	 * xtensa_user_stack_perms().
	 */
	if (is_migration) {
		uintptr_t stack_end_addr = thread->stack_info.start + thread->stack_info.size;

		if (stack_end_addr < thread->stack_info.start) {
			/* Account for wrapping around back to 0. */
			stack_end_addr = END_OF_MEMORY_ADDR;
		}

		/* Add stack to new domain's MPU map. */
		ret = mpu_map_region_add(&domain->arch.mpu_map,
					 thread->stack_info.start, stack_end_addr,
					 XTENSA_MPU_ACCESS_P_RW_U_RW,
					 NULL);

		/* Probably this fails due to no more available slots in MPU map. */
		__ASSERT_NO_MSG(ret == 0);

		/*
		 * Remove stack from old MPU map by...
		 * "adding" a new memory region to the map
		 * as this carves a hole in the existing map.
		 */
		ret = mpu_map_region_restore(old_map,
					     thread->stack_info.start, stack_end_addr);

		/* Probably this fails due to no more available slots in MPU map. */
		__ASSERT_NO_MSG(ret == 0);
	}

	/*
	 * Need to switch to new MPU map if this is the current
	 * running thread.
	 */
	if (thread == _current_cpu->current) {
		xtensa_mpu_thread_map_write(thread);
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
		stack_end_addr = END_OF_MEMORY_ADDR;
	}

	/*
	 * Restore permissions on the thread's stack area since it is no
	 * longer a member of the domain.
	 */
	ret = mpu_map_region_restore(&domain->arch.mpu_map,
				     thread->stack_info.start, stack_end_addr);

	xtensa_mpu_thread_map_write(thread);

out:
	return ret;
}

int arch_buffer_validate(const void *addr, size_t size, int write)
{
	uintptr_t aligned_addr;
	size_t aligned_size, addr_offset;
	int ret = -EINVAL;

	/* addr/size arbitrary, fix this up into an aligned region */
	aligned_addr = ROUND_DOWN((uintptr_t)addr, XCHAL_MPU_ALIGN);
	addr_offset = (uintptr_t)addr - aligned_addr;

	if (size_add_overflow(size, addr_offset, &aligned_size)) {
		goto out;
	}

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

	ret = 0;

out:
	return ret;
}

bool xtensa_buffer_is_kernel_readable(const void *addr, size_t size)
{
	uintptr_t aligned_addr;
	size_t aligned_size, addr_offset;
	bool ret = false;

	/* addr/size arbitrary, fix this up into an aligned region */
	aligned_addr = ROUND_DOWN((uintptr_t)addr, XCHAL_MPU_ALIGN);
	addr_offset = (uintptr_t)addr - aligned_addr;

	if (size_add_overflow(size, addr_offset, &aligned_size)) {
		goto out;
	}

	aligned_size = ROUND_UP(size + addr_offset, XCHAL_MPU_ALIGN);

	for (size_t offset = 0; offset < aligned_size; offset += XCHAL_MPU_ALIGN) {
		uint32_t probed = xtensa_pptlb_probe(aligned_addr + offset);

		if ((probed & XTENSA_MPU_PROBE_VALID_ENTRY_MASK) == 0U) {
			/* There is no foreground or background entry associated
			 * with the region.
			 */
			goto out;
		}

		uint8_t access_rights = (probed & XTENSA_MPU_PPTLB_ACCESS_RIGHTS_MASK) >>
					XTENSA_MPU_PPTLB_ACCESS_RIGHTS_SHIFT;

		/* The region must be readable in privileged (kernel) mode. */
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
			goto out;
		}
	}

	ret = true;

out:
	return ret;
}

void xtensa_user_stack_perms(struct k_thread *thread)
{
	int ret;

	uintptr_t stack_end_addr = thread->stack_info.start + thread->stack_info.size;

	if (stack_end_addr < thread->stack_info.start) {
		/* Account for wrapping around back to 0. */
		stack_end_addr = END_OF_MEMORY_ADDR;
	}

	(void)memset((void *)thread->stack_info.start,
		     (IS_ENABLED(CONFIG_INIT_STACKS)) ? 0xAA : 0x00,
		     thread->stack_info.size - thread->stack_info.delta);

	/* Add stack to new domain's MPU map. */
	ret = mpu_map_region_add(thread->arch.mpu_map,
				 thread->stack_info.start, stack_end_addr,
				 XTENSA_MPU_ACCESS_P_RW_U_RW,
				 NULL);

	/*
	 * Probably this fails due to no more available slots in MPU map.
	 * We cannot proceed with this thread as memory access permissions
	 * would be all wrong. So we do a kernel OOPS here.
	 *
	 * Note that we cannot use ASSERT at this point as we are running in
	 * the privilege stack and yet the thread USER flag enabled. Any access
	 * to the runtime built assertion string will fail arch_buffer_validate(),
	 * resulting in cryptic error messages.
	 *
	 * Another note is that we use arch_syscall_oops() directly instead of
	 * K_OOPS() because K_OOPS() passes _current->syscall_frame to
	 * arch_syscall_oops() but we are not sure if ->syscall_frame has been
	 * setup correctly (or rather, cleared when we are here). Just to be safe
	 * so we call arch_syscall_oops(NULL) directly here.
	 */
	if (ret != 0) {
		arch_syscall_oops(NULL);
	}
}

#endif /* CONFIG_USERSPACE */
